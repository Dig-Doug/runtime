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

// RUN: tfrt_translate --bef-to-mlir %s.bef \
// RUN: | tfrt_opt -allow-unregistered-dialect \
// RUN: | FileCheck %s --dump-input=fail

// CHECK-LABEL: func private @some_func
// CHECK-SAME: tfrt.native
func private @some_func(%x: i32) -> i32 attributes {tfrt.native}

// CHECK-LABEL: func @call_native
func @call_native(%x : i32) -> i32 {

  // CHECK: "native_call"({{.*}}) {callee = @some_func}
  %r = "native_call"(%x) {callee = @some_func} : (i32) -> i32
  tfrt.return %r : i32
}
