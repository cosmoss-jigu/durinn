#include "fence_src_map.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include <iostream>
#include <string.h>
#include <sstream>

using namespace ciri;

//===----------------------------------------------------------------------===//
//                        Command Line Arguments
//===----------------------------------------------------------------------===//

cl::opt<std::string>
FenceIds("fence-ids", cl::desc("a list of id of fences"), cl::init("-"));

cl::opt<std::string>
OutputFile("output", cl::desc("output file"), cl::init("-"));

//===----------------------------------------------------------------------===//
//                             Implementations
//===----------------------------------------------------------------------===//
// ID Variable to identify the pass
char FenceSrcMap::ID = 0;

// Pass registration
static RegisterPass<FenceSrcMap> X("dfencesrcmap", "Fence Src Map");

std::string FenceSrcMap::locateSrcInfo(Instruction *I) {
  if (DILocation *Loc = I->getDebugLoc().get()) {
    std::stringstream ss;
    ss << Loc->getFilename().str() << ":" << Loc->getLine();
    return ss.str();
  } else {
    return "NIL";
  }
}

void FenceSrcMap::init() {
  // get the QueryLoadStoreNumbers for instruction ID
  lsNumPass = &getAnalysis<QueryLoadStoreNumbers>();

  // init output file stream
  output.open(OutputFile);
}

void FenceSrcMap::run() {
  std::string id_str;
  std::istringstream id_stream(FenceIds);
  while (std::getline(id_stream, id_str, ',')) {
    long id = stol(id_str);
    Instruction* I = lsNumPass->getInstByID(id);
    std::string srcInfo = locateSrcInfo(I);
    output << id << "," << srcInfo << std::endl;
  }
}

void FenceSrcMap::cleanup() {
  output.close();
}

bool FenceSrcMap::runOnModule(Module &M) {
  init();
  run();
  cleanup();
  // This is an analysis pass, so always return false.
  return false;
}
