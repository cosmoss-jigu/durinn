#include "print_src_map.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include <iostream>
#include <string.h>
#include <sstream>

using namespace ciri;

// ID Variable to identify the pass
char PrintSrcMap::ID = 0;

// Pass registration
static RegisterPass<PrintSrcMap> X("dprintsrcmap", "Print Src Map");

std::string PrintSrcMap::locateSrcInfo(Instruction *I) {
  if (DILocation *Loc = I->getDebugLoc().get()) {
    std::stringstream ss;
    ss << Loc->getFilename().str() << ":" << Loc->getLine();
    return ss.str();
  } else {
    return "NIL";
  }
}

void PrintSrcMap::print_src_map_to_output(Instruction *I, unsigned id, std::string suffix) {
    std::string srcInfo = locateSrcInfo(I);
    output << id << "," << srcInfo << "," << suffix << std::endl;
}

void PrintSrcMap::init() {
  count = 0;
  // init output file stream
  output.open("print_src_map.txt");
}

void PrintSrcMap::cleanup() {
  output.close();
}

bool PrintSrcMap::runOnModule(Module &M) {
  init();
  visit(&M);
  cleanup();
  // This is an analysis pass, so always return false.
  return false;
}
