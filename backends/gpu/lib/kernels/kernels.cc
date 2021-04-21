// Copyright 2020 The TensorFlow Runtime Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//===- kernels.cc - CUDA runtime interface --------------------------------===//
//
// This file defines the C++ functions that implement the kernels provided by
// the TFRT CUDA runtime.

#include "kernels.h"

#include <memory>
#include <tuple>

#include "llvm/Support/Error.h"
#include "llvm_derived/Support/raw_ostream.h"
#include "tfrt/dtype/dtype.h"
#include "tfrt/gpu/event_manager.h"
#include "tfrt/gpu/gpu_types.h"
#include "tfrt/gpu/memory/gpu_buffer.h"
#include "tfrt/gpu/tensor/dense_gpu_tensor.h"
#include "tfrt/gpu/wrapper/cuda_wrapper.h"
#include "tfrt/host_context/async_dispatch.h"
#include "tfrt/host_context/attribute_utils.h"
#include "tfrt/host_context/kernel_registry.h"
#include "tfrt/host_context/kernel_utils.h"
#include "tfrt/support/error_util.h"
#include "tfrt/support/ref_count.h"
#include "tfrt/tensor/dense_host_tensor.h"
#include "tfrt/tensor/tensor_shape.h"

namespace tfrt {
namespace gpu {

// tfrt_gpu.init initializes CUDA driver.
static Error CudaInit() { return wrapper::Init(wrapper::Platform::CUDA); }

// tfrt_gpu.device.get returns the CUDA Device at the given index.
static Expected<wrapper::Device> CudaDeviceGet(int32_t ordinal) {
  return wrapper::DeviceGet(wrapper::Platform::CUDA, ordinal);
}

// tfrt_gpu.context.create creates a CUDA context for the given
// device.
static Expected<GpuContext> CudaContextCreate(wrapper::Device device) {
  auto context = wrapper::CtxCreate(wrapper::CtxFlags::SCHED_AUTO, device);
  if (!context) return context.takeError();
  return GpuContext(std::move(*context));
}

// tfrt_gpu.stream.create creates a new stream that does not implicitly
// synchronize with stream 0.
static Expected<GpuStream> CudaStreamCreate(Argument<GpuContext> context) {
  auto current = wrapper::CtxSetCurrent(context->get());
  if (!current) return current.takeError();
  auto stream =
      wrapper::StreamCreate(*current, wrapper::StreamFlags::NON_BLOCKING);
  if (!stream) return stream.takeError();
  return GpuStream(context.ValueRef(), std::move(*stream));
}

// tfrt_gpu.stream.synchronize waits until all stream's tasks are completed.
//
// Result: Sets the output chain when all tasks submitted on a stream are
// completed. This kernel will block the caller thread.
static Error CudaStreamSynchronizeSync(const GpuStream& stream) {
  return wrapper::StreamSynchronize(stream.get());
}
static void CudaStreamSynchronizeAsync(Argument<GpuStream> stream,
                                       Chain in_chain, Result<Chain> out_chain,
                                       const ExecutionContext& exec_ctx) {
  auto result = out_chain.Allocate();
  bool enqueued = EnqueueBlockingWork(
      exec_ctx, [result = result.CopyRef(), stream = stream.ValueRef(),
                 in_chain = in_chain]() mutable {
        if (auto error = CudaStreamSynchronizeSync(*stream))
          return result.SetError(StrCat(error));
        result.emplace(in_chain);
      });
  if (!enqueued) return result.SetError("Failed to enqueue blocking work.");
}

// tfrt_gpu.event.create creates a new cuda event.
//
// Result: new cuda event.
static Expected<GpuEvent> CudaEventCreate(Argument<GpuContext> context) {
  auto current = wrapper::CtxSetCurrent(context->get());
  if (!current) return current.takeError();
  auto event =
      wrapper::EventCreate(*current, wrapper::EventFlags::DISABLE_TIMING);
  if (!event) return event.takeError();
  return GpuEvent(context.ValueRef(), std::move(*event));
}

// tfrt_gpu.event.create creates a new cuda event.
//
// Result: new cuda event.
static Error CudaEventRecord(const GpuEvent& event, const GpuStream& stream) {
  return wrapper::EventRecord(event.get(), stream.get());
}

// tfrt_gpu.event.synchronize sets the output chain when the event has been
// reached, i.e. all work scheduled prior to the last call to
// tfrt_gpu.event.record has been completed.
static Error CudaEventSynchronizeSync(const GpuEvent& event) {
  return wrapper::EventSynchronize(event.get());
}
static void CudaEventSynchronizeAsync(Argument<GpuEvent> event, Chain in_chain,
                                      Result<Chain> out_chain,
                                      const ExecutionContext& exec_ctx) {
  auto result = out_chain.Allocate();
  // Check if event has already completed and we can skip enqueuing work.
  auto ready = wrapper::EventQuery(event->get());
  if (!ready) return result.SetError(StrCat(ready.takeError()));
  if (*ready) return result.emplace(in_chain);
  bool enqueued = EnqueueBlockingWork(
      exec_ctx, [result = result.CopyRef(), event = event.ValueRef(),
                 in_chain = in_chain]() mutable {
        if (auto error = CudaEventSynchronizeSync(*event))
          return result.SetError(StrCat(error));
        result.emplace(in_chain);
      });
  if (!enqueued) return result.SetError("Failed to enqueue blocking work.");
}

// tfrt_gpu.allocator.create creates a new allocator.
//
// Result: new allocator.
static Expected<GpuAllocator> CudaAllocatorCreate(
    Argument<GpuContext> context) {
  return GpuAllocator(context.ValueRef());
}

// tfrt_gpu.mem.allocate allocates a new CUDA buffer.
static Expected<GpuBuffer> CudaMemAllocate(Argument<GpuAllocator> allocator,
                                           const GpuStream& stream,
                                           int64_t size) {
  return GpuBuffer::Allocate(allocator.ValueRef(), size, stream.get());
}

// tfrt_gpu.mem.print_metadata prints `buffer`'s metadata.
static void CudaMemPrintMetadata(const GpuBuffer& buffer) {
  // The check for buffer validity is not done intentionally. Printing invalid
  // buffers can be useful for debugging.
  tfrt::outs() << "GpuBuffer<pointer=" << buffer.pointer()
               << ", size=" << buffer.size() << ">";
  tfrt::outs().flush();
}

// tfrt_gpu.tensor.make creates a tensor of type T from a shape and buffer.
template <typename T>
static Expected<DenseGpuTensor> CudaTensorMake(Argument<GpuBuffer> buffer,
                                               TensorShape shape) {
  if (!*buffer) {
    return MakeStringError(
        "Cannot make tensor from invalid (moved from?) buffer");
  }
  if (buffer->size() != shape.GetNumElements() * GetDType<T>().GetHostSize()) {
    return MakeStringError(
        "tfrt_gpu.tensor.make failed: buffer_size (", buffer->size(),
        ") is not equal to the number of elements in shape (", shape,
        ") times element size (", GetDType<T>().GetHostSize(), ")");
  }
  GpuCrtBuffer::Deallocator deallocator(
      [buffer = buffer.ValueRef()](GpuCrtBuffer*) {
        if (auto error = buffer->Deallocate()) TFRT_LOG(ERROR) << error;
      });
  auto crt_buffer = TakeRef(new GpuCrtBuffer(buffer->pointer(), buffer->size(),
                                             std::move(deallocator)));
  return DenseGpuTensor(shape, GetDType<T>(), std::move(crt_buffer));
}

// tfrt_gpu.tensor.print_metadata prints `tensor`'s metadata.
static void CudaTensorPrintMetadata(const DenseGpuTensor& tensor) {
  (tfrt::outs() << tensor << "\n").flush();
}

static Error CheckMemcpySizes(size_t dst_size, size_t src_size,
                              int64_t copy_size) {
  if (src_size < copy_size) {
    return MakeStringError("source buffer is smaller (", src_size,
                           ") than number of bytes to copy (", copy_size, ")");
  }
  if (dst_size < copy_size) {
    return MakeStringError("destination buffer is smaller (", dst_size,
                           ") than number of bytes to copy (", copy_size, ")");
  }
  return Error::success();
}

// tfrt_gpu.mem.copy_host_to_device copies memory from host to device.
static Error CudaMemcpyHtoD(const GpuContext& context, const GpuBuffer& dst,
                            const RCReference<HostBuffer>& src,
                            int64_t bytes_count, const GpuStream& stream) {
  if (auto error = CheckMemcpySizes(dst.size(), src->size(), bytes_count))
    return error;
  auto current = wrapper::CtxSetCurrent(context.get());
  if (!current) return current.takeError();
  return wrapper::MemcpyAsync(
      *current, dst.pointer(),
      wrapper::Pointer<const void>(src->data(), context->platform()),
      bytes_count, stream.get());
}

// tfrt_gpu.mem.copy_host_to_device copies memory from host to device.
static Error CudaMemcpyDtoH(const GpuContext& context,
                            const RCReference<HostBuffer>& dst,
                            const GpuBuffer& src, int64_t bytes_count,
                            const GpuStream& stream) {
  if (auto error = CheckMemcpySizes(dst->size(), src.size(), bytes_count))
    return error;
  auto current = wrapper::CtxSetCurrent(context.get());
  if (!current) return current.takeError();
  return wrapper::MemcpyAsync(*current,
                              GpuPointer(dst->data(), context->platform()),
                              src.pointer(), bytes_count, stream.get());
}

static Expected<wrapper::Function> CudaFunctionLoad(
    Argument<GpuContext> context,
    // Note: Attributes must be in alphabetical order (see b/140896071).
    StringAttribute data, Attribute<uint64_t> key, StringAttribute name) {
  return context->GetFunction(key.get(), data.get(), name.get());
}

static Error CudaFunctionLaunch(const GpuStream& stream, GpuFunction function,
                                uint32_t grid_dim_x, uint32_t grid_dim_y,
                                uint32_t grid_dim_z, uint32_t block_dim_x,
                                uint32_t block_dim_y, uint32_t block_dim_z,
                                uint32_t shared_memory_size_bytes, Chain chain,
                                RemainingArguments args) {
  auto current = wrapper::CtxSetCurrent(stream.context());
  if (!current) return current.takeError();

  // Kernel params are a vector of pointers to the kernel args, so we must first
  // materialize the kernel arg values.
  llvm::SmallVector<uintptr_t, 16> arg_values;
  arg_values.reserve(args.size());
  for (const auto& arg : args.values()) {
    if (arg->IsType<GpuBuffer>()) {
      auto pointer = arg->get<GpuBuffer>().pointer();
      arg_values.push_back(reinterpret_cast<uintptr_t>(pointer.raw()));
    } else if (arg->IsType<int32_t>()) {
      arg_values.push_back(arg->get<int32_t>());
    } else {
      return MakeStringError("Unsupported argument type");
    }
  }

  // Add required layer of indirection for kernel params.
  // TODO(idan): Consider using packed params interface.
  llvm::SmallVector<void*, 16> arg_pointers;
  arg_pointers.reserve(args.size());
  for (auto& arg_value : arg_values) arg_pointers.push_back(&arg_value);

  return wrapper::LaunchKernel(*current, function, grid_dim_x, grid_dim_y,
                               grid_dim_z, block_dim_x, block_dim_y,
                               block_dim_z, shared_memory_size_bytes,
                               stream.get(), arg_pointers, {});
}

#define TFRT_WITH_CHAIN_RESULT(sync_func) \
  internal::WithChainResult<decltype(&sync_func), &sync_func>::Invoke

void RegisterCudaKernels(KernelRegistry* kernel_reg) {
  kernel_reg->AddKernel("tfrt_gpu.init",
                        TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaInit)));
  kernel_reg->AddKernel("tfrt_gpu.device.get", TFRT_KERNEL(CudaDeviceGet));
  kernel_reg->AddKernel("tfrt_gpu.context.create",
                        TFRT_KERNEL(CudaContextCreate));

