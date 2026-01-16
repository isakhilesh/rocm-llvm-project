//===- SPIRVOpenMP.cpp - SPIR-V OpenMP Tool Implementations -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SPIRVOpenMP.h"
#include "CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/InputInfo.h"
#include "clang/Options/Options.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang::driver::tools;
using namespace llvm::opt;

namespace clang::driver::tools::SPIRVOpenMP {

void Linker::constructLinkAndEmitSpirvCommand(
    Compilation &C, const JobAction &JA, const InputInfoList &Inputs,
    const InputInfo &Output, const llvm::opt::ArgList &Args) const {

  assert(!Inputs.empty() && "Must have at least one input.");

  const auto &TC =
      static_cast<const toolchains::SPIRVOpenMPToolChain &>(getToolChain());
  StringRef OutputFileName = Output.getFilename();
  std::string TempBCName =
      C.getDriver().GetTemporaryPath(llvm::sys::path::stem(OutputFileName), "bc");
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

  TrArgs.push_back("--spirv-max-version=1.4");

  TrArgs.push_back("--spirv-ext=+all");

  InputInfo TrInput = InputInfo(types::TY_LLVM_BC, TempFile, TempFile);
  SPIRV::constructTranslateCommand(C, *this, JA, Output, TrInput, TrArgs);
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
    : SPIRVToolChain(D, Triple, Args), HostTC(HostToolchain) {}

void SPIRVOpenMPToolChain::addClangTargetOptions(
    const llvm::opt::ArgList &DriverArgs, llvm::opt::ArgStringList &CC1Args,
    Action::OffloadKind DeviceOffloadingKind) const {

  if (DeviceOffloadingKind != Action::OFK_OpenMP)
    return;

  if (!DriverArgs.hasFlag(options::OPT_offloadlib, options::OPT_no_offloadlib,
                          true))
    return;
  addOpenMPDeviceRTL(getDriver(), DriverArgs, CC1Args, "", getTriple(), HostTC);
}

} // namespace clang::driver::toolchains
