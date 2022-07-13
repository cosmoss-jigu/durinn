#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstVisitor.h"

#include <fstream>

using namespace llvm;

namespace ciri{

class PrintSrcMap : public ModulePass,
                    public InstVisitor<PrintSrcMap> {
public:
  static char ID;
  PrintSrcMap() : ModulePass(ID) {}

  /// \return false - The module was not modified.
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    // This pass is an analysis pass, so it does not modify anything
    AU.setPreservesAll();
  };

  ////////////////////// Instruction visitors //////////////////////
  void visitLoadInst(LoadInst &LI) {
    print_src_map_to_output(&LI, ++count, "load");
  }
  void visitStoreInst(StoreInst &SI) {
    print_src_map_to_output(&SI, ++count, "store");
  }
  void visitSelectInst(SelectInst &SI) {
    print_src_map_to_output(&SI, ++count, "select");
  }
  void visitCallInst(CallInst &CI) {
    print_src_map_to_output(&CI, ++count, "call");
  }
  void visitAtomicRMWInst(AtomicRMWInst &AI) {
    print_src_map_to_output(&AI, ++count, "atomic_rmw");
  }
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &AI) {
    print_src_map_to_output(&AI, ++count, "atomic_cmp");
  }

  void visitBranchInst(BranchInst &BI) {
    if (BI.isUnconditional()) {
      return;
    }
    print_src_map_to_output(&BI, ++count, "br_cond");
  }

private:
  std::string locateSrcInfo(Instruction* I);
  void init();
  void cleanup();
  void print_src_map_to_output(Instruction *I, unsigned id, std::string suffix);

private:
  // output file stream
  std::ofstream output;
  unsigned count; ///< Counter for assigning unique IDs
};

}
