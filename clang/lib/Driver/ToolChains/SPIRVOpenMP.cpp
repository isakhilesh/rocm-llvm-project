//===- SPIRVOpenMP.cpp - SPIR-V OpenMP ToolChain -*- C++ -*-----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SPIRVOpenMP.h"
#include "clang/Driver/CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/InputInfo.h"
#include "clang/Options/Options.h"
#include "llvm/Config/llvm-config.h" 
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang::driver::tools;
using namespace clang;
using namespace llvm::opt;


namespace clang::driver::tools::SPIRVOpenMP {

void Linker::constructLinkAndEmitSpirvCommand(
    Compilation &C, const JobAction &JA, const InputInfoList &Inputs,
    const InputInfo &Output, const llvm::opt::ArgList &Args) const {

  assert(!Inputs.empty() && "Must have at least one input.");

  const auto &TC =
      static_cast<const toolchains::SPIRVOpenMPToolChain &>(getToolChain());
  StringRef OutputFileName = Output.getFilename();

  std::string TempBCName = C.getDriver().GetTemporaryPath(
      llvm::sys::path::stem(OutputFileName), "bc");
  const char *TempFile = C.getArgs().MakeArgString(TempBCName);

  ArgStringList LinkArgs{};

  for (const auto &Input : Inputs)
    LinkArgs.push_back(Input.getFilename());

  for (const auto &BCLib : TC.getDeviceLibs(Args, Action::OFK_OpenMP)) {
    LinkArgs.push_back(Args.MakeArgString(BCLib.Path));
  }

  for (const Arg *A : Args.filtered(options::OPT_mlink_builtin_bitcode)) {
    LinkArgs.push_back(A->getValue());
  }

  SmallString<128> LLVMLinkPath(C.getDriver().Dir);
  llvm::sys::path::append(LLVMLinkPath, "llvm-link");
  const char *LLVMLink = Args.MakeArgString(LLVMLinkPath);

  ArgStringList LLVMLinkCmdArgs;
  LLVMLinkCmdArgs.push_back("-o");
  LLVMLinkCmdArgs.push_back(TempFile);
  for (const char *Arg : LinkArgs)
    LLVMLinkCmdArgs.push_back(Arg);

  C.addCommand(std::make_unique<Command>(JA, *this, ResponseFileSupport::None(),
                                         LLVMLink, LLVMLinkCmdArgs, Inputs,
                                         InputInfo(&JA, TempFile, TempFile)));

  ArgStringList TrArgs;
  
  TrArgs.push_back("--spirv-max-version=1.6");
  TrArgs.push_back("--spirv-ext=+all");
  TrArgs.push_back("--spirv-allow-unknown-intrinsics");
  TrArgs.push_back("--spirv-lower-const-expr");
  TrArgs.push_back("--spirv-preserve-auxdata");
  TrArgs.push_back("--spirv-debug-info-version=nonsemantic-shader-200");
  
  TrArgs.push_back(TempFile);
  TrArgs.push_back("-o");
  TrArgs.push_back(Output.getFilename());

  std::string VersionedAMD = "amd-llvm-spirv-" + std::to_string(LLVM_VERSION_MAJOR);
  std::string ExePath = TC.GetProgramPath(VersionedAMD.c_str());
  
  if (!llvm::sys::fs::can_execute(ExePath))
    ExePath = TC.GetProgramPath("amd-llvm-spirv");
  if (!llvm::sys::fs::can_execute(ExePath)) {
    std::string VersionedStd = "llvm-spirv-" + std::to_string(LLVM_VERSION_MAJOR);
    ExePath = TC.GetProgramPath(VersionedStd.c_str());
  }
  if (!llvm::sys::fs::can_execute(ExePath))
    ExePath = TC.GetProgramPath("llvm-spirv");

  const char *Translator = Args.MakeArgString(ExePath);
  
  InputInfo TrInput = InputInfo(types::TY_LLVM_BC, TempFile, TempFile);
  C.addCommand(std::make_unique<Command>(JA, *this, ResponseFileSupport::None(),
                                         Translator, TrArgs, TrInput, Output));
}

void Linker::ConstructJob(Compilation &C, const JobAction &JA,
                          const InputInfo &Output, const InputInfoList &Inputs,
                          const llvm::opt::ArgList &Args,
                          const char *LinkingOutput) const {
  constructLinkAndEmitSpirvCommand(C, JA, Inputs, Output, Args);
}

}


