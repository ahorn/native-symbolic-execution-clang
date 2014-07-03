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

extern const char *NseInternalClassName;
extern const char *NseAssumeFunctionName;
extern const char *NseAssertFunctionName;
extern const char *NseSymbolicFunctionRegex;
extern const char *NseMakeSymbolicFunctionName;

extern const char *IfConditionBindId;
extern const char *IfConditionVariableBindId;
extern const char *ForConditionBindId;
extern const char *LocalVarBindId;
extern const char *GlobalVarBindId;
extern const char *FieldBindId;
extern const char *MainFunctionBindId;
extern const char *FieldBindId;
extern const char *ParmVarBindId;
extern const char *ReturnTypeBindId;

StatementMatcher makeIfConditionMatcher();
StatementMatcher makeIfConditionVariableMatcher();
StatementMatcher makeForConditionMatcher();
StatementMatcher makeWhileConditionMatcher();
StatementMatcher makeLocalVarMatcher();
DeclarationMatcher makeGlobalVarMatcher();
DeclarationMatcher makeFieldMatcher();
DeclarationMatcher makeMainFunctionMatcher();
DeclarationMatcher makeParmVarDeclMatcher();
DeclarationMatcher makeReturnTypeMatcher();
StatementMatcher makeAssumeMatcher();
StatementMatcher makeAssertMatcher();
StatementMatcher makeSymbolicMatcher();
StatementMatcher makeMakeSymbolicMatcher();

void instrumentVarDecl(
  StringRef crvClass,
  SourceRange SR,
  SourceManager &SM,
  tooling::Replacements &R);

void instrumentNamedVarDecl(
  StringRef NseClassPrefix,
  StringRef TypeName,
  StringRef VariableName,
  SourceRange SR,
  SourceManager &SM,
  const LangOptions &LO,
  tooling::Replacements &Replace);

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

class WhileConditionReplacer : public MatchFinder::MatchCallback {
public :
  WhileConditionReplacer(
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
  LocalVarReplacer(tooling::Replacements *Replace)
      : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  tooling::Replacements *Replace;
};

class GlobalVarReplacer : public MatchFinder::MatchCallback {
public :
  GlobalVarReplacer(tooling::Replacements *Replace)
      : GlobalVars(), Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

  std::vector<const VarDecl *> GlobalVars;

private:
  tooling::Replacements *Replace;
};

class FieldReplacer : public MatchFinder::MatchCallback {
public :
  FieldReplacer(tooling::Replacements *Replace)
      : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  tooling::Replacements *Replace;
};

class MainFunctionReplacer : public MatchFinder::MatchCallback {
public :
  MainFunctionReplacer(
    const std::string& NseNamespace,
    const std::string& Strategy,
    tooling::Replacements *Replace,
    std::vector<const VarDecl *> *GlobalVars,
    IncludesManager* IM)
      : NseNamespace(NseNamespace),
        Strategy(Strategy),
        Replace(Replace),
        GlobalVars(GlobalVars),
        IM(IM) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  const std::string& NseNamespace;
  const std::string& Strategy;
  tooling::Replacements *Replace;
  std::vector<const VarDecl *> *GlobalVars;
  IncludesManager* IM;
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

class AssumeReplacer : public MatchFinder::MatchCallback {
public :
  AssumeReplacer(
    const std::string& NseStrategy,
    tooling::Replacements *Replace)
      : NseStrategy(NseStrategy),
        Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  const std::string& NseStrategy;
  tooling::Replacements *Replace;
};

class AssertReplacer : public MatchFinder::MatchCallback {
public :
  AssertReplacer(
    const std::string& NseStrategy,
    tooling::Replacements *Replace)
      : NseStrategy(NseStrategy),
        Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  const std::string& NseStrategy;
  tooling::Replacements *Replace;
};

class SymbolicReplacer : public MatchFinder::MatchCallback {
public :
  SymbolicReplacer(
    const std::string& NseNamespace,
    tooling::Replacements *Replace)
      : NseNamespace(NseNamespace),
        Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  const std::string& NseNamespace;
  tooling::Replacements *Replace;
};

class MakeSymbolicReplacer : public MatchFinder::MatchCallback {
public :
  MakeSymbolicReplacer(
    const std::string& NseNamespace,
    tooling::Replacements *Replace)
      : NseNamespace(NseNamespace),
        Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result)
      override;

private:
  const std::string& NseNamespace;
  tooling::Replacements *Replace;
};

#endif
