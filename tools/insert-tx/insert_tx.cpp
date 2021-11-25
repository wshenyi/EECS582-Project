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

// global variable for counting how many times a CUDA kernel function has been called.
int kernelCount = 0; // used to find the first kernel call
int traverseCount = 1; // for this tool we need to traverse the AST tree 3 times
int gridX = 0;
int gridY = 0;
int gridInt = 0;
std::string gridValueX;
std::string gridValueY;
std::string gridIntValue;
std::list<std::string> parameterList = {};
std::list<std::string> functionnameList = {};
std::string functionName;

std::string PoolFileName;

SourceLocation sl;
int num_parents = 0;
int loop = 0;
//CompilerInstance *CI;
SourceManager *SM;
LangOptions *LO;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("insert-tx options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");


bool MyRecursiveASTVisitor::VisitDeclRefExpr(DeclRefExpr *S){
  std::string call_name = S->getDecl()->getDeclName().getAsString();
  if(call_name == "pmemobj_persist"){
    auto parent = Context->getParents(*S)[0];
    const clang::Stmt *parent_node = parent.get<clang::Stmt>();
    auto grand_parent =  Context->getParents(*parent_node)[0];
    const clang::Stmt *grand_parent_node = grand_parent.get<clang::Stmt>();

    Rewrite.InsertText(grand_parent_node->getBeginLoc().getLocWithOffset(-1), "\tTX_BEGIN(" + PoolFileName + "){\n" , false, true);
    Rewrite.InsertText(grand_parent_node->getEndLoc().getLocWithOffset(2), "\n\t}TX_END\n", true, true);
  }
  return true;
}


void MyRecursiveASTVisitor::RewriteBlockIdx(Stmt *s) {
  if(MemberExpr *me = dyn_cast<MemberExpr>(s)){
    //std::string member = me->getMemberDecl()->getNameAsString();
    //me->dump();
    std::string member = me->getMemberDecl()->getNameAsString();
    if(OpaqueValueExpr *ove = dyn_cast<OpaqueValueExpr>(me->getBase())){
      Expr *SE = ove->getSourceExpr();
      if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(SE)) {
        std::string base = DRE->getNameInfo().getAsString();
        if(base == "blockIdx" && member == "__fetch_builtin_x"){
          Rewrite.ReplaceText(me->getBeginLoc(), 10, "(int)fmodf((float)__SMC_chunkID, (float)__SMC_orgGridDim.x)");
        }
        if(base == "blockIdx" && member == "__fetch_builtin_y"){
          Rewrite.ReplaceText(me->getBeginLoc(), 10, "(int)(__SMC_chunkID/__SMC_orgGridDim.x)");
        }
        //std::cout<<base<<"."<<member<<"\n";
      }
    }
    //std::cout<<"This is a MemberExpr in kernel function! Member name: ";
    //std::cout<<me->getMemberNameInfo().getName().getAsString() << "\n";
  }
  for(Stmt::child_iterator CI = s->child_begin(), CE = s->child_end(); CI != CE; ++CI){
    if (*CI) RewriteBlockIdx(*CI);
  }
}


int MyRecursiveASTVisitor::GetParentStmt(const Stmt &stmt) {
  auto it = Context->getParents(stmt).begin();
  if(it == Context->getParents(stmt).end()){
    return 1;
  }
  else{
    const clang::Stmt *s = it->get<clang::Stmt>();
    if(s){
      num_parents++;
      return GetParentStmt(*s);
    }
    const clang::FunctionDecl *fd = it->get<clang::FunctionDecl>();
    if(fd){
      std::string functionname = fd->getNameInfo().getName().getAsString();
      functionnameList.push_back(functionname);
      //std::cout<<functionname<<"\n";
    }
    return 1;
  }

  return 0;
}


void MyRecursiveASTVisitor::GetStmt(int num_parents, const Stmt &stmt) {
  if(loop == num_parents-2){
    auto it = Context->getParents(stmt).begin();
    const clang::Stmt *s = it->get<clang::Stmt>();
    Rewrite.InsertText(s->getBeginLoc(), "__SMC_init();\n", true, true);
  }
  else{
    auto it = Context->getParents(stmt).begin();
    const clang::Stmt *s = it->get<clang::Stmt>();
    loop++;
    return GetStmt(num_parents, *s);
  }

}


void MyRecursiveASTVisitor::RewriteKernelCall(Stmt *s){}


std::string MyRecursiveASTVisitor::getStmtText(Stmt *s) {
  SourceLocation a(SM->getExpansionLoc(s->getBeginLoc())),
                 b(Lexer::getLocForEndOfToken(SourceLocation(SM->getExpansionLoc(s->getEndLoc())),
                                   0,  *SM, *LO));
  return std::string(SM->getCharacterData(a), SM->getCharacterData(b)-SM->getCharacterData(a));
}


void PMEMPoolFinder::run(const MatchFinder::MatchResult &Result) {
    if (const clang::VarDecl *VD = Result.Nodes.getNodeAs<clang::VarDecl>("pmemobj_create")){
        PoolFileName = VD->getDeclName().getAsString();
    }else{
        std::cout << "No persistent memory pool file!!!\n";
        assert(false);
    }
}


MyASTConsumer::MyASTConsumer(Rewriter &Rewrite, ASTContext *Context, CompilerInstance *comp)
    : Visitor(Rewrite, Context), Finder(Rewrite), CI(comp){
//  SourceLocation startOfFile = Rewrite.getSourceMgr().getLocForStartOfFile(Rewrite.getSourceMgr().getMainFileID());
//  Rewrite.InsertText(startOfFile, "/* Added smc.h*/ \n#include \"smc.h\"\n\n",true,true);
}


void MyASTConsumer::Initialize(ASTContext &Context) {
  SM = &Context.getSourceManager();
  LO = &CI->getLangOpts();
}


void MyASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  auto dsl = varDecl(hasInitializer(callExpr(callee(functionDecl(hasAnyName("pmemobj_create")))))).bind("pmemobj_create");
  Matcher.addMatcher(dsl, &Finder);
  Matcher.matchAST(Context);

  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}


std::unique_ptr<ASTConsumer>
MyFrontendAction::CreateASTConsumer(CompilerInstance &Compiler,
                                    llvm::StringRef File) {
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