namespace clang::driver::toolchains {

SPIRVOpenMPToolChain::SPIRVOpenMPToolChain(const Driver &D,
                                           const llvm::Triple &Triple,
                                           const ToolChain &HostToolchain,
                                           const ArgList &Args)
    : SPIRVToolChain(D, Triple, Args), HostTC(HostToolchain) {
  getProgramPaths().push_back(getDriver().Dir);
}

Tool *SPIRVOpenMPToolChain::buildLinker() const {
  return new tools::SPIRVOpenMP::Linker(*this);
}

void SPIRVOpenMPToolChain::addClangTargetOptions(
    const llvm::opt::ArgList &DriverArgs, llvm::opt::ArgStringList &CC1Args,
    Action::OffloadKind DeviceOffloadingKind) const {

  HostTC.addClangTargetOptions(DriverArgs, CC1Args, DeviceOffloadingKind);

  if (DeviceOffloadingKind != Action::OFK_OpenMP)
    return;

  CC1Args.append({"-mllvm", "-vectorize-loops=false",
                  "-mllvm", "-vectorize-slp=false"});

  if (!DriverArgs.hasArg(options::OPT_fvisibility_EQ,
                         options::OPT_fvisibility_ms_compat)) {
    CC1Args.append({"-fvisibility=hidden", "-fapply-global-visibility-to-externs"});
  }

  if (!DriverArgs.hasFlag(options::OPT_offloadlib, options::OPT_no_offloadlib,
                          true))
    return;

  addOpenMPDeviceRTL(getDriver(), DriverArgs, CC1Args, "", getTriple(), HostTC);
}

void SPIRVOpenMPToolChain::addClangWarningOptions(
    ArgStringList &CC1Args) const {
  HostTC.addClangWarningOptions(CC1Args);
}

ToolChain::CXXStdlibType
SPIRVOpenMPToolChain::GetCXXStdlibType(const ArgList &Args) const {
  return HostTC.GetCXXStdlibType(Args);
}

void SPIRVOpenMPToolChain::AddClangSystemIncludeArgs(
    const ArgList &DriverArgs, ArgStringList &CC1Args) const {
  HostTC.AddClangSystemIncludeArgs(DriverArgs, CC1Args);
}

void SPIRVOpenMPToolChain::AddClangCXXStdlibIncludeArgs(
    const ArgList &Args, ArgStringList &CC1Args) const {
  HostTC.AddClangCXXStdlibIncludeArgs(Args, CC1Args);
}

llvm::SmallVector<ToolChain::BitCodeLibraryInfo, 12>
SPIRVOpenMPToolChain::getDeviceLibs(const llvm::opt::ArgList &DriverArgs,
                                     Action::OffloadKind DeviceOffloadKind) const {
  llvm::SmallVector<BitCodeLibraryInfo, 12> BCLibs;

  if (!DriverArgs.hasFlag(options::OPT_offloadlib, options::OPT_no_offloadlib,
                          true))
    return {};

  SmallVector<std::string, 4> LibraryPaths;

  StringRef UserPath =
      DriverArgs.getLastArgValue(options::OPT_libomptarget_spirv_bc_path_EQ);
  if (!UserPath.empty()) {
    LibraryPaths.push_back(UserPath.str());
  }

  StringRef RocmPath = DriverArgs.getLastArgValue(options::OPT_rocm_path_EQ);
  if (!RocmPath.empty()) {
    SmallString<128> P(RocmPath);
    llvm::sys::path::append(P, "lib");
    LibraryPaths.push_back(std::string(P));
  }

  if (!getDriver().SysRoot.empty()) {
    SmallString<128> P(getDriver().SysRoot);
    llvm::sys::path::append(P, "lib");
    LibraryPaths.push_back(std::string(P));
  }

  SmallString<128> ResourceLibPath(getDriver().ResourceDir);
  llvm::sys::path::append(ResourceLibPath, "lib");
  LibraryPaths.push_back(std::string(ResourceLibPath));

  SmallString<128> DriverLibPath(getDriver().Dir);
  llvm::sys::path::append(DriverLibPath, "..", "lib");
  LibraryPaths.push_back(std::string(DriverLibPath));

  std::string BCName = "libomptarget-spirv.bc";
  for (const auto &Path : LibraryPaths) {
    SmallString<128> FullPath(Path);
    llvm::sys::path::append(FullPath, BCName);
    if (llvm::sys::fs::exists(FullPath)) {
      BCLibs.emplace_back(std::string(FullPath));
      return BCLibs;
    }
  }

  if (!DriverArgs.hasArg(options::OPT_no_offloadlib)) {
    getDriver().Diag(diag::err_drv_omp_offload_target_missingbcruntime)
        << BCName << getTriple().str();
  }

  return BCLibs;
}

SanitizerMask SPIRVOpenMPToolChain::getSupportedSanitizers() const {
  return HostTC.getSupportedSanitizers();
}

VersionTuple
SPIRVOpenMPToolChain::computeMSVCVersion(const Driver *D,
                                          const ArgList &Args) const {
  return HostTC.computeMSVCVersion(D, Args);
}

void SPIRVOpenMPToolChain::adjustDebugInfoKind(
    llvm::codegenoptions::DebugInfoKind &DebugInfoKind,
    const llvm::opt::ArgList &Args) const {
  DebugInfoKind = llvm::codegenoptions::NoDebugInfo;
}

}
