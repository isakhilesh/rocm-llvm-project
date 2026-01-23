// REQUIRES: x86-registered-target
// REQUIRES: spirv-registered-target

//=============================================================================
// Test 1: Basic compilation with spirv64
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-BASIC

// CHECK-BASIC: "-cc1" "-triple" "spirv64"
// CHECK-BASIC-SAME: "-fopenmp"
// CHECK-BASIC: "-cc1" "-triple" "x86_64
// CHECK-BASIC-SAME: "-fopenmp"

//=============================================================================
// Test 2: Vectorization must be disabled
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-NOVECTORIZE

// CHECK-NOVECTORIZE: "-cc1" "-triple" "spirv64"
// CHECK-NOVECTORIZE-SAME: "-mllvm" "-vectorize-loops=false"
// CHECK-NOVECTORIZE-SAME: "-mllvm" "-vectorize-slp=false"

//=============================================================================
// Test 3: Hidden visibility must be default
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-VISIBILITY

// CHECK-VISIBILITY: "-cc1" "-triple" "spirv64"
// CHECK-VISIBILITY-SAME: "-fvisibility=hidden"
// CHECK-VISIBILITY-SAME: "-fapply-global-visibility-to-externs"

//=============================================================================
// Test 4: Linking uses llvm-link and llvm-spirv
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-LINK

// CHECK-LINK: llvm-link
// CHECK-LINK: {{amd-llvm-spirv|llvm-spirv}}
// CHECK-LINK-SAME: "--spirv-max-version=1.6"
// CHECK-LINK-SAME: "--spirv-ext=+all"
// CHECK-LINK-SAME: "--spirv-allow-unknown-intrinsics"
// CHECK-LINK-SAME: "--spirv-lower-const-expr"
// CHECK-LINK-SAME: "--spirv-preserve-auxdata"

//=============================================================================
// Test 5: Device library is found with sysroot
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 \
// RUN:   --sysroot=%S/Inputs/spirv-openmp %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-DEVLIB

// CHECK-DEVLIB: "-mlink-builtin-bitcode" "{{.*}}libomptarget-spirv.bc"

//=============================================================================
// Test 6: -nogpulib suppresses device library
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-NOGPULIB

// CHECK-NOGPULIB-NOT: libomptarget-spirv.bc

//=============================================================================
// Test 7: ROCm path is accepted
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 --rocm-path=/opt/rocm \
// RUN:   -nogpulib %s 2>&1 | FileCheck %s --check-prefix=CHECK-ROCM

// CHECK-ROCM: "-triple" "spirv64"

//=============================================================================
// Test code
//=============================================================================

int main() {
  int a[100];
  #pragma omp target teams distribute parallel for
  for (int i = 0; i < 100; i++)
    a[i] = i;
  return a[0];
}