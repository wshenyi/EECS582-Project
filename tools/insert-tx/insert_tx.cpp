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

bool isDirectGridSizeInit = true; // if false, it means we don't need to run the matcher

SourceLocation sl;
int num_parents = 0;
int loop = 0;
std::string kernel_grid = "";
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
    auto grand_parent =  Context->getParents(*parent.get<clang::Stmt>())[0];
    const clang::Stmt *gnode = grand_parent.get<clang::Stmt>();

    Rewrite.InsertText(gnode->getBeginLoc().getLocWithOffset(-1), "TX_BEGIN(){\n", false, true);
    Rewrite.InsertText(gnode->getEndLoc().getLocWithOffset(2), "\n}TX_END\n", true, true);
  }
  /*
  if(traverseCount != 1){ // second time traversing the AST tree
    if(FunctionDecl *f = dyn_cast<FunctionDecl>(Declaration)){
      kernelCount = 0;
      num_parents = 0;
      loop = 0;
      if(f->hasAttr<CUDAGlobalAttr>()){
        ;
      }
      else{ // this FunctionDecl is not a CUDA kernel function declaration
        for(unsigned int i = 0; i < f->getNumParams(); i++){
          std::string parameters = f->parameters()[i]->getQualifiedNameAsString();
          parameterList.push_back(parameters);
        }
        if(f->doesThisDeclarationHaveABody()){
          if(Stmt *s = f->getBody()){
            functionName = f->getNameInfo().getName().getAsString();
            RewriteKernelCall(s);
            functionName = "";
            parameterList.clear();
          }
        }
      }
    }

  }else if(FunctionDecl *f = dyn_cast<FunctionDecl>(Declaration)){
    kernelCount = 0;
    num_parents = 0;
    loop = 0;
    if(f->hasAttr<CUDAGlobalAttr>()){
      // we found a FunctionDecl with __global__ attribute
      // which means this is a CUDA kernel function declaration
      TypeSourceInfo *tsi = f->getTypeSourceInfo();
      TypeLoc tl = tsi->getTypeLoc();
      FunctionTypeLoc FTL = tl.getAsAdjusted<FunctionTypeLoc>();
      if(f->getNumParams() == 0){
        Rewrite.InsertText(FTL.getRParenLoc(), "dim3 __SMC_orgGridDim, int __SMC_workersNeeded, int *__SMC_workerCount, int * __SMC_newChunkSeq, int * __SMC_seqEnds", true, true);
      }
      else{
        Rewrite.InsertText(FTL.getRParenLoc(), ", dim3 __SMC_orgGridDim, int __SMC_workersNeeded, int *__SMC_workerCount, int * __SMC_newChunkSeq, int * __SMC_seqEnds", true, true);
      }
      //Rewrite.InsertText(FTL.getRParenLoc(), ", dim3 __SMC_orgGridDim, int __SMC_workersNeeded, int *__SMC_workerCount, int * __SMC_newChunkSeq, int * __SMC_seqEnds", true, true);

      if(f->doesThisDeclarationHaveABody()){
        Stmt *FuncBody = f->getBody();
        Rewrite.InsertText(FuncBody->getBeginLoc().getLocWithOffset(1), "\n    __SMC_Begin\n", true, true);
        Rewrite.InsertText(FuncBody->getEndLoc(), "\n    __SMC_End\n", true, true);
        if(Stmt *s = f->getBody()){
          RewriteBlockIdx(s);
        }
      }
    }else{ // this FunctionDecl is not a CUDA kernel function declaration
      if(f->doesThisDeclarationHaveABody()){
        if(Stmt *s = f->getBody()){
          RewriteKernelCall(s);
        }
      }
    }
  }
   */
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
void MyRecursiveASTVisitor::RewriteKernelCall(Stmt *s) {
  /*
          We first check the traverseCount.
          If it's our first time traversing, rewrite the CUDA kernel call first, and store the grid variable name.
          If it's our second time traversing, check if the grid name variable is indirectly initialized.
          If it's our 3rd time traversing, check if the grid variable is an single int.
  */
  if(traverseCount != 1){// second time traversing the AST tree
    if(!parameterList.empty() && traverseCount == 2){
      std::list<std::string>::iterator finder = std::find(parameterList.begin(), parameterList.end(), kernel_grid);
      std::list<std::string>::iterator finder1 = std::find(functionnameList.begin(), functionnameList.end(), functionName);
      if (finder != parameterList.end() && finder1 != functionnameList.end()){
        std::stringstream orgGridDim;
        orgGridDim << "\n\t"
                   << "dim3 "
                   << "__SMC_orgGridDim"
                   << "("
                   << kernel_grid
                   << ");\n";
        std::cout<<"Function Parameter grid rewrite\n";
        Rewrite.InsertText(s->getBeginLoc().getLocWithOffset(1), orgGridDim.str(), true, true);
        isDirectGridSizeInit = false;
        parameterList.clear();
        return;
      }
    }

    if(gridX == 1 && gridY == 1 && traverseCount == 2){
      SourceLocation sl = s->getBeginLoc();
      std::stringstream gridVariable;
      gridVariable << "dim3 "
                   << "__SMC_orgGridDim"
                   << "("
                   << gridValueX
                   << ", "
                   << gridValueY
                   << ");\n";
      std::cout<<"Indirect grid rewrite\n";
      Rewrite.InsertText(sl, gridVariable.str(), true, true);
      traverseCount++;
      isDirectGridSizeInit = false;
      return;

    }

      /*
              Second time, check if the grid variable is defined in this form:
              dim3 grid;
              grid.x = ...;
              grid.y = ...;
      */
    else if(BinaryOperator *bo = dyn_cast<BinaryOperator>(s)){
      if(traverseCount == 2){


        std::string LHS = getStmtText(bo->getLHS());
        std::string RHS = getStmtText(bo->getRHS());
        std::stringstream gridNameX;
        gridNameX<< kernel_grid << ".x";
        std::stringstream gridNameY;
        gridNameY<< kernel_grid << ".y";

        if(LHS == gridNameX.str()){
          gridX++;
          gridValueX = RHS;
        }

        else if (LHS == gridNameY.str()){
          gridY++;
          gridValueY = RHS;
        }
      }
    }

      /*
              3rd time traversing, check if there is a integer grid variable. e.g. int numBlocks = 128;
      */
    else if(DeclStmt *ds = dyn_cast<DeclStmt>(s)){
      if(traverseCount == 3){
        if(ds->isSingleDecl()){
          Decl *d = ds->getSingleDecl();
          if(VarDecl *vd = dyn_cast<VarDecl>(d)){
            //if(ValueDecl *value = dyn_cast<ValueDecl>(vd)){
            //	std::string dataType = value->getType().getAsString();
            if(vd->hasInit()){
              std::string dataType = "";
              if(ValueDecl *value = dyn_cast<ValueDecl>(vd)){
                dataType = value->getType().getAsString();
              }
              std::string gridname = vd->getNameAsString();
              std::string gridvalue = getStmtText(vd->getInit());

              if(gridname == kernel_grid && dataType != "dim3"){
                // found 1D grid init
                //std::cout<<"SINGLE GRID DEMESION!\n";
                std::stringstream temp;
                temp << "dim3 "
                     << "__SMC_orgGridDim"
                     << "("
                     << gridvalue
                     << ");\n";
                gridIntValue = temp.str();
                gridInt = 1;
                //std::cout<<gridIntValue<<"\n";
                std::cout<<"Single int grid rewrite\n";
                std::cout<<"gridname: "<<gridname<<"\n";
                std::cout<<"gridvalue: "<<gridvalue<<"\n";
                std::cout<<"kernel_grid name: "<<kernel_grid<<"\n";
                Rewrite.InsertText(ds->getBeginLoc(), temp.str(), true, true);
                //traverseCount++;
                isDirectGridSizeInit = false;
              }
            }
          }
        }

        /*
        Expr *e = vd->getInit();
        if(BinaryOperator *bb = dyn_cast<BinaryOperator>(e)){
                std::string LHS = getStmtText(bb->getLHS());
                std::cout<<LHS<<"\n";
        }
        */
      }
    }
  }
  else if(CUDAKernelCallExpr *kce = dyn_cast<CUDAKernelCallExpr>(s)){
    if(traverseCount != 1){
      return;
    }
    //std::cout<<"inside cudakernelcallexpr\n";
    kernelCount++;
    //std::cout<<"KernelCount = "<<kernelCount<<"\n";
    CallExpr *kernelConfig = kce->getConfig();
    Expr *grid = kernelConfig->getArg(0);
    kernel_grid = getStmtText(grid);
    //std::cout<<kernel_grid<<"\n";
    Rewrite.ReplaceText(grid->getBeginLoc(), kernel_grid.length(), "grid");
    //std::cout<<"Finished rewrite grid in <<>>>\n";
    if(kernelCount == 1){
      //std::cout<<"Inside kerNelCount == 1 \n";
      //Rewrite.InsertText(kce->getBeginLoc(), "__SMC_init();\n", true, true);
      int result = GetParentStmt(*s);
      //std::cout<<"Now check if result == 1 \n";
      if(result == 1){
        //std::cout<<num_parents<<"\n";
        if(num_parents <= 1){
          //std::cout<<"num_parents <= 1 \n";
          Rewrite.InsertText(kce->getBeginLoc(), "__SMC_init();\n", true, true);
        }
        else{
          //std::cout<<"else\n";
          GetStmt(num_parents, *s);
        }
      }
    }
    //std::cout<<"Finished adding __SMC_init();\n=====================\n";
    //Rewrite.InsertText(kce->getBeginLoc(), "__SMC_init();\n", true, true);
    int num_args = kce->getNumArgs();
    if(num_args == 0){
      Rewrite.InsertText(kce->getRParenLoc(), "__SMC_orgGridDim, __SMC_workersNeeded, __SMC_workerCount, __SMC_newChunkSeq, __SMC_seqEnds", true, true);
    }
    else{
      Rewrite.InsertText(kce->getRParenLoc(), ", __SMC_orgGridDim, __SMC_workersNeeded, __SMC_workerCount, __SMC_newChunkSeq, __SMC_seqEnds", true, true);
    }
    //Rewrite.InsertText(kce->getRParenLoc(), ", __SMC_orgGridDim, __SMC_workersNeeded, __SMC_workerCount,__SMC_newChunkSeq, __SMC_seqEnds", true, true);
    //std::cout<<"finished adding 5 extra parameters for kernel call\n";
  }

  for(Stmt::child_iterator CI = s->child_begin(), CE = s->child_end(); CI != CE; ++CI){
    if (*CI) RewriteKernelCall(*CI);

  }
}
std::string MyRecursiveASTVisitor::getStmtText(Stmt *s) {
  SourceLocation a(SM->getExpansionLoc(s->getBeginLoc())),
                 b(Lexer::getLocForEndOfToken(SourceLocation(SM->getExpansionLoc(s->getEndLoc())),
                                   0,  *SM, *LO));
  return std::string(SM->getCharacterData(a), SM->getCharacterData(b)-SM->getCharacterData(a));
}


