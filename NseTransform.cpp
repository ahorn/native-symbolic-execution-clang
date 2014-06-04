#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/ASTContext.h"
#include "NseTransform.h"

const char *NseInternalClassName = "crv::Internal<";
const char *NseAssumeFunctionName = "nse_assume";

const char *IfConditionBindId = "if_condition";
const char *IfConditionVariableBindId = "if_condition_variable";
const char *ForConditionBindId = "for_condition";
const char *LocalVarBindId = "internal_decl";
const char *GlobalVarBindId = "external_decl";
const char *MainFunctionBindId = "main_function";
const char *ParmVarBindId = "parm_var_decl";
const char *ReturnTypeBindId = "return_type";
const char *IncrementBindId = "post_increment";
const char *AssumeBindId = "assume";

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

DeclarationMatcher makeMainFunctionMatcher() {
  return functionDecl(hasName("main")).bind(MainFunctionBindId);
}

DeclarationMatcher makeParmVarDeclMatcher() {
  return parmVarDecl().bind(ParmVarBindId);
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

StatementMatcher makeAssumeMatcher() {
  return callExpr(callee(functionDecl(
    hasName(NseAssumeFunctionName)))).bind(AssumeBindId);
}

void instrumentControlFlow(
  const std::string& NseBranchStrategy,
  SourceRange SR,
  SourceManager &SM,
  const LangOptions &LO,
  tooling::Replacements &R) {

  CharSourceRange Range = Lexer::makeFileCharRange(
      CharSourceRange::getTokenRange(SR), SM, LO);

  R.insert(tooling::Replacement(SM, Range.getBegin(), 0, NseBranchStrategy + "("));
  R.insert(tooling::Replacement(SM, Range.getEnd(), 0, ")"));
}

void IfConditionReplacer::run(const MatchFinder::MatchResult &Result) {
  const Expr *E = Result.Nodes.getNodeAs<Expr>(IfConditionBindId);
  assert(E && "Bad Callback. No node provided");

  SourceLocation Loc = E->getExprLoc();
  SourceManager &SM = *Result.SourceManager;
  if (!Result.Context->getSourceManager().isWrittenInMainFile(Loc))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(Loc) << '\n');
    return;
  }

  SourceRange Range = E->getSourceRange();
  instrumentControlFlow(NseBranchStrategy, Range, SM,
    Result.Context->getLangOpts(), *Replace);
}

void IfConditionVariableReplacer::run(const MatchFinder::MatchResult &Result) {
  const Expr *E = Result.Nodes.getNodeAs<Expr>(IfConditionVariableBindId);

  SourceLocation Loc = E->getExprLoc();
  SourceManager &SM = *Result.SourceManager;
  if (!Result.Context->getSourceManager().isWrittenInMainFile(Loc))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(Loc) << '\n');
    return;
  }

  assert(0 && "Condition variables are currently not supported");
}

void ForConditionReplacer::run(const MatchFinder::MatchResult &Result) {
  const Expr *E = Result.Nodes.getNodeAs<Expr>(ForConditionBindId);
  assert(E && "Bad Callback. No node provided");

  SourceLocation Loc = E->getExprLoc();
  SourceManager &SM = *Result.SourceManager;
  if (!Result.Context->getSourceManager().isWrittenInMainFile(Loc))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(Loc) << '\n');
    return;
  }

  instrumentControlFlow(NseBranchStrategy, E->getSourceRange(), SM,
    Result.Context->getLangOpts(), *Replace);
}

// TODO: Fix buffer corruption issue, perhaps use clang-apply-replacements?
void addNseHeader(
  const FileEntry *File,
  tooling::Replacements &Replace,
  IncludeDirectives &Includes) {

  const tooling::Replacement &IncludeReplace = Includes.addAngledInclude(File, "crv.h");
  if (IncludeReplace.isApplicable()) {
    Replace.insert(IncludeReplace);
  }
}

void instrumentVarDecl(
  StringRef NseClassPrefix,
  SourceRange SR,
  SourceManager &SM,
  const LangOptions &LO,
  tooling::Replacements &Replace) {

  const std::string NseClassSuffix = ">";
  CharSourceRange Range = Lexer::makeFileCharRange(
      CharSourceRange::getTokenRange(SR), SM, LO);

  Replace.insert(tooling::Replacement(SM, Range.getBegin(), 0, NseClassPrefix));
  Replace.insert(tooling::Replacement(SM, Range.getEnd(), 0, NseClassSuffix));
}

