//===-- ClangNse.cpp - Main file for the Clang CRV instrumentation tool ---===//
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

#include "NseTransform.h"

namespace cl = llvm::cl;

static cl::OptionCategory NseOptionCategory("Native symbolic execution options");

static cl::opt<std::string> NamespaceOpt(
  "namespace",
  cl::init("crv"),
  cl::desc("C++ namespace of the NSE library (default=crv)."),
  cl::cat(NseOptionCategory));

static cl::opt<std::string> BranchOpt(
  "branch",
  cl::init("branch"),
  cl::desc("Name of the function that is called on control-flow statements (default=branch)."),
  cl::cat(NseOptionCategory));

static cl::opt<std::string> StrategyOpt(
  "strategy",
  cl::init("sequential_dfs_checker"),
  cl::desc("Extern function that determines the symbolic execution path search strategy (default=sequential_dfs_checker)."),
  cl::cat(NseOptionCategory));

int main(int argc, const char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal();

  tooling::CommonOptionsParser OptionsParser(argc, argv, NseOptionCategory);
  tooling::RefactoringTool Tool(OptionsParser.getCompilations(),
    OptionsParser.getSourcePathList());

  // fully qualified function name without parenthesis
  const std::string NseStrategy = NamespaceOpt + "::" + StrategyOpt + "()";
  const std::string NseBranchStrategy = NseStrategy + "." + BranchOpt;

  IncludesManager IM;
  tooling::Replacements *Replace = &Tool.getReplacements();
  IfConditionReplacer IfStmts(NseBranchStrategy, Replace);
  IfConditionVariableReplacer IfConditionVariableStmts;
  ForConditionReplacer ForStmts(NseBranchStrategy, Replace);
  WhileConditionReplacer WhileStmts(NseBranchStrategy, Replace);
  LocalVarReplacer LocalVarDecls(Replace);
  GlobalVarReplacer GlobalVarDecls(Replace);
  FieldReplacer FieldDecls(Replace);
  MainFunctionReplacer MainFunction(NamespaceOpt, StrategyOpt,
    Replace, &GlobalVarDecls.GlobalVars, &IM);

  ParmVarReplacer ParmVarDecls(Replace);
  ReturnTypeReplacer ReturnTypes(Replace);
  AssumeReplacer Assumptions(NseStrategy, Replace);
  AssertReplacer Assertions(NseStrategy, Replace);
  SymbolicReplacer Symbolics(NamespaceOpt, Replace);
  MakeSymbolicReplacer MakeSymbolics(NamespaceOpt, Replace);
  CStyleCastReplacer CStyleCasts(NamespaceOpt, Replace);

  MatchFinder Finder;
  Finder.addMatcher(makeIfConditionMatcher(), &IfStmts);
  Finder.addMatcher(makeIfConditionVariableMatcher(), &IfConditionVariableStmts);
  Finder.addMatcher(makeForConditionMatcher(), &ForStmts);
  Finder.addMatcher(makeWhileConditionMatcher(), &WhileStmts);
  Finder.addMatcher(makeLocalVarMatcher(), &LocalVarDecls);
  Finder.addMatcher(makeGlobalVarMatcher(), &GlobalVarDecls);
  Finder.addMatcher(makeFieldMatcher(), &FieldDecls);

  // requires GlobalVarDecls to run first, so order matters
  Finder.addMatcher(makeMainFunctionMatcher(), &MainFunction);
  Finder.addMatcher(makeParmVarDeclMatcher(), &ParmVarDecls);
  Finder.addMatcher(makeReturnTypeMatcher(), &ReturnTypes);
  Finder.addMatcher(makeAssumeMatcher(), &Assumptions);
  Finder.addMatcher(makeAssertMatcher(), &Assertions);
  Finder.addMatcher(makeSymbolicMatcher(), &Symbolics);
  Finder.addMatcher(makeMakeSymbolicMatcher(), &MakeSymbolics);
  Finder.addMatcher(makeCStyleCastMatcher(), &CStyleCasts);

  return Tool.runAndSave(tooling::newFrontendActionFactory(&Finder, &IM).get());
}
