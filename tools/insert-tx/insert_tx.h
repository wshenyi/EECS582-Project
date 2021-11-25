//
// Created by 王珅祎 on 2021/10/30.
//

#ifndef CLANG_INSERT_TX_H
#define CLANG_INSERT_TX_H

#include "clang/AST/AST.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/Tooling.h"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;

class MyRecursiveASTVisitor : public RecursiveASTVisitor <MyRecursiveASTVisitor> {
private:
  Rewriter &Rewrite;
  ASTContext *Context;

public:
  explicit MyRecursiveASTVisitor(Rewriter &R, ASTContext *Context):Rewrite(R), Context(Context){}

  // This function is used to traverse the entire AST tree and finds any function declarations.
  // If any FunctionDecl is found, check if it's a CUDA kernel FunctionDecl, then perform different action accordingly.
  bool VisitDeclRefExpr(DeclRefExpr *S);

  // This function is to rewrite the blockIdx.x and blockIdx.y to smc form
  void RewriteBlockIdx(Stmt *s);

  // try to find the first CUDAKernelCallExpr's parent and
  // to see if this Expr has any parent if/for/while stmt.
  // hope this works
  int GetParentStmt(const clang::Stmt& stmt);

  //This function inserts __SMC_init(); at the right place.
  void GetStmt(int num_parents, const clang::Stmt& stmt);

  /*
          The is the core function of this tool.
          For every CUDA kernel function call, it first record the name of the grid variable during the first traverse.
          Then it tries to find out if the grid variable is initialized separately in the form of grid.x = ... and grid.y = ...
          If this is the case, add a new line after the initialization: dim3 __SMC_orgGridDim();

          If no, it traverse the AST for the 3rd time and tries to find out if the grid variable is initialized as a integer, in other words, 1-D grid.
          If yes, rewrite the grid variable to dim3 __SMC_orgGridDim(...);
  */
  void RewriteKernelCall(Stmt *s);

  // This function returns the string representation of any give stmt pointer
  std::string getStmtText(Stmt *s);
};

class PMEMPoolFinder : public MatchFinder::MatchCallback{
private:
  Rewriter &Rewrite;

public:
  PMEMPoolFinder(Rewriter &Rewrite) : Rewrite(Rewrite){}
  virtual void run(const MatchFinder::MatchResult &Result) override;
};

class MyASTConsumer: public ASTConsumer {
private:
  MyRecursiveASTVisitor Visitor;
  PMEMPoolFinder Finder;
  MatchFinder Matcher;
  CompilerInstance *CI;
  //SourceManager *SM;
  //LangOptions *LO;

public:

  explicit MyASTConsumer(Rewriter &Rewrite, ASTContext *Context, CompilerInstance *comp);

  virtual void Initialize(ASTContext &Context) override;

  virtual void HandleTranslationUnit(ASTContext &Context) override;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
private:
  Rewriter TheRewriter;
  std::string filename;

public:
  MyFrontendAction() {}

  virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance &Compiler, llvm::StringRef InFile) override;

  void EndSourceFileAction() override;
};

#endif // CLANG_INSERT_TX_H
