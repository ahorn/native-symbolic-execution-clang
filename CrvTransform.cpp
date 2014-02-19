#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "CrvTransform.h"

const char *CrvInternalClassName = "crv::Internal<";
const char *CrvExternalClassName = "crv::External<";

const char *IfConditionBindId = "if_condition";
const char *IfConditionVariableBindId = "if_condition_variable";
const char *ForConditionBindId = "for_condition";
const char *LocalVarBindId = "internal_decl";
const char *GlobalVarBindId = "external_decl";
const char *ParmVarBindId = "parm_var_decl";
const char *ReturnTypeBindId = "return_type";
const char *IncrementBindId = "post_increment";

bool IncludesManager::handleBeginSource(CompilerInstance &CI,
  StringRef Filename) {

  Includes = new IncludeDirectives(CI);
  return true;
}

StatementMatcher makeIfConditionMatcher() {
  return ifStmt(
    hasCondition(
      expr().bind(IfConditionBindId)),
    unless(hasConditionVariableStatement(declStmt())));
}

StatementMatcher makeIfConditionVariableMatcher() {
  return ifStmt(
    hasConditionVariableStatement(
      declStmt().bind(IfConditionVariableBindId)));
}

StatementMatcher makeForConditionMatcher() {
  return forStmt(
    hasCondition(
      expr().bind(ForConditionBindId)));
}

StatementMatcher makeLocalVarMatcher() {
  return declStmt(
    has(
      varDecl(
        hasType(isInteger())))).bind(LocalVarBindId);
}

DeclarationMatcher makeGlobalVarMatcher() {
  return varDecl(
    hasType(
      isInteger())).bind(GlobalVarBindId);
}

DeclarationMatcher makeParmVarDeclMatcher() {
  return parmVarDecl(
    hasType(
      isInteger())).bind(ParmVarBindId);
}

DeclarationMatcher makeReturnTypeMatcher() {
  return functionDecl(
    returns(
      qualType(
        isInteger()))).bind(ReturnTypeBindId);
}

StatementMatcher makeIncrementMatcher() {
  return unaryOperator(
    hasOperatorName("++")).bind(IncrementBindId);
}

void instrumentControlFlow(
  SourceRange SR,
  SourceManager &SM,
  tooling::Replacements &R) {

  CharSourceRange Range = Lexer::makeFileCharRange(
      CharSourceRange::getTokenRange(SR), SM, LangOptions());

  R.insert(tooling::Replacement(SM, Range.getBegin(), 0, "crv::tracer().decide_flip("));
  R.insert(tooling::Replacement(SM, Range.getEnd(), 0, ")"));
}

void IfConditionReplacer::run(const MatchFinder::MatchResult &Result) {
  const Expr *E = Result.Nodes.getNodeAs<Expr>(IfConditionBindId);
  assert(E && "Bad Callback. No node provided");

  SourceManager &SM = *Result.SourceManager;
  SourceRange Range = E->getSourceRange();
  instrumentControlFlow(Range, SM, *Replace);
}

void IfConditionVariableReplacer::run(const MatchFinder::MatchResult &Result) {
  assert(0 && "Condition variables are currently not supported");
}

void ForConditionReplacer::run(const MatchFinder::MatchResult &Result) {
  const Expr *E = Result.Nodes.getNodeAs<Expr>(ForConditionBindId);
  assert(E && "Bad Callback. No node provided");

  SourceManager &SM = *Result.SourceManager;
  instrumentControlFlow(E->getSourceRange(), SM, *Replace);
}

// TODO: Fix buffer corruption issue, perhaps use clang-apply-replacements?
void addCrvHeader(
  const FileEntry *File,
  tooling::Replacements &Replace,
  IncludeDirectives &Includes) {

  const tooling::Replacement &IncludeReplace = Includes.addAngledInclude(File, "crv.h");
  if (IncludeReplace.isApplicable()) {
    Replace.insert(IncludeReplace);
  }
}

void instrumentVarDecl(
  StringRef crvClass,
  SourceRange SR,
  SourceManager &SM,
  tooling::Replacements &Replace) {

  CharSourceRange Range = Lexer::makeFileCharRange(
      CharSourceRange::getTokenRange(SR), SM, LangOptions());

  Replace.insert(tooling::Replacement(SM, Range.getBegin(), 0, crvClass));
  Replace.insert(tooling::Replacement(SM, Range.getEnd(), 0, ">"));
}

void LocalVarReplacer::run(const MatchFinder::MatchResult &Result) {
  const DeclStmt *D = Result.Nodes.getNodeAs<DeclStmt>(LocalVarBindId);
  assert(D && "Bad Callback. No node provided");

  const VarDecl *V = cast<VarDecl>(*D->decl_begin());
  if (!V->hasLocalStorage())
    return;

  TypeLoc TL = V->getTypeSourceInfo()->getTypeLoc();
  SourceManager &SM = *Result.SourceManager;
  instrumentVarDecl(CrvInternalClassName, TL.getSourceRange(), SM, *Replace);

  //const FileEntry *File = SM.getFileEntryForID(SM.getFileID(TL.getBeginLoc()));
  //addCrvHeader(File, *Replace, *IM->Includes);
}

void GlobalVarReplacer::run(const MatchFinder::MatchResult &Result) {
  const VarDecl *V = Result.Nodes.getNodeAs<VarDecl>(GlobalVarBindId);
  assert(V && "Bad Callback. No node provided");
  if (!V->hasGlobalStorage())
    return;

  TypeLoc TL = V->getTypeSourceInfo()->getTypeLoc();
  SourceManager &SM = *Result.SourceManager;
  instrumentVarDecl(CrvExternalClassName, TL.getSourceRange(), SM, *Replace);
}

// Replaces currently only integral parameters passed by value
void ParmVarReplacer::run(const MatchFinder::MatchResult &Result) {
  const ParmVarDecl *D = Result.Nodes.getNodeAs<ParmVarDecl>(ParmVarBindId);
  assert(D && "Bad Callback. No node provided");

  TypeLoc TL = D->getTypeSourceInfo()->getTypeLoc();
  SourceManager &SM = *Result.SourceManager;
  instrumentVarDecl(CrvInternalClassName, TL.getSourceRange(), SM, *Replace);
}

void ReturnTypeReplacer::run(const MatchFinder::MatchResult &Result) {
  const FunctionDecl *D = Result.Nodes.getNodeAs<FunctionDecl>(ReturnTypeBindId);
  assert(D && "Bad Callback. No node provided");

  if (D->getName() == "main")
    return;

  TypeLoc TL = D->getTypeSourceInfo()->getTypeLoc().
    IgnoreParens().castAs<FunctionProtoTypeLoc>().getReturnLoc();

  SourceManager &SM = *Result.SourceManager;
  instrumentVarDecl(CrvInternalClassName, TL.getSourceRange(), SM, *Replace);
}

void IncrementReplacer::run(const MatchFinder::MatchResult &Result) {
  const UnaryOperator *O = Result.Nodes.getNodeAs<UnaryOperator>(IncrementBindId);
  assert(O && "Bad Callback. No node provided");

  if (!O->isPostfix())
    return;

  SourceManager &SM = *Result.SourceManager;
  SourceLocation LocBegin = O->getSubExpr()->getLocStart();
  SourceLocation LocEnd = O->getOperatorLoc();

  Replace->insert(tooling::Replacement(SM, LocBegin, 0, "crv::post_increment("));
  Replace->insert(tooling::Replacement(SM, LocEnd, 2, ")"));
}
