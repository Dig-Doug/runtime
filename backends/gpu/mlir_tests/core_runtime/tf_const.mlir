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

// RUN: bef_executor --test_init_function=register_op_handlers_gpu %s.bef | FileCheck %s --dump-input=fail

func @register_op_handlers_gpu() {
  %null = "corert.create_null_op_handler"() : () -> !corert.ophandler
  %gpu_ordinal = tfrt.constant.i32 0
  %gpu = "corert.create_gpu_op_handler" (%gpu_ordinal, %null) : (i32, !corert.ophandler) -> !corert.ophandler
  corert.register_op_handler %gpu "gpu"
  tfrt.return
}

// CHECK: --- Running 'const_f32'
func @const_f32() -> !tfrt.chain {
  %ch_epoch = tfrt.new.chain
  %gpu = corert.get_op_handler %ch_epoch "gpu"

  %gpu_handle_result = corert.executeop(%gpu)
    "tf.Const"() {value = dense<[-1.0, -0.5, 0.0, 0.5, 1.0]> : tensor<5xf32>, dtype = f32} : 1

  %cpu_handle_result = corert.executeop(%gpu) "tfrt_test.gpu_tensor_to_host_tensor"(%gpu_handle_result) : 1
  // CHECK: DenseHostTensor dtype = f32, shape = [5], values = [-1, -0.5, 0, 0.5, 1]
  %ch_print_cpu = corert.executeop.seq(%gpu, %ch_epoch) "tfrt_test.print"(%cpu_handle_result) : 0
  tfrt.return %ch_print_cpu : !tfrt.chain
}
