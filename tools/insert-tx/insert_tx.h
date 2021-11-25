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

class MyInputProcessor{
private:
    unsigned tx_begin_line;
    unsigned tx_end_line;

public:
    MyInputProcessor(){
        tx_begin_line = 39;
        tx_end_line = 40;
    }

    unsigned GetTXBegin();
    unsigned GetTXEnd();
};

class MyRecursiveASTVisitor : public RecursiveASTVisitor <MyRecursiveASTVisitor> {
private:
  Rewriter &Rewrite;
  ASTContext *Context;

public:
  explicit MyRecursiveASTVisitor(Rewriter &R, ASTContext *Context):Rewrite(R), Context(Context){}

  // This function is used to traverse the entire AST tree and finds any function declarations.
  // If any FunctionDecl is found, check if it's a CUDA kernel FunctionDecl, then perform different action accordingly.
  bool VisitDeclRefExpr(DeclRefExpr *S);

};

class PMEMPoolFinder : public MatchFinder::MatchCallback{
public:
  virtual void run(const MatchFinder::MatchResult &Result) override;
};

class MyASTConsumer: public ASTConsumer {
private:
  MyInputProcessor InputData;
  MyRecursiveASTVisitor Visitor;
  Rewriter &Inserter;
  PMEMPoolFinder Finder;
  MatchFinder Matcher;
  CompilerInstance *CI;
  SourceManager *SM;
  LangOptions *LO;

public:

  explicit MyASTConsumer(Rewriter &Rewrite, ASTContext *Context, CompilerInstance *comp)
  : Visitor(Rewrite, Context), Inserter(Rewrite), CI(comp){}

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