  kernel_reg->AddKernel("tfrt_gpu.stream.create",
                        TFRT_KERNEL(CudaStreamCreate));
  kernel_reg->AddKernel("tfrt_gpu.stream.synchronize",
                        TFRT_KERNEL(CudaStreamSynchronizeAsync));

  kernel_reg->AddKernel("tfrt_gpu.event.create", TFRT_KERNEL(CudaEventCreate));
  kernel_reg->AddKernel("tfrt_gpu.event.record",
                        TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaEventRecord)));
  kernel_reg->AddKernel("tfrt_gpu.event.synchronize",
                        TFRT_KERNEL(CudaEventSynchronizeAsync));

  kernel_reg->AddKernel("tfrt_gpu.allocator.create",
                        TFRT_KERNEL(CudaAllocatorCreate));

  kernel_reg->AddKernel("tfrt_gpu.mem.allocate", TFRT_KERNEL(CudaMemAllocate));
  kernel_reg->AddKernel(
      "tfrt_gpu.mem.print_metadata",
      TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaMemPrintMetadata)));

  kernel_reg->AddKernel(
      "tfrt_gpu.tensor.make.i8",
      TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaTensorMake<int8_t>)));
  kernel_reg->AddKernel(
      "tfrt_gpu.tensor.make.i32",
      TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaTensorMake<int32_t>)));
  kernel_reg->AddKernel(
      "tfrt_gpu.tensor.make.i64",
      TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaTensorMake<int64_t>)));
  kernel_reg->AddKernel(
      "tfrt_gpu.tensor.make.f32",
      TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaTensorMake<float>)));
  kernel_reg->AddKernel(
      "tfrt_gpu.tensor.make.f64",
      TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaTensorMake<double>)));

  kernel_reg->AddKernel(
      "tfrt_gpu.tensor.print_metadata",
      TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaTensorPrintMetadata)));
  kernel_reg->AddKernel("tfrt_gpu.mem.copy_host_to_device",
                        TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaMemcpyHtoD)));
  kernel_reg->AddKernel("tfrt_gpu.mem.copy_device_to_host",
                        TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaMemcpyDtoH)));

  kernel_reg->AddKernel("tfrt_gpu.function.load",
                        TFRT_KERNEL(CudaFunctionLoad));
  kernel_reg->AddKernel(
      "tfrt_gpu.function.launch",
      TFRT_KERNEL(TFRT_WITH_CHAIN_RESULT(CudaFunctionLaunch)));
}

}  // namespace gpu
}  // namespace tfrt