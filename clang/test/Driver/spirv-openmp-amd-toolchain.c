// REQUIRES: x86-registered-target
// REQUIRES: spirv-registered-target

//=============================================================================
// Test 1: Basic compilation with spirv64
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-SPIRV64

// CHECK-SPIRV64: "-cc1" "-triple" "spirv64"
// CHECK-SPIRV64-SAME: "-fopenmp"
// CHECK-SPIRV64: "-cc1" "-triple" "x86_64
// CHECK-SPIRV64-SAME: "-fopenmp"

//=============================================================================
// Test 2: Compilation with spirv64-amd-amdhsa
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64-amd-amdhsa -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-SPIRV64-AMD

// CHECK-SPIRV64-AMD: "-cc1" "-triple" "spirv64-amd-amdhsa"
// CHECK-SPIRV64-AMD-SAME: "-fopenmp"

//=============================================================================
// Test 3: Compilation with --offload-arch=amdgcnspirv
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   --offload-arch=amdgcnspirv -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-AMDGCNSPIRV

// CHECK-AMDGCNSPIRV: "-cc1" "-triple" "spirv64-amd-amdhsa"
// CHECK-AMDGCNSPIRV-SAME: "-fopenmp"

//=============================================================================
// Test 4: Vectorization must be disabled
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-NOVECTORIZE

// CHECK-NOVECTORIZE: "-cc1" "-triple" "spirv64"
// CHECK-NOVECTORIZE-SAME: "-mllvm" "-vectorize-loops=false"
// CHECK-NOVECTORIZE-SAME: "-mllvm" "-vectorize-slp=false"

//=============================================================================
// Test 5: Hidden visibility must be default
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-VISIBILITY

// CHECK-VISIBILITY: "-cc1" "-triple" "spirv64"
// CHECK-VISIBILITY-SAME: "-fvisibility=hidden"
// CHECK-VISIBILITY-SAME: "-fapply-global-visibility-to-externs"

//=============================================================================
// Test 6: Device library is found with sysroot
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 \
// RUN:   --sysroot=%S/Inputs/spirv-openmp %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-DEVLIB

// CHECK-DEVLIB: "-mlink-builtin-bitcode" "{{.*}}libomptarget-spirv.bc"

//=============================================================================
// Test 7: -nogpulib suppresses device library
//=============================================================================

// RUN: %clang -### --target=x86_64-linux-gnu -fopenmp \
// RUN:   -fopenmp-targets=spirv64 -nogpulib %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=CHECK-NOGPULIB

// CHECK-NOGPULIB-NOT: libomptarget-spirv.bc

//=============================================================================
// Test 8: ROCm path is accepted
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