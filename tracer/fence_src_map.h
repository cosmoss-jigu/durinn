#include "llvm/IR/Instruction.h"

#include "LoadStoreNumbering.h"

#include <fstream>

namespace ciri{

class FenceSrcMap: public ModulePass {
public:
  static char ID;
  FenceSrcMap() : ModulePass(ID) {}

  /// \return false - The module was not modified.
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequiredTransitive<QueryLoadStoreNumbers>();

    // This pass is an analysis pass, so it does not modify anything
    AU.setPreservesAll();
  };

private:
  std::string locateSrcInfo(Instruction* I);
  void init();
  void run();
  void cleanup();

private:
  // output file stream
  std::ofstream output;

  /// Passes used by this pass
  const QueryLoadStoreNumbers *lsNumPass;
};

}
