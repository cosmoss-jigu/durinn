#include "LoadStoreNumbering.h"
#include <unordered_set>
#include <fstream>

using namespace llvm;

namespace ciri{

class PrintLpsFromBrs: public ModulePass {
public:
  static char ID;
  PrintLpsFromBrs() : ModulePass(ID) {}

  /// \return false - The module was not modified.
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequiredTransitive<QueryLoadStoreNumbers>();

    // This pass is an analysis pass, so it does not modify anything
    AU.setPreservesAll();
  };
private:
  void get_lps_from_br(BranchInst* BI);
  void init();
  void run();
  void cleanup();

private:
  // input file stream
  std::ifstream input;
  // output file stream
  std::ofstream output;

  std::unordered_set<BranchInst*> BIs;
  std::unordered_set<Instruction*> LPs;

  /// Passes used by this pass
  const QueryLoadStoreNumbers *lsNumPass;
};

}
