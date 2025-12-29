; RUN: not --crash llc -filetype=null -global-isel=0 -mtriple=amdgcn -mcpu=gfx1250 %s 2>&1 | FileCheck --ignore-case %s
; RUN: not         llc -filetype=null -global-isel=1 -mtriple=amdgcn -mcpu=gfx1250 %s 2>&1 | FileCheck --ignore-case %s
;
; CHECK: LLVM ERROR: Cannot select

define amdgpu_ps void @async_err() {
  call void @llvm.amdgcn.asyncmark()
  call void @llvm.amdgcn.wait.asyncmark(i16 0)
  ret void
}
