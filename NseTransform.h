#define DEBUG_TYPE "NseTransform"

#include "llvm/Support/Debug.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Refactoring.h"
#include "IncludeDirectives.h"

#include <string>
#include <vector>

#ifndef CLANG_CRV_TRANSFORM_H
#define CLANG_CRV_TRANSFORM_H

using namespace clang;
using namespace clang::ast_matchers;

extern const char *CrvInternalClassName;

extern const char *IfConditionBindId;
extern const char *IfConditionVariableBindId;
extern const char *ForConditionBindId;
extern const char *LocalVarBindId;
extern const char *GlobalVarBindId;
extern const char *MainFunctionBindId;
extern const char *FieldBindId;
extern const char *ParmVarBindId;
extern const char *ReturnTypeBindId;
extern const char *IncrementBindId;

StatementMatcher makeIfConditionMatcher();
StatementMatcher makeIfConditionVariableMatcher();
StatementMatcher makeForConditionMatcher();
StatementMatcher makeLocalVarMatcher();
DeclarationMatcher makeGlobalVarMatcher();
DeclarationMatcher makeMainFunctionMatcher();
DeclarationMatcher makeParmVarDeclMatcher();
DeclarationMatcher makeReturnTypeMatcher();
StatementMatcher makeIncrementMatcher();

void instrumentVarDecl(
  StringRef crvClass,
  SourceRange SR,
  SourceManager &SM,
  tooling::Replacements &R);

struct IncludesManager : public tooling::SourceFileCallbacks {
  IncludesManager()
      : Includes(0) {}

  ~IncludesManager() {
    delete Includes;
  }

  IncludeDirectives* Includes;

  virtual bool handleBeginSource(CompilerInstance &CI, StringRef Filename)
      override;
};

class IfConditionReplacer : public MatchFinder::MatchCallback {
public :
  IfConditionReplacer(
    const std::string& NseBranchStrategy,
    tooling::Replacements *Replace)
      : NseBranchStrategy(NseBranchStrategy),
        Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  const std::string& NseBranchStrategy;
  tooling::Replacements *Replace;
};

class IfConditionVariableReplacer : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result)
      override;
};

class ForConditionReplacer : public MatchFinder::MatchCallback {
public :
  ForConditionReplacer(
    const std::string& NseBranchStrategy,
    tooling::Replacements *Replace)
      : NseBranchStrategy(NseBranchStrategy),
        Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  const std::string& NseBranchStrategy;
  tooling::Replacements *Replace;
};

class LocalVarReplacer : public MatchFinder::MatchCallback {
public :
  LocalVarReplacer(tooling::Replacements *Replace, IncludesManager* IM)
      : Replace(Replace), IM(IM) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  tooling::Replacements *Replace;
  IncludesManager* IM;
};

class GlobalVarReplacer : public MatchFinder::MatchCallback {
public :
  GlobalVarReplacer(tooling::Replacements *Replace)
      : GlobalVars(), Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

  std::vector<std::string> GlobalVars;

private:
  tooling::Replacements *Replace;
};

class MainFunctionReplacer : public MatchFinder::MatchCallback {
public :
  MainFunctionReplacer(
    const std::string& NseNamespace,
    const std::string& Strategy,
    tooling::Replacements *Replace,
    std::vector<std::string> *GlobalVars)
      : NseNamespace(NseNamespace),
        Strategy(Strategy),
        Replace(Replace),
        GlobalVars(GlobalVars) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  const std::string& NseNamespace;
  const std::string& Strategy;
  tooling::Replacements *Replace;
  std::vector<std::string> *GlobalVars;
};

class ParmVarReplacer : public MatchFinder::MatchCallback {
public :
  ParmVarReplacer(tooling::Replacements *Replace)
      : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  tooling::Replacements *Replace;
};

class ReturnTypeReplacer : public MatchFinder::MatchCallback {
public :
  ReturnTypeReplacer(tooling::Replacements *Replace)
      : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  tooling::Replacements *Replace;
};

class IncrementReplacer : public MatchFinder::MatchCallback {
public :
  IncrementReplacer(tooling::Replacements *Replace)
      : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  tooling::Replacements *Replace;
};

#endif