bool isSupportedType(QualType QT) {
  QualType CanonicalType = QT.getTypePtr()->getCanonicalTypeInternal();
  if (CanonicalType->isFundamentalType())
    return true;

  if (const PointerType *PtrType = dyn_cast<PointerType>(CanonicalType.getTypePtr())) {
    QualType PointeeType = PtrType->getPointeeType();
    return PointeeType->isFundamentalType();
  }

  return false;
}

void LocalVarReplacer::run(const MatchFinder::MatchResult &Result) {
  const DeclStmt *D = Result.Nodes.getNodeAs<DeclStmt>(LocalVarBindId);
  assert(D && "Bad Callback. No node provided");

  const VarDecl *V = cast<VarDecl>(*D->decl_begin());
  if (!V->hasLocalStorage() || !isSupportedType(V->getType()))
    return;

  SourceLocation Loc = V->getLocation();
  SourceManager &SM = *Result.SourceManager;
  if (!Result.Context->getSourceManager().isWrittenInMainFile(Loc))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(Loc) << '\n');
    return;
  }

  TypeLoc TL = V->getTypeSourceInfo()->getTypeLoc();
  instrumentVarDecl(NseInternalClassName, TL.getSourceRange(), SM,
    Result.Context->getLangOpts(), *Replace);

  //const FileEntry *File = SM.getFileEntryForID(SM.getFileID(TL.getBeginLoc()));
  //addNseHeader(File, *Replace, *IM->Includes);
}

void GlobalVarReplacer::run(const MatchFinder::MatchResult &Result) {
  const VarDecl *V = Result.Nodes.getNodeAs<VarDecl>(GlobalVarBindId);
  assert(V && "Bad Callback. No node provided");
  if (!V->hasGlobalStorage() || !isSupportedType(V->getType()))
    return;

  SourceLocation Loc = V->getLocation();
  SourceManager &SM = *Result.SourceManager;
  if (!Result.Context->getSourceManager().isWrittenInMainFile(Loc))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(Loc) << '\n');
    return;
  }

  GlobalVars.push_back(V->getName().str());

  TypeLoc TL = V->getTypeSourceInfo()->getTypeLoc();
  instrumentVarDecl(NseInternalClassName, TL.getSourceRange(), SM,
    Result.Context->getLangOpts(), *Replace);
}

void MainFunctionReplacer::run(const MatchFinder::MatchResult &Result) {
  const FunctionDecl *D = Result.Nodes.getNodeAs<FunctionDecl>(MainFunctionBindId);
  assert(D && "Bad Callback. No node provided");
  assert(GlobalVars && "GlobalVars is NULL");

  SourceManager &SM = *Result.SourceManager;
  SourceLocation Loc = D->getLocation();
  if (!Result.Context->getSourceManager().isWrittenInMainFile(Loc))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(Loc) << '\n');
    return;
  }

  SourceLocation NameLocBegin = D->getNameInfo().getBeginLoc();
  Replace->insert(tooling::Replacement(SM, NameLocBegin, 4, "nse_main"));

  if (!D->hasBody())
    return;

  DEBUG(llvm::errs() << "MainFunctionReplacer: " << GlobalVars->size()
                     << " global variables" << "\n");

  const std::string NseMakeAny = "  " + NseNamespace + "::make_any(";
  Stmt *FuncBody = D->getBody();
  SourceLocation BodyLocBegin = FuncBody->getLocStart().getLocWithOffset(1);
  std::string MakeAnys = "\n";
  for (const std::string& GlobalVar : *GlobalVars)
  {
    MakeAnys += NseMakeAny + GlobalVar + ");\n";
  }
  Replace->insert(tooling::Replacement(SM, BodyLocBegin, 0, MakeAnys));

  const std::string NseStrategy = NseNamespace + "::" + Strategy + "()";
  SourceLocation BodyEndLoc = FuncBody->getLocEnd().getLocWithOffset(1);
  Replace->insert(tooling::Replacement(SM, BodyEndLoc, 0,
    "\n\n"
    "int main() {\n"
    "  bool error = false;\n"
    "  std::chrono::seconds seconds(std::chrono::seconds::zero());\n"
    "  {\n"
    "    smt::NonReentrantTimer<std::chrono::seconds> timer(seconds);\n"
    "\n"
    "    do {\n"
    "      nse_main();\n"
    "      error |= smt::sat == " + NseStrategy + ".check();\n"
    "    } while (" + NseStrategy + ".find_next_path() && !error);\n"
    "  }\n"
    "\n"
    "  if (error)\n"
    "    std::cout << \"Found bug!\" << std::endl;\n"
    "  else\n"
    "    std::cout << \"Could not find any bugs.\" << std::endl;\n"
    "\n"
    "  report_statistics(" + NseStrategy + ".solver().stats(), " + NseStrategy + ".stats(), seconds);\n"
    "\n"
    "  return error;\n"
    "}"));
}