void GridHandler::run(const MatchFinder::MatchResult &Result) {
  //std::cout<<"Executing GridHandler\n";
  if(const VarDecl *vd = Result.Nodes.getNodeAs<VarDecl>("gridcall")){
    //std::cout<<"1\n";
    SourceLocation source;
    if(vd->hasInit()){
      source = vd->getInit()->getBeginLoc();
    }
    else{
      source = vd->getBeginLoc();
    }
    //std::cout<<"2\n";
    if(sl != source){
      //std::cout<<"3\n";
      if(vd->hasInit()){
        sl = vd->getInit()->getBeginLoc();
      }
      else{
        sl = vd->getBeginLoc();
      }
      //sl  = vd->getInit()->getBeginLoc();
      //std::cout<<"3.5\n";
      std::cout<<"Mathcer grid rewrite\n";
      Rewrite.ReplaceText(sl, kernel_grid.length(), "__SMC_orgGridDim");
    }

    //SourceLocation sl = vd->getInit()->getBeginLoc();
    //std::cout<<"About to add __SMC_orgGridDim!\n";
    //std::cout<<kernel_grid<<"\n";
    //Rewrite.ReplaceText(sl, kernel_grid.length(), "__SMC_orgGridDim");
  }
}


MyASTConsumer::MyASTConsumer(Rewriter &Rewrite, ASTContext *Context,
                             CompilerInstance *comp)
    : visitor(Rewrite, Context), HandleGrid(Rewrite), CI(comp){
//  SourceLocation startOfFile = Rewrite.getSourceMgr().getLocForStartOfFile(Rewrite.getSourceMgr().getMainFileID());
//  Rewrite.InsertText(startOfFile, "/* Added smc.h*/ \n#include \"smc.h\"\n\n",true,true);
}
void MyASTConsumer::Initialize(ASTContext &Context) {
  SM = &Context.getSourceManager();
  LO = &CI->getLangOpts();
}
void MyASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  // visitor.TraverseDecl(Context.getTranslationUnitDecl());

  // visitor.TraverseDecl(Context.getTranslationUnitDecl());

  visitor.TraverseDecl(Context.getTranslationUnitDecl());

//  if(isDirectGridSizeInit){
//    //std::cout<<kernel_grid<<"\n";
//    Matcher.addMatcher(varDecl(hasName(kernel_grid)).bind("pmemobj_persist"), &HandleGrid);
//    Matcher.matchAST(Context);
//  }
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
