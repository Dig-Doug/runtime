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

// RUN: tfrt_translate -bef-to-mlir %s.bef | tfrt_opt -allow-unregistered-dialect | FileCheck %s --dump-input=fail

// CHECK-LABEL: func @sync_func_foo
// CHECK-SAME: tfrt.sync
func @sync_func_foo(%x: i32, %y: i32) -> i32 attributes {tfrt.sync} {
  %z = "foo_kernel"(%x) : (i32) -> i32
  tfrt.return %z : i32
}
