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

// Thin wrapper around the NCCL API adding llvm::Error.
#include "tfrt/gpu/wrapper/nccl_wrapper.h"

#include <utility>

#include "wrapper_detail.h"

namespace tfrt {
namespace gpu {
namespace wrapper {

llvm::Expected<int> NcclGetVersion() {
  int version = 0;
  RETURN_IF_ERROR(ncclGetVersion(&version));
  return version;
}

llvm::Expected<ncclUniqueId> NcclGetUniqueId() {
  ncclUniqueId id;
  RETURN_IF_ERROR(ncclGetUniqueId(&id));
  return id;
}

llvm::Expected<OwningCclComm> NcclCommInitRank(CurrentContext current,
                                               int nranks, ncclUniqueId commId,
                                               int rank) {
  CheckCudaContext(current);
  ncclComm_t comm;
  RETURN_IF_ERROR(ncclCommInitRank(&comm, nranks, commId, rank));
  return OwningCclComm({comm, Platform::CUDA});
}

llvm::Error NcclCommDestroy(ncclComm_t comm) {
  return TO_ERROR(ncclCommDestroy(comm));
}

llvm::Error NcclCommAbort(ncclComm_t comm) {
  return TO_ERROR(ncclCommAbort(comm));
}

llvm::Error NcclCommGetAsyncError(ncclComm_t comm) {
  ncclResult_t result;
  RETURN_IF_ERROR(ncclCommGetAsyncError(comm, &result));
  return TO_ERROR(result);
}

llvm::Expected<int> NcclCommCount(ncclComm_t comm) {
  int count = 0;
  RETURN_IF_ERROR(ncclCommCount(comm, &count));
  return count;
}

llvm::Expected<int> NcclCommUserRank(ncclComm_t comm) {
  int rank = 0;
  RETURN_IF_ERROR(ncclCommUserRank(comm, &rank));
  return rank;
}

llvm::Error NcclReduce(CurrentContext current, Pointer<const void> sendbuff,
                       Pointer<void> recvbuff, size_t count,
                       ncclDataType_t datatype, ncclRedOp_t op, int root,
                       ncclComm_t comm, cudaStream_t stream) {
  CheckCudaContext(current);
  return TO_ERROR(ncclReduce(sendbuff.raw(Platform::CUDA),
                             recvbuff.raw(Platform::CUDA), count, datatype, op,
                             root, comm, stream));
}

llvm::Error NcclBcast(CurrentContext current, Pointer<void> buffer,
                      size_t count, ncclDataType_t datatype, int root,
                      ncclComm_t comm, cudaStream_t stream) {
  CheckCudaContext(current);
  return TO_ERROR(ncclBcast(buffer.raw(Platform::CUDA), count, datatype, root,
                            comm, stream));
}

llvm::Error NcclBroadcast(CurrentContext current, Pointer<const void> sendbuff,
                          Pointer<void> recvbuff, size_t count,
                          ncclDataType_t datatype, int root, ncclComm_t comm,
                          cudaStream_t stream) {
  CheckCudaContext(current);
  return TO_ERROR(ncclBroadcast(sendbuff.raw(Platform::CUDA),
                                recvbuff.raw(Platform::CUDA), count, datatype,
                                root, comm, stream));
}

llvm::Error NcclAllReduce(CurrentContext current, Pointer<const void> sendbuff,
                          Pointer<void> recvbuff, size_t count,
                          ncclDataType_t datatype, ncclRedOp_t op,
                          ncclComm_t comm, cudaStream_t stream) {
  CheckCudaContext(current);
  return TO_ERROR(ncclAllReduce(sendbuff.raw(Platform::CUDA),
                                recvbuff.raw(Platform::CUDA), count, datatype,
                                op, comm, stream));
}

llvm::Error NcclReduceScatter(CurrentContext current,
                              Pointer<const void> sendbuff,
                              Pointer<void> recvbuff, size_t recvcount,
                              ncclDataType_t datatype, ncclRedOp_t op,
                              ncclComm_t comm, cudaStream_t stream) {
  CheckCudaContext(current);
  return TO_ERROR(ncclReduceScatter(sendbuff.raw(Platform::CUDA),
                                    recvbuff.raw(Platform::CUDA), recvcount,
                                    datatype, op, comm, stream));
}

llvm::Error NcclAllGather(CurrentContext current, Pointer<const void> sendbuff,
                          Pointer<void> recvbuff, size_t sendcount,
                          ncclDataType_t datatype, ncclComm_t comm,
                          cudaStream_t stream) {
  CheckCudaContext(current);
  return TO_ERROR(ncclAllGather(sendbuff.raw(Platform::CUDA),
                                recvbuff.raw(Platform::CUDA), sendcount,
                                datatype, comm, stream));
}

llvm::Error NcclSend(CurrentContext current, Pointer<const void> sendbuff,
                     size_t count, ncclDataType_t datatype, int peer,
                     ncclComm_t comm, cudaStream_t stream) {
  CheckCudaContext(current);
  return TO_ERROR(ncclSend(sendbuff.raw(Platform::CUDA), count, datatype, peer,
                           comm, stream));
}

llvm::Error NcclRecv(CurrentContext current, Pointer<void> recvbuff,
                     size_t count, ncclDataType_t datatype, int peer,
                     ncclComm_t comm, cudaStream_t stream) {
  CheckCudaContext(current);
  return TO_ERROR(ncclRecv(recvbuff.raw(Platform::CUDA), count, datatype, peer,
                           comm, stream));
}

llvm::Error NcclGroupStart() { return TO_ERROR(ncclGroupStart()); }

llvm::Error NcclGroupEnd() { return TO_ERROR(ncclGroupEnd()); }

}  // namespace wrapper
}  // namespace gpu
}  // namespace tfrt
