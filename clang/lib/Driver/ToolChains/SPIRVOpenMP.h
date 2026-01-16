//===--- SPIRVOpenMP.h - SPIR-V OpenMP Tool Implementations ------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_SPIRV_OPENMP_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_SPIRV_OPENMP_H

#include "SPIRV.h"
#include "clang/Driver/Tool.h"
#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace tools {
namespace SPIRVOpenMP {

class LLVM_LIBRARY_VISIBILITY Linker final : public Tool {
public:
  Linker(const ToolChain &TC)
      : Tool("SPIRVOpenMP::Linker", "spirv-omp-link", TC) {}

  bool hasIntegratedCPP() const override { return false; }

  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;

private:
  void constructLinkAndEmitSpirvCommand(Compilation &C, const JobAction &JA,
                                        const InputInfoList &Inputs,
                                        const InputInfo &Output,
                                        const llvm::opt::ArgList &Args) const;
};

}
}

namespace toolchains {

class LLVM_LIBRARY_VISIBILITY SPIRVOpenMPToolChain final : public ToolChain {
public:
  SPIRVOpenMPToolChain(const Driver &D, const llvm::Triple &Triple,
                       const ToolChain &HostTC, const llvm::opt::ArgList &Args);

  const llvm::Triple *getAuxTriple() const override {
    return &HostTC.getTriple();
  }

  void addClangTargetOptions(
      const llvm::opt::ArgList &DriverArgs, llvm::opt::ArgStringList &CC1Args,
      Action::OffloadKind DeviceOffloadingKind) const override;

  void addClangWarningOptions(llvm::opt::ArgStringList &CC1Args) const override;

  CXXStdlibType GetCXXStdlibType(const llvm::opt::ArgList &Args) const override;

  void
  AddClangSystemIncludeArgs(const llvm::opt::ArgList &DriverArgs,
                            llvm::opt::ArgStringList &CC1Args) const override;

  void AddClangCXXStdlibIncludeArgs(
      const llvm::opt::ArgList &Args,
      llvm::opt::ArgStringList &CC1Args) const override;

  llvm::SmallVector<BitCodeLibraryInfo, 12>
  getDeviceLibs(const llvm::opt::ArgList &Args,
                Action::OffloadKind DeviceOffloadKind) const override;

  SanitizerMask getSupportedSanitizers() const override;

  VersionTuple
  computeMSVCVersion(const Driver *D,
                     const llvm::opt::ArgList &Args) const override;

  void adjustDebugInfoKind(llvm::codegenoptions::DebugInfoKind &DebugInfoKind,
                           const llvm::opt::ArgList &Args) const override;

  bool IsMathErrnoDefault() const override { return false; }
  bool useIntegratedAs() const override { return true; }
  bool isCrossCompiling() const override { return true; }
  bool isPICDefault() const override { return false; }
  bool isPIEDefault(const llvm::opt::ArgList &Args) const override {
    return false;
  }
  bool isPICDefaultForced() const override { return false; }
  bool SupportsProfiling() const override { return false; }

  const ToolChain &HostTC;

protected:
  Tool *buildLinker() const override;
};

}
}
}

#endif
