//
// Created by 王珅祎 on 2021/10/30.
//

#include "insert_tx.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

// Declares llvm::cl::extrahelp.
#include "llvm/Support/raw_ostream.h"

std::string PoolFileName;


// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("insert-tx options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");

unsigned MyInputProcessor::GetTXBegin() {return tx_begin_line;}

unsigned MyInputProcessor::GetTXEnd() {return tx_end_line;}

bool MyRecursiveASTVisitor::VisitDeclRefExpr(DeclRefExpr *S){
  std::string call_name = S->getDecl()->getDeclName().getAsString();
  if(call_name == "pmemobj_persist"){
    auto parent = Context->getParents(*S)[0];
    const clang::Stmt *parent_node = parent.get<clang::Stmt>();
    auto grand_parent =  Context->getParents(*parent_node)[0];
    const clang::Stmt *grand_parent_node = grand_parent.get<clang::Stmt>();

    Rewrite.InsertText(grand_parent_node->getBeginLoc().getLocWithOffset(-1), "\tTX_BEGIN(" + PoolFileName + "){\n" , false, true);
    Rewrite.InsertText(grand_parent_node->getBeginLoc().getLocWithOffset(2), "\n\t}TX_END\n", true, true);
  }
  return true;
}


void PMEMPoolFinder::run(const MatchFinder::MatchResult &Result) {
    if (const clang::VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("pmemobj_create")){
        PoolFileName = VD->getDeclName().getAsString();
    }else{
        std::cout << "No persistent memory pool file!!!\n";
        assert(false);
    }
}


void MyASTConsumer::Initialize(ASTContext &Context) {
  SM = &Context.getSourceManager();
  LO = &CI->getLangOpts();
}


void MyASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  auto matcher = varDecl(hasInitializer(callExpr(callee(functionDecl(hasAnyName("pmemobj_create")))))).bind("pmemobj_create");
  Matcher.addMatcher(matcher, &Finder);
  Matcher.matchAST(Context);

  Inserter.InsertText(SM->translateLineCol(SM->getMainFileID(), InputData.GetTXBegin(), 1), "TX_BEGIN(" + PoolFileName + "){\n" , false, true);
  Inserter.InsertText(SM->translateLineCol(SM->getMainFileID(), InputData.GetTXEnd(), 1), "}TX_END\n", false, true);

//  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}


std::unique_ptr<ASTConsumer>
MyFrontendAction::CreateASTConsumer(CompilerInstance &Compiler, llvm::StringRef File) {
  TheRewriter.setSourceMgr(Compiler.getSourceManager(), Compiler.getLangOpts());
  return std::make_unique<MyASTConsumer>(TheRewriter, &Compiler.getASTContext(), &Compiler);
}


void MyFrontendAction::EndSourceFileAction() {
  const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(TheRewriter.getSourceMgr().getMainFileID());
  if (RewriteBuf != nullptr){
    std::ofstream outputFile;
    filename = std::string(getCurrentFile());
    auto insert_position = filename.find(".c");
    if(insert_position != std::string::npos){
      filename.insert(insert_position, "_tx");
    }else{
      filename.insert(filename.length(), "_tx");
    }
    if (!filename.empty()){
      outputFile.open(filename);
    }else{
      outputFile.open("output_tx");
    }

    outputFile << std::string(RewriteBuf->begin(), RewriteBuf->end());
    outputFile.close();
  }
}


int main(int argc, const char **argv) {
  // Create 'CompilationDatabase'
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  // Construct 'ClangTool'
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  int result = Tool.run(newFrontendActionFactory<MyFrontendAction>().get());

  return result;
}
