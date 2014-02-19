//===-- ClangCrv.cpp - Main file for the Clang CRV instrumentation tool ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Instruments C++11 code to symbolically execute it with CRV
///
/// See user documentation for usage instructions.
///
//===----------------------------------------------------------------------===//

#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Signals.h"

#include "CrvTransform.h"

namespace cl = llvm::cl;

static cl::OptionCategory CrvOptionCategory("crv options");

int main(int argc, const char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal();

  tooling::CommonOptionsParser OptionsParser(argc, argv, CrvOptionCategory);
  tooling::RefactoringTool Tool(OptionsParser.getCompilations(),
    OptionsParser.getSourcePathList());

  IncludesManager IM;
  tooling::Replacements *Replace = &Tool.getReplacements();
  IfConditionReplacer IfStmts(Replace);
  IfConditionVariableReplacer IfConditionVariableStmts;
  ForConditionReplacer ForStmts(Replace);
  LocalVarReplacer LocalVarDecls(Replace, &IM);
  GlobalVarReplacer GlobalVarDecls(Replace);
  ParmVarReplacer ParmVarDecls(Replace);
  ReturnTypeReplacer ReturnTypes(Replace);
  IncrementReplacer Increments(Replace);

  MatchFinder Finder;
  Finder.addMatcher(makeIfConditionMatcher(), &IfStmts);
  Finder.addMatcher(makeIfConditionVariableMatcher(), &IfConditionVariableStmts);
  Finder.addMatcher(makeForConditionMatcher(), &ForStmts);
  Finder.addMatcher(makeLocalVarMatcher(), &LocalVarDecls);
  Finder.addMatcher(makeGlobalVarMatcher(), &GlobalVarDecls);
  Finder.addMatcher(makeParmVarDeclMatcher(), &ParmVarDecls);
  Finder.addMatcher(makeReturnTypeMatcher(), &ReturnTypes);
  Finder.addMatcher(makeIncrementMatcher(), &Increments);

  return Tool.runAndSave(tooling::newFrontendActionFactory(&Finder, &IM));
}