// Passes integral parameters passed by value and other types by reference
void ParmVarReplacer::run(const MatchFinder::MatchResult &Result) {
  const ParmVarDecl *D = Result.Nodes.getNodeAs<ParmVarDecl>(ParmVarBindId);
  assert(D && "Bad Callback. No node provided");

  if (!isSupportedType(D->getType()))
    return;

  SourceLocation Loc = D->getLocation();
  SourceManager &SM = *Result.SourceManager;
  if (!Result.Context->getSourceManager().isWrittenInMainFile(Loc))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(Loc) << '\n');
    return;
  }

  TypeLoc TL = D->getTypeSourceInfo()->getTypeLoc();
  instrumentVarDecl(NseInternalClassName, TL.getSourceRange(), SM,
    Result.Context->getLangOpts(), *Replace);
}

void ReturnTypeReplacer::run(const MatchFinder::MatchResult &Result) {
  const FunctionDecl *D = Result.Nodes.getNodeAs<FunctionDecl>(ReturnTypeBindId);
  assert(D && "Bad Callback. No node provided");

  if (D->getName() == "main" || !isSupportedType(D->getReturnType()))
    return;

  SourceLocation Loc = D->getLocation();
  SourceManager &SM = *Result.SourceManager;
  if (!Result.Context->getSourceManager().isWrittenInMainFile(Loc))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(Loc) << '\n');
    return;
  }

  TypeLoc TL = D->getTypeSourceInfo()->getTypeLoc().
    IgnoreParens().castAs<FunctionProtoTypeLoc>().getReturnLoc();

  instrumentVarDecl(NseInternalClassName, TL.getSourceRange(), SM,
    Result.Context->getLangOpts(), *Replace);
}

void IncrementReplacer::run(const MatchFinder::MatchResult &Result) {
  const UnaryOperator *O = Result.Nodes.getNodeAs<UnaryOperator>(IncrementBindId);
  assert(O && "Bad Callback. No node provided");

  if (!O->isPostfix())
    return;

  SourceManager &SM = *Result.SourceManager;
  SourceLocation LocBegin = O->getSubExpr()->getLocStart();
  SourceLocation LocEnd = O->getOperatorLoc();

  if (!Result.Context->getSourceManager().isWrittenInMainFile(LocBegin))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(LocBegin) << '\n');
    return;
  }

  Replace->insert(tooling::Replacement(SM, LocBegin, 0, "crv::post_increment("));
  Replace->insert(tooling::Replacement(SM, LocEnd, 2, ")"));
}

void AssumeReplacer::run(const MatchFinder::MatchResult &Result) {
  const CallExpr *E = Result.Nodes.getNodeAs<CallExpr>(AssumeBindId);
  assert(E && "Bad Callback. No node provided");

  SourceManager &SM = *Result.SourceManager;
  SourceLocation LocBegin = E->getCallee()->getExprLoc();

  if (!Result.Context->getSourceManager().isWrittenInMainFile(LocBegin))
  {
    DEBUG(llvm::errs() << "Ignore file: " << SM.getFilename(LocBegin) << '\n');
    return;
  }

  const std::string NseAssume = NseStrategy + "::add_assertion";
  Replace->insert(tooling::Replacement(SM, LocBegin, 10, NseAssume));
}
