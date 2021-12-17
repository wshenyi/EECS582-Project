//
// Created by 王珅祎 on 2021/10/30.
//

#include "insert_tx.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

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
static cl::opt<std::string> TargetFile(
    cl::Positional,
    cl::init("-"),
    cl::desc("Specify input source file"),
    cl::value_desc("filename"),
//    cl::Required,
    cl::cat(MyToolCategory)
);

static cl::opt<std::string> PositionFile(
    "pos",
    cl::init("-"),
    cl::desc("Provide positions of intervals for transaction insertion"),
    cl::value_desc("filename"),
//    cl::Required,
    cl::cat(MyToolCategory)
);

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp(
    "\tFor example, to run clang-check on all files in a subtree of the\n"
    "\tsource tree, use:\n"
    "\n"
    "\t  find path/in/subtree -name '*.cpp'|xargs clang-check\n"
    "\n"
    "\tor using a specific build path:\n"
    "\n"
    "\t  find path/in/subtree -name '*.cpp'|xargs clang-check -p build/path\n"
    "\n"
    "\tNote, that path/in/subtree and current directory should follow the\n"
    "\trules described above.\n"
    "\n"
);

//void MyInputManager::SetInterval(unsigned int index, unsigned int LineNum) {
//  assert(index < 0 || LineNum < 0);
//
//  interval_array[]
//}

MyInputManager::MyInputManager(){
  std::string s;
  std::ifstream file(PositionFile.c_str());

  if(file.good()){
    while(!file.eof()){
      std::getline(file, s);
      RegulateFormat(s);
      json begin = json::parse(s.c_str());
      std::getline(file, s);
      RegulateFormat(s);
      json end = json::parse(s.c_str());
      AddInterval(begin["line"], end["line"]);
    }
  }else{
    std::cout << "Open file failed!\n";
    assert(false);
  }

  file.close();
  MergeInterval();
}

void MyInputManager::RegulateFormat(std::string &s) {
  auto pos = s.find("'");
  while(std::string::npos != pos){
    s.replace(pos, 1, "\"");
    pos = s.find("'");
  }
}

void MyInputManager::MergeInterval(){
  unsigned int flag = 1;
  unsigned int begin, end, begin_t, end_t;
  for(unsigned i = 1; i < interval_array.size(); i++ ){
    begin = interval_array[i].first;
    end   = interval_array[i].second;
    bool trigger = true;
    for(unsigned j = 0; j < flag; j++){
      begin_t = interval_array[j].first;
      end_t   = interval_array[j].second;
      if(!(begin_t >= end || end_t <= begin)){
        trigger = false;
        interval_array[j].first = std::min(begin_t, begin);
        interval_array[j].second = std::max(end_t, end);
        break;
      }
    }
    if(trigger){
      interval_array[flag].first = begin;
      interval_array[flag].second = end;
      flag++;
    }
  }

  for(unsigned back = interval_array.size() - 1; back >= flag; back--){
    interval_array.pop_back();
  }
}

unsigned MyInputManager::GetSize(){return interval_array.size();}

void MyInputManager::AddInterval(unsigned int BeginLine, unsigned int EndLine) {
  assert(BeginLine != 0 || EndLine != 0);

  interval_array.push_back(std::pair<unsigned, unsigned>(BeginLine, EndLine));
}

std::pair<unsigned, unsigned>& MyInputManager::GetInterval(unsigned int index) {
  assert(interval_array.size() != 0);

  return interval_array[index];
}

//bool MyRecursiveASTVisitor::VisitDeclRefExpr(DeclRefExpr *S){
//  std::string call_name = S->getDecl()->getDeclName().getAsString();
//  if(call_name == "pmemobj_persist"){
//    auto parent = Context->getParents(*S)[0];
//    const clang::Stmt *parent_node = parent.get<clang::Stmt>();
//    auto grand_parent =  Context->getParents(*parent_node)[0];
//    const clang::Stmt *grand_parent_node = grand_parent.get<clang::Stmt>();
//      PMEMoid root = pmemobj_root(pop, sizeof (struct vector));
//    Rewrite.InsertText(grand_parent_node->getBeginLoc().getLocWithOffset(-1), "\tTX_BEGIN(" + PoolFileName + "){\n" , false, true);
//    Rewrite.InsertText(grand_parent_node->getBeginLoc().getLocWithOffset(2), "\n\t}TX_END\n", true, true);
//  }
//  return true;
//}

bool MyRecursiveASTVisitor::VisitFunctionDecl(FunctionDecl *F) {
  unless(isExpansionInSystemHeader());
//  if(F->hasBody()){
//
//  }
//  F->getASTContext();
//  Context->getSourceManager().isInSLocAddrSpace()
return true;
}

void PMEMPoolFinder::run(const MatchFinder::MatchResult &Result) {
    unless(isExpansionInSystemHeader());
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

//  for(unsigned i = 0; i < InputData.GetSize(); i++){
//    auto interval = InputData.GetInterval(i);
//
//    SourceLocation begin = SM->translateLineCol(SM->getMainFileID(), interval.first, 1);
//    SourceLocation end   = SM->translateLineCol(SM->getMainFileID(), interval.second, 1);
//
//    Inserter.InsertText(begin, "TX_BEGIN(" + PoolFileName + "){\n" , false, true);
//    Inserter.InsertText(end, "}TX_END\n", false, true);
//  }

  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}


std::unique_ptr<ASTConsumer>
MyFrontendAction::CreateASTConsumer(CompilerInstance &Compiler, llvm::StringRef File) {
  TheRewriter.setSourceMgr(Compiler.getSourceManager(), Compiler.getLangOpts());
  return std::make_unique<MyASTConsumer>(TheRewriter, &Compiler.getASTContext(), &Compiler);
}


void MyFrontendAction::EndSourceFileAction() {
  const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(TheRewriter.getSourceMgr().getMainFileID());
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


int main(int argc, const char **argv) {
  // Create 'CompilationDatabase'
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  // Construct 'ClangTool'
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  int result = Tool.run(newFrontendActionFactory<MyFrontendAction>().get());

  return result;
}
