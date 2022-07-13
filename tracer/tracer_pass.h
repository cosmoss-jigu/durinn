#include "LoadStoreNumbering.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstVisitor.h"
#include <vector>
#include <cstdarg>
#include <string>
#include <unordered_set>

using namespace llvm;

namespace ciri {

static inline Value *castTo(Value *V,
                            Type *Ty,
                            Twine Name,
                            Instruction *InsertPt) {
  // Assert that we're not trying to cast a NULL value.
  assert (V && "castTo: trying to cast a NULL Value!\n");

  // Don't bother creating a cast if it's already the correct type.
  if (V->getType() == Ty)
    return V;

  // If it's a constant, just create a constant expression.
  if (Constant *C = dyn_cast<Constant>(V)) {
    Constant *CE = ConstantExpr::getZExtOrBitCast(C, Ty);
    return CE;
  }

  // Otherwise, insert a cast instruction.
  return CastInst::CreateZExtOrBitCast(V, Ty, Name, InsertPt);
}


template<typename T>
inline std::vector<T> make_vector(T A, ...) {
  va_list Args;
  va_start(Args, A);
  std::vector<T> Result;
  Result.push_back(A);
  while (T Val = va_arg(Args, T))
    Result.push_back(Val);
  va_end(Args);
  return Result;
}

static inline GlobalVariable *stringToGV(const std::string &s, Module *M) {
  //create a constant string array and add a null terminator
  Constant *arr = ConstantDataArray::getString(M->getContext(),
                                               StringRef(s),
                                               true);
  return new GlobalVariable(*M,
                            arr->getType(),
                            true,
                            GlobalValue::InternalLinkage,
                            arr,
                            Twine("str"));
}

class CiriTracerPass: public ModulePass,
                      public InstVisitor<CiriTracerPass> {
public:
  static char ID;
  CiriTracerPass() : ModulePass(ID) {}

  virtual bool doInitialization(Module &M);
  virtual bool doFinalization(Module &M) { return false; }
  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<QueryLoadStoreNumbers>();
    AU.addPreserved<QueryLoadStoreNumbers>();
    AU.setPreservesCFG();
  };

  /// Visit a load instruction. This method instruments the load instruction
  /// with a call to the tracing run-time that will record, in the dynamic
  /// trace, the memory read by this load instruction.
  void visitLoadInst(LoadInst &LI);

  /// Visit a store instruction. This method instruments the store instruction
  /// with a call to the tracing run-time that will record, in the dynamic
  /// trace, the memory written by this store instruction.
  void visitStoreInst(StoreInst &SI);

  /// Visit an atomic instruction and record it as a store
  void visitAtomicRMWInst(AtomicRMWInst &AI);
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &AI);

  /// Visit a call instruction. For most call instructions, we will instrument
  /// the call so that the trace contains enough information to track back from
  /// the callee to the caller. For some call instructions, we will emit
  /// special instrumentation to record the memory read and/or written by the
  /// call instruction. Also call records are needed to map invariant failures
  /// to call insts.
  void visitCallInst(CallInst &CI);
  void visitInvokeInst(InvokeInst &II);

  void visitBranchInst(BranchInst &BI);


private:
  std::unordered_set<std::string> lock_modeling_set;
  std::unordered_set<std::string> unlock_modeling_set;

  // Pointers to other passes
  const DataLayout *TD;
  const QueryLoadStoreNumbers *lsNumPass;

  // Functions for recording events during execution
  FunctionCallee RecordBr;
  FunctionCallee RecordLoad;
  FunctionCallee RecordStore;
  FunctionCallee RecordAtomicStore;
  FunctionCallee RecordCAS;
  FunctionCallee RecordStrLoad;
  FunctionCallee RecordStrStore;
  FunctionCallee RecordStrcatStore;
  FunctionCallee RecordFlush;
  FunctionCallee RecordFlushWrapper;
  FunctionCallee RecordFence;
  FunctionCallee Init;
  FunctionCallee EnableRecording;
  FunctionCallee RecordLock;
  FunctionCallee RecordLockBegin;
  FunctionCallee RecordLockEnd;
  FunctionCallee RecordUnlock;
  FunctionCallee RecordUnlockBegin;
  FunctionCallee RecordUnlockEnd;
  FunctionCallee RecordAPIBegin;
  FunctionCallee RecordAPIEnd;
  FunctionCallee Locking;
  FunctionCallee Unlocking;
  FunctionCallee RecordAlloc;
  FunctionCallee RecordTxAlloc;
  FunctionCallee RecordMmap;

  Type *Int8Type;
  Type *Int32Type;
  Type *Int64Type;
  Type *VoidType;
  Type *VoidPtrType;
  Type *OIDType;

private:
  bool visitInlineAsm(CallInst &CI);
  bool visitSpecialCall(CallInst &CI, std::string name);
  bool visitPmemCall(CallInst &CI, std::string name);
  bool visitLockCall(CallInst &CI, std::string name);
  bool visitMmapCall(CallInst &CI, std::string name);
  void instrumentLock(Instruction *I);
  void instrumentUnlock(Instruction *I);
  void instrumentInit(Module &M);
  void instrumentAPIBegin(Module &M);
  void instrumentAPIEnd(Module &M);
  void instrumentLockModeling(Module &M);
  void instrumentUnlockModeling(Module &M);
};

} // END namespace ciri
