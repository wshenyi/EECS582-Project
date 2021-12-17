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
#include "nlohmann/json.hpp"

#include <vector>
#include <utility>

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;
using json = nlohmann::json;

class MyInputManager{
private:
    std::vector<std::pair<unsigned, unsigned>> interval_array;
    std::string filepath;

public:
    explicit MyInputManager();

    void RegulateFormat(std::string &s);
    void MergeInterval();
    unsigned GetSize();
    void AddInterval(unsigned int BeginLine, unsigned int EndLine);
    std::pair<unsigned, unsigned>& GetInterval(unsigned int index);
};

class MyRecursiveASTVisitor : public RecursiveASTVisitor <MyRecursiveASTVisitor> {
private:
  Rewriter &Rewrite;
  ASTContext *Context;

public:
  explicit MyRecursiveASTVisitor(Rewriter &R, ASTContext *Context):Rewrite(R), Context(Context){}

//  bool VisitDeclRefExpr(DeclRefExpr *S);

  bool VisitFunctionDecl(FunctionDecl *F);
};

class PMEMPoolFinder : public MatchFinder::MatchCallback{
public:
  virtual void run(const MatchFinder::MatchResult &Result) override;
};

class MyASTConsumer: public ASTConsumer {
private:
  MyInputManager InputData;
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
