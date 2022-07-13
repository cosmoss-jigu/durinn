#include "print_lps_from_brs.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/Debug.h"

#include <iostream>
#include <string.h>
#include <sstream>
#include <queue>

using namespace ciri;

//===----------------------------------------------------------------------===//
//                             Implementations
//===----------------------------------------------------------------------===//
// ID Variable to identify the pass
char PrintLpsFromBrs::ID = 0;

// Pass registration
static RegisterPass<PrintLpsFromBrs> X("dprintlpsfrombrs", "PrintLpsFromBrs");

void PrintLpsFromBrs::init() {
  // get the QueryLoadStoreNumbers for instruction ID
  lsNumPass = &getAnalysis<QueryLoadStoreNumbers>();

  input.open("brs.txt");
  std::string id_str;
  while (std::getline(input, id_str)) {
    long id = stol(id_str);
    Instruction* I = lsNumPass->getInstByID(id);
    BranchInst *BI = dyn_cast<BranchInst>(I);
    assert(BI->isConditional());
    BIs.insert(BI);
  }
  input.close();
}

void PrintLpsFromBrs::get_lps_from_br(BranchInst* BI) {
  std::queue<Value*> todo;
  std::unordered_set<Value*> visited;
  todo.push(BI->getCondition());
  visited.insert(BI->getCondition());
  while (!todo.empty()) {
    Value* V = todo.front();
    todo.pop();
    if (isa<LoadInst>(V)) {
      LoadInst *LI = dyn_cast<LoadInst>(V);
      LPs.insert(LI);

      BasicBlock* bb = LI->getParent();
      for (BasicBlock::iterator i = bb->begin(), ie = bb->end(); i != ie; ++i) {
        if (isa<StoreInst>(&*i)){
          StoreInst* SI = dyn_cast<StoreInst>(&*i);
          if (SI->getPointerOperand() == LI->getPointerOperand()) {
            Value* next_v = SI->getValueOperand();
            if (!isa<Constant>(next_v) && visited.find(next_v) == visited.end()) {
              todo.push(next_v);
              visited.insert(next_v);
            }
          }
        }
      }
    } else if (isa<AtomicCmpXchgInst>(V)) {
      Instruction *I = dyn_cast<Instruction>(V);
      LPs.insert(I);
    } else if (isa<CallInst>(V)) {
      CallInst *CI = dyn_cast<CallInst>(V);
      for (int i = 0; i < CI->arg_size(); i++) {
        Value* next_v = CI->getArgOperand(i);
        if (!isa<Constant>(next_v) && visited.find(next_v) == visited.end()) {
          todo.push(next_v);
          visited.insert(next_v);
        }
      }

      Function* CalledFunc = CI->getCalledFunction();
      if (!CalledFunc) {
        continue;
      }
      //if (CalledFunc->isDeclaration()) {
      //  continue;
      //}
      for (Function::iterator b = CalledFunc->begin(), be = CalledFunc->end(); b != be; ++b) {
        BasicBlock* bb = dyn_cast<BasicBlock>(&*b);
        Instruction* I = bb->getTerminator();
        if (isa<ReturnInst>(I)) {
          ReturnInst* RI = dyn_cast<ReturnInst>(I);
          Value* next_v = RI->getReturnValue();
          if (!isa<Constant>(next_v) && visited.find(next_v) == visited.end()) {
            todo.push(next_v);
            visited.insert(next_v);
          }
        }
      }
    } else if (isa<Instruction>(V)) {
    //} else if (isa<CmpInst>(V) ||
    //           isa<UnaryInstruction>(V) ||
    //           isa<BinaryOperator>(V) ||
    //           isa<PHINode>(V)) {
      Instruction *I = dyn_cast<Instruction>(V);
      for (unsigned index = 0; index < I->getNumOperands(); ++index) {
        Value* next_v = I->getOperand(index);
        if (!isa<Constant>(next_v) && visited.find(next_v) == visited.end()) {
          todo.push(next_v);
          visited.insert(next_v);
        }
      }
    }
  }
}

void PrintLpsFromBrs::run() {
  for (BranchInst* BI : BIs) {
    get_lps_from_br(BI);
  }
}

void PrintLpsFromBrs::cleanup() {
  // init output file stream
  output.open("lps_from_brs.txt");
  for (Instruction* lp : LPs) {
    unsigned id = lsNumPass->getID(lp);
    output << id << std::endl;
  }
  output.close();
}

bool PrintLpsFromBrs::runOnModule(Module &M) {
  init();
  run();
  cleanup();
  // This is an analysis pass, so always return false.
  return false;
}
