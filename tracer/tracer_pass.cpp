#define DEBUG_TYPE "ciri"

#include "tracer_pass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/Support/Debug.h"

#include <cxxabi.h>
#include <fstream>

using namespace ciri;
using namespace llvm;

cl::opt<std::string> TraceFilename("trace-file",
                                   cl::desc("Trace filename"),
                                   cl::init("test.trace"));

cl::opt<std::string> LockModeling("lock-modeling-file",
                                  cl::desc("Lock modeling"),
                                  cl::init("-"));

cl::opt<std::string> UnlockModeling("unlock-modeling-file",
                                    cl::desc("Unlock modeling"),
                                    cl::init("-"));

STATISTIC(NumLoads, "Number of load instructions processed");
STATISTIC(NumStores, "Number of store instructions processed");
STATISTIC(NumLoadStrings, "Number of load instructions processed");
STATISTIC(NumStoreStrings, "Number of store instructions processed");
STATISTIC(NumCalls, "Number of call instructions processed");
STATISTIC(NumInvokes, "Number of invoke instructions processed");
STATISTIC(NumExtFuns, "Number of special external calls processed, e.g. memcpy");
STATISTIC(NumFlushes, "Number of cacheline flushes instructions processed");
STATISTIC(NumFences, "Number of mfences instructions processed");

char CiriTracerPass::ID = 0;

static RegisterPass<CiriTracerPass>
X("ciri-tracer-pass", "Instrument code to trace");

bool CiriTracerPass::doInitialization(Module & M) {
  Int8Type  = IntegerType::getInt8Ty(M.getContext());
  Int32Type = IntegerType::getInt32Ty(M.getContext());
  Int64Type = IntegerType::getInt64Ty(M.getContext());
  VoidPtrType = PointerType::getUnqual(Int8Type);
  VoidType = Type::getVoidTy(M.getContext());
  OIDType = StructType::get(M.getContext(), make_vector<Type*>(Int64Type, Int64Type, 0));

  Init = M.getOrInsertFunction("recordInit",
                               VoidType,
                               VoidPtrType);

  Locking = M.getOrInsertFunction("locking",
                                   VoidType,
                                   VoidPtrType);

  Unlocking = M.getOrInsertFunction("unlocking",
                                     VoidType,
                                     VoidPtrType);

  RecordLock = M.getOrInsertFunction("recordLock",
                                     VoidType,
                                     Int32Type,
                                     VoidPtrType);

  RecordLockBegin = M.getOrInsertFunction("recordLockBegin",
                                          VoidType,
                                          Int32Type,
                                          VoidPtrType);

  RecordLockEnd = M.getOrInsertFunction("recordLockEnd",
                                        VoidType,
                                        Int32Type,
                                        VoidPtrType);

  RecordUnlock = M.getOrInsertFunction("recordUnlock",
                                       VoidType,
                                       Int32Type,
                                       VoidPtrType);

  RecordUnlockBegin = M.getOrInsertFunction("recordUnlockBegin",
                                            VoidType,
                                            Int32Type,
                                            VoidPtrType);

  RecordUnlockEnd = M.getOrInsertFunction("recordUnlockEnd",
                                          VoidType,
                                          Int32Type,
                                          VoidPtrType);

  RecordAPIBegin = M.getOrInsertFunction("recordAPIBegin",
                                         VoidType,
                                         Int32Type);

  RecordAPIEnd = M.getOrInsertFunction("recordAPIEnd",
                                       VoidType,
                                       Int32Type);

  RecordBr = M.getOrInsertFunction("recordBr",
                                   VoidType,
                                   Int32Type);

  RecordLoad = M.getOrInsertFunction("recordLoad",
                                     VoidType,
                                     Int32Type,
                                     VoidPtrType,
                                     Int64Type);

  RecordStrLoad = M.getOrInsertFunction("recordStrLoad",
                                        VoidType,
                                        Int32Type,
                                        VoidPtrType);

  RecordStore = M.getOrInsertFunction("recordStore",
                                      VoidType,
                                      Int32Type,
                                      VoidPtrType,
                                      Int64Type);

  RecordAtomicStore = M.getOrInsertFunction("recordAtomicStore",
                                            VoidType,
                                            Int32Type,
                                            VoidPtrType,
                                            Int64Type);

  RecordCAS= M.getOrInsertFunction("recordCAS",
                                      VoidType,
                                      Int32Type,
                                      VoidPtrType,
                                      Int64Type,
                                      Int64Type);

  RecordStrStore = M.getOrInsertFunction("recordStrStore",
                                         VoidType,
                                         Int32Type,
                                         VoidPtrType);

  RecordStrcatStore = M.getOrInsertFunction("recordStrcatStore",
                                            VoidType,
                                            Int32Type,
                                            VoidPtrType,
                                            VoidPtrType);

  RecordFlush = M.getOrInsertFunction("recordFlush",
                                      VoidType,
                                      Int32Type,
                                      VoidPtrType);

  RecordFlushWrapper = M.getOrInsertFunction("recordFlushWrapper",
                                             VoidType,
                                             Int32Type,
                                             VoidPtrType,
                                             Int64Type);

  RecordFence = M.getOrInsertFunction("recordFence",
                                      VoidType,
                                      Int32Type);

  RecordAlloc = M.getOrInsertFunction("recordAlloc",
                                      VoidType,
                                      Int32Type,
                                      VoidPtrType,
                                      Int64Type);

  RecordTxAlloc = M.getOrInsertFunction("recordTxAlloc",
                                      VoidType,
                                      Int32Type,
                                      OIDType,
                                      Int64Type);


  RecordMmap = M.getOrInsertFunction("recordMmap",
                                     VoidType,
                                     Int32Type,
                                     VoidPtrType,
                                     Int64Type);


  std::ifstream file0(LockModeling);
  std::string str0;
  while (std::getline(file0, str0)) {
    lock_modeling_set.insert(str0);
  }

  std::ifstream file1(UnlockModeling);
  std::string str1;
  while (std::getline(file1, str1)) {
    unlock_modeling_set.insert(str1);
  }

  return true;
}

bool CiriTracerPass::runOnModule(Module &M) {
  TD = &(M.getDataLayout());
  lsNumPass = &getAnalysis<QueryLoadStoreNumbers>();

  for (auto F = M.begin(); F != M.end(); ++F) {
    for (auto BB = (&*F)->begin(); BB != (&*F)->end(); ++BB) {
      std::vector<Instruction *> Worklist;
      for (auto I = (&*BB)->begin(); I != (&*BB)->end(); ++I) {
        Worklist.push_back(&*I);
      }
      visit(Worklist.begin(), Worklist.end());
    }
  }

  instrumentInit(M);
  instrumentAPIBegin(M);
  instrumentAPIEnd(M);
  instrumentLockModeling(M);
  instrumentUnlockModeling(M);

  return true;
}

void CiriTracerPass::instrumentInit(Module& M) {
  Function* fn = M.getFunction("main");
  assert(fn != NULL);

  BasicBlock *BB = &fn->getEntryBlock();
  Instruction *I = &*(BB->getFirstInsertionPt());
  Constant *Name = stringToGV(TraceFilename, &M);
  Name = ConstantExpr::getZExtOrBitCast(Name, VoidPtrType);
  CallInst::Create(Init, Name, "", I);
}

void CiriTracerPass::instrumentAPIBegin(Module& M) {
  Function* fn = M.getFunction("api_begin");
  if (fn == NULL) {
    fn = M.getFunction("_Z9api_beginv");
  }
  assert(fn != NULL);

  BasicBlock *BB = &fn->getEntryBlock();
  Instruction *I = &*(BB->getFirstInsertionPt());

  Value *CallID = ConstantInt::get(Int32Type, -1);
  // Create the call to the run-time to record the call instruction.
  std::vector<Value *> args=make_vector<Value *>(CallID, 0);
  CallInst::Create(RecordAPIBegin, args, "", I);
}

void CiriTracerPass::instrumentAPIEnd(Module& M) {
  Function* fn = M.getFunction("api_end");
  if (fn == NULL) {
    fn = M.getFunction("_Z7api_endv");
  }
  assert(fn != NULL);

  BasicBlock *BB = &fn->getEntryBlock();
  Instruction *I = &*(BB->getFirstInsertionPt());

  Value *CallID = ConstantInt::get(Int32Type, -1);
  // Create the call to the run-time to record the call instruction.
  std::vector<Value *> args=make_vector<Value *>(CallID, 0);
  CallInst::Create(RecordAPIEnd, args, "", I);
}

void CiriTracerPass::instrumentLockModeling(Module& M) {
  for (auto name : lock_modeling_set) {
    Function* fn = M.getFunction(name);
    if (fn == NULL) {
      continue;
    }

    BasicBlock *BB = &fn->getEntryBlock();
    Instruction *I = &*(BB->getFirstInsertionPt());
    Value *CallID = ConstantInt::get(Int32Type, -1);
    Value * Pointer = &*(fn->arg_begin());
    Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), I);
    std::vector<Value *> args = make_vector<Value *>(CallID, Pointer, 0);

    CallInst::Create(RecordLockBegin, args, "", I);
    for (auto BB = fn->begin(); BB != fn->end(); ++BB) {
      for (auto I = (&*BB)->begin(); I != (&*BB)->end(); ++I) {
        if (ReturnInst *RI = dyn_cast<ReturnInst> (&*I)) {
          CallInst::Create(RecordLockEnd, args, "", RI);
        }
      }
    }
  }
}

void CiriTracerPass::instrumentUnlockModeling(Module& M) {
  for (auto name : unlock_modeling_set) {
    Function* fn = M.getFunction(name);
    if (fn == NULL) {
      continue;
    }

    BasicBlock *BB = &fn->getEntryBlock();
    Instruction *I = &*(BB->getFirstInsertionPt());
    Value *CallID = ConstantInt::get(Int32Type, -1);
    Value * Pointer = &*(fn->arg_begin());
    Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), I);
    std::vector<Value *> args = make_vector<Value *>(CallID, Pointer, 0);

    CallInst::Create(RecordUnlockBegin, args, "", I);
    for (auto BB = fn->begin(); BB != fn->end(); ++BB) {
      for (auto I = (&*BB)->begin(); I != (&*BB)->end(); ++I) {
        if (ReturnInst *RI = dyn_cast<ReturnInst> (&*I)) {
          CallInst::Create(RecordUnlockEnd, args, "", RI);
        }
      }
    }
  }
}

void CiriTracerPass::instrumentLock(Instruction *I) {
  std::string s;
  raw_string_ostream rso(s);
  I->print(rso);
  Constant *Name = stringToGV(rso.str(),
                              I->getParent()->getParent()->getParent());
  Name = ConstantExpr::getZExtOrBitCast(Name, VoidPtrType);
  CallInst::Create(Locking, Name)->insertBefore(I);
}

void CiriTracerPass::instrumentUnlock(Instruction *I) {
  std::string s;
  raw_string_ostream rso(s);
  I->print(rso);
  Constant *Name = stringToGV(rso.str(),
                              I->getParent()->getParent()->getParent());
  Name = ConstantExpr::getZExtOrBitCast(Name, VoidPtrType);
  CallInst::Create(Unlocking, Name)->insertAfter(I);
}

void CiriTracerPass::visitBranchInst(BranchInst &BI) {
  if (BI.isUnconditional()) {
    return;
  }

  //instrumentLock(&BI);
  Value *BrID = ConstantInt::get(Int32Type, lsNumPass->getID(&BI));
  std::vector<Value *> args=make_vector<Value *>(BrID, 0);
  CallInst::Create(RecordBr, args, "", &BI);
  //instrumentUnlock(&BI);
}

void CiriTracerPass::visitLoadInst(LoadInst &LI) {
  instrumentLock(&LI);

  // Get the ID of the load instruction.
  Value *LoadID = ConstantInt::get(Int32Type, lsNumPass->getID(&LI));
  // Cast the pointer into a void pointer type.
  Value *Pointer = LI.getPointerOperand();
  Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &LI);
  // Get the size of the loaded data.
  uint64_t size = TD->getTypeStoreSize(LI.getType());
  Value *LoadSize = ConstantInt::get(Int64Type, size);
  // Create the call to the run-time to record the load instruction.
  std::vector<Value *> args=make_vector<Value *>(LoadID, Pointer, LoadSize, 0);
  CallInst::Create(RecordLoad, args, "", &LI);

  instrumentUnlock(&LI);
  ++NumLoads; // Update statistics
}

void CiriTracerPass::visitStoreInst(StoreInst &SI) {
  instrumentLock(&SI);

  // Cast the pointer into a void pointer type.
  Value * Pointer = SI.getPointerOperand();
  Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &SI);
  // Get the size of the stored data.
  uint64_t size = TD->getTypeStoreSize(SI.getOperand(0)->getType());
  Value *StoreSize = ConstantInt::get(Int64Type, size);
  // Get the ID of the store instruction.
  Value *StoreID = ConstantInt::get(Int32Type, lsNumPass->getID(&SI));
  // Create the call to the run-time to record the store instruction.
  std::vector<Value *> args=make_vector<Value *>(StoreID, Pointer, StoreSize, 0);
  CallInst *recStore = CallInst::Create(RecordStore, args, "", &SI);

  instrumentUnlock(&SI);
  // Insert RecordStore after the instruction so that we can get the value
  SI.moveBefore(recStore);
  ++NumStores; // Update statistics
}

void CiriTracerPass::visitAtomicRMWInst(AtomicRMWInst &AI) {
  instrumentLock(&AI);

  // Cast the pointer into a void pointer type.
  Value * Pointer = AI.getPointerOperand();
  Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &AI);
  // Get the size of the stored data.
  uint64_t size = TD->getTypeStoreSize(AI.getValOperand()->getType());
  Value *StoreSize = ConstantInt::get(Int64Type, size);
  // Get the ID of the store instruction.
  Value *StoreID = ConstantInt::get(Int32Type, lsNumPass->getID(&AI));
  // Create the call to the run-time to record the store instruction.
  std::vector<Value *> args=make_vector<Value *>(StoreID, Pointer, StoreSize, 0);
  CallInst *recStore = CallInst::Create(RecordAtomicStore, args, "", &AI);

  instrumentUnlock(&AI);
  // Insert RecordStore after the instruction so that we can get the value
  AI.moveBefore(recStore);
  ++NumStores; // Update statistics
}

void CiriTracerPass::visitAtomicCmpXchgInst(AtomicCmpXchgInst &AI) {
  instrumentLock(&AI);

  // Cast the pointer into a void pointer type.
  Value * Pointer = AI.getPointerOperand();
  Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &AI);

  Value * NewValOp = AI.getNewValOperand();
  NewValOp = castTo(NewValOp, Int64Type, Pointer->getName(), &AI);

  // Get the size of the stored data.
  uint64_t size = TD->getTypeStoreSize(AI.getNewValOperand()->getType());
  Value *StoreSize = ConstantInt::get(Int64Type, size);
  // Get the ID of the store instruction.
  Value *StoreID = ConstantInt::get(Int32Type, lsNumPass->getID(&AI));
  // Create the call to the run-time to record the store instruction.
  std::vector<Value *> args=make_vector<Value *>(StoreID, Pointer, NewValOp, StoreSize, 0);
  CallInst *recCAS = CallInst::Create(RecordCAS, args, "", &AI);

  instrumentUnlock(&AI);
  // Insert RecordCAS after the instruction so that we can get the value
  AI.moveBefore(recCAS);
  ++NumStores; // Update statistics
}

// TODO
void CiriTracerPass::visitInvokeInst(InvokeInst &II) { };

void CiriTracerPass::visitCallInst(CallInst &CI) {
  if (isa<InlineAsm>(CI.getCalledValue())) {
     visitInlineAsm(CI);
     return;
   }

  // Attempt to get the called function.
  Function *CalledFunc = CI.getCalledFunction();
  if (!CalledFunc) {
    return;
  }

  // Function name de-mangling for c++
  std::string func_name_mangled = CalledFunc->getName().str();
  int status = -1;
  std::unique_ptr<char, void(*)(void*)> res
          { abi::__cxa_demangle(func_name_mangled.c_str(), NULL, NULL, &status),
            std::free };
  std::string FuncName;
  if (status == 0) {
    FuncName = res.get();
    FuncName = FuncName.substr(0, FuncName.find("("));
  } else {
    FuncName = func_name_mangled;
  }

  if (FuncName == "pmemobj_alloc") {
    instrumentLock(&CI);

    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    Value * Pointer = CI.getArgOperand(1);
    Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &CI);
    Value *size = CI.getOperand(2);
    std::vector<Value *> args = make_vector(CallID, Pointer, size, 0);
    CallInst *recInst = CallInst::Create(RecordAlloc, args, "", &CI);

    instrumentUnlock(&CI);
    CI.moveBefore(recInst);
    return;
  }

  if (FuncName == "pmemobj_tx_alloc") {
    instrumentLock(&CI);

    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    Value * Pointer = &CI;
    Value *size = CI.getOperand(0);
    std::vector<Value *> args = make_vector(CallID, Pointer, size, 0);
    CallInst *recInst = CallInst::Create(RecordTxAlloc, args, "", &CI);

    instrumentUnlock(&CI);
    CI.moveBefore(recInst);
    return;
  }

  if (!CalledFunc->isDeclaration()) {
    return;
  }

  if (visitSpecialCall(CI, FuncName) ||
      visitPmemCall(CI, FuncName) ||
      visitLockCall(CI, FuncName) ||
      visitMmapCall(CI, FuncName)) {
    return;
  }
}

bool CiriTracerPass::visitInlineAsm(CallInst &CI) {
  // InlineAsm conversion
  const InlineAsm *IA = cast<InlineAsm>(CI.getCalledValue());
  // Get ASM string
  const std::string& asm_str = IA->getAsmString();

  // These strings are inferred from: RECIPE, FAIR-FAST, CCEH, Level-Hashing
  // CLFLUSH and CLFLUSHOPT
  const std::string CLFLUSH ("clflush $0");
  // CLWB
  const std::string CLWB ("xsaveopt $0");
  // MFENCE
  const std::string MFENCE ("mfence");
  // XCHGQ
  const std::string XCHGQ ("xchgq $0,$1");

  // TODO: we only use a naive substring matching for both flush and fence.
  // If it is a flush
  if (asm_str.find(CLFLUSH) != std::string::npos ||
        asm_str.find(CLWB) != std::string::npos) {
    assert(CI.getNumArgOperands() == 1 || CI.getNumArgOperands() == 2
            && "FLUSH ASM's # of args is not 1 or 2!");

    // TODO: operand format (only use op 0) is only inferred from (see above);

    // Instrument the code and add RecordFLush
    instrumentLock(&CI);
    // Cast the pointer into a void pointer type.
    Value * Pointer = CI.getArgOperand(0);
    Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &CI);
    // Get the ID of the call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the call instruction.
    std::vector<Value *> args=make_vector<Value *>(CallID, Pointer, 0);
    CallInst *recFlush = CallInst::Create(RecordFlush, args, "", &CI);
    instrumentUnlock(recFlush);

    CI.moveBefore(recFlush);

    // STATISTIC
    ++NumFlushes;
    return true;
  }

  // TODO: we only handle mfence here
  // If it is a mfence
  if (asm_str.find(MFENCE) != std::string::npos) {
    assert(CI.getNumArgOperands() == 0 && "FENCE ASM's # of args is not 0!");

    // Instrument the code and add RecordFence
    instrumentLock(&CI);
    // Get the ID of the call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the call instruction.
    std::vector<Value *> args = make_vector<Value *>(CallID, 0);
    CallInst* recFence = CallInst::Create(RecordFence, args, "", &CI);
    instrumentUnlock(recFence);

    CI.moveBefore(recFence);

    ++NumFences;
    return true;
  }

  // TODO: hard-coded for now
  // If it is a xchgq
  if (asm_str.find(XCHGQ) != std::string::npos) {
    assert(CI.getNumArgOperands() == 2 && "XCHGQ ASM's # of args is not 2!");

    instrumentLock(&CI);

    // Cast the pointer into a void pointer type.
    Value * Pointer = CI.getOperand(0);
    Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &CI);
    // Get the size of the stored data.
    uint64_t size = TD->getTypeStoreSize(CI.getOperand(0)->getType());
    Value *StoreSize = ConstantInt::get(Int64Type, size);
    // Get the ID of the store instruction.
    Value *StoreID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the store instruction.
    std::vector<Value *> args=make_vector<Value *>(StoreID, Pointer, StoreSize, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);

    instrumentUnlock(&CI);
    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);
    ++NumStores; // Update statistics

    return true;
  }

  return false;
}

bool CiriTracerPass::visitSpecialCall(CallInst &CI, std::string name) {
  // Check the name of the function against a list of known special functions.
  if (name.substr(0,12) == "llvm.memset.") {
    instrumentLock(&CI);

    // Get the destination pointer and cast it to a void pointer.
    Value *dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    // Get the number of bytes that will be written into the buffer.
    Value *NumElts = CI.getOperand(2);
    // Get the ID of the external funtion call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the external call instruction.
    std::vector<Value *> args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);

    instrumentUnlock(&CI);
    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);
    ++NumExtFuns; // Update statistics
    return true;
  } else if (name.substr(0,12) == "llvm.memcpy." ||
             name.substr(0,13) == "llvm.memmove." ||
             name == "strcpy") {
    instrumentLock(&CI);

    /* Record Load src, [CI] Load dst [CI] */
    // Get the destination and source pointers and cast them to void pointers.
    Value *dstPointer = CI.getOperand(0);
    Value *srcPointer  = CI.getOperand(1);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);
    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // Create the call to the run-time to record the loads and stores of
    // external call instruction.
    if(name == "strcpy") {
      // FIXME: If the tracer function should be inserted before or after????
      std::vector<Value *> args = make_vector(CallID, srcPointer, 0);
      CallInst::Create(RecordStrLoad, args, "", &CI);

      args = make_vector(CallID, dstPointer, 0);
      CallInst *recStore = CallInst::Create(RecordStrStore, args, "", &CI);
      instrumentUnlock(&CI);
      // Insert RecordStore after the instruction so that we can get the value
      CI.moveBefore(recStore);
    } else {
      // get the num elements to be transfered
      Value *NumElts = CI.getOperand(2);
      std::vector<Value *> args = make_vector(CallID, srcPointer, NumElts, 0);
      CallInst::Create(RecordLoad, args, "", &CI);

      args = make_vector(CallID, dstPointer, NumElts, 0);
      CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);
      instrumentUnlock(&CI);
      // Insert RecordStore after the instruction so that we can get the value
      CI.moveBefore(recStore);
    }

    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "strncpy" || name == "__strncpy_chk") {
    instrumentLock(&CI);
    /* Record Load src, [CI] Load dst [CI] */
    // Get the destination and source pointers and cast them to void pointers.
    Value *dstPointer = CI.getOperand(0);
    Value *srcPointer  = CI.getOperand(1);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);
    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    std::vector<Value *> args = make_vector(CallID, srcPointer, 0);
    CallInst::Create(RecordStrLoad, args, "", &CI);

    // TODO we assume the size is smaller than the dest size
    args = make_vector(CallID, dstPointer, 0);
    CallInst *recStore = CallInst::Create(RecordStrStore, args, "", &CI);

    instrumentUnlock(&CI);
    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);

    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "strcat") { /* Record Load dst, Load Src, Store dst-end before call inst  */
    instrumentLock(&CI);

    // Get the destination and source pointers and cast them to void pointers.
    Value *dstPointer = CI.getOperand(0);
    Value *srcPointer = CI.getOperand(1);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);

    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // Create the call to the run-time to record the loads and stores of
    // external call instruction.
    // CHECK: If the tracer function should be inserted before or after????
    std::vector<Value *> args = make_vector(CallID, dstPointer, 0);
    CallInst::Create(RecordStrLoad, args, "", &CI);

    args = make_vector(CallID, srcPointer, 0);
    CallInst::Create(RecordStrLoad, args, "", &CI);

    // Record the addresses before concat as they will be lost after concat
    args = make_vector(CallID, dstPointer, srcPointer, 0);
    CallInst *recStore = CallInst::Create(RecordStrcatStore, args, "", &CI);

    instrumentUnlock(&CI);
    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);
    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "strlen") { /* Record Load */
    instrumentLock(&CI);

    // Get the destination and source pointers and cast them to void pointers.
    Value *srcPointer  = CI.getOperand(0);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);
    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    std::vector<Value *> args = make_vector(CallID, srcPointer, 0);
    CallInst::Create(RecordStrLoad, args, "", &CI);

    instrumentUnlock(&CI);
    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "strcmp") {
    instrumentLock(&CI);

    // Get the 2 pointers and cast them to void pointers.
    Value *strCmpPtr0 = CI.getOperand(0);
    Value *strCmpPtr1 = CI.getOperand(1);
    strCmpPtr0 = castTo(strCmpPtr0, VoidPtrType, strCmpPtr0->getName(), &CI);
    strCmpPtr1 = castTo(strCmpPtr1, VoidPtrType, strCmpPtr1->getName(), &CI);

    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // Load for the first pointer
    std::vector<Value *> args = make_vector(CallID, strCmpPtr0, 0);
    CallInst::Create(RecordStrLoad, args, "", &CI);

    // Load for the second pointer
    args = make_vector(CallID, strCmpPtr1, 0);
    CallInst::Create(RecordStrLoad, args, "", &CI);

    instrumentUnlock(&CI);
    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "calloc") {
    instrumentLock(&CI);

    // Get the number of bytes that will be written into the buffer.
    Value *NumElts = BinaryOperator::Create(BinaryOperator::Mul,
                                            CI.getOperand(0),
                                            CI.getOperand(1),
                                            "calloc par1 * par2",
                                            &CI);
    // Get the destination pointer and cast it to a void pointer.
    // Instruction * dstPointerInst;
    Value *dstPointer = castTo(&CI, VoidPtrType, CI.getName(), &CI);

    /* // To move after call inst, we need to know if cast is a constant expr or inst
    if ((dstPointerInst = dyn_cast<Instruction>(dstPointer))) {
        CI.moveBefore(dstPointerInst); // dstPointerInst->insertAfter(&CI);
        // ((Instruction *)NumElts)->insertAfter(dstPointerInst);
    }
    else {
        CI.moveBefore((Instruction *)NumElts); // ((Instruction *)NumElts)->insertAfter(&CI);
	}
    dstPointer = dstPointerInst; // Assign to dstPointer for instrn or non-instrn values
    */

    // Get the ID of the external funtion call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    //
    // Create the call to the run-time to record the external call instruction.
    //
    std::vector<Value *> args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);
    instrumentUnlock(&CI);

    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore); //recStore->insertAfter((Instruction *)NumElts);
    // Moove cast, #byte computation and store to after call inst
    CI.moveBefore(cast<Instruction>(NumElts));

    ++NumExtFuns; // Update statistics
    return true;
  } else if (name == "tolower" || name == "toupper") {
    // Not needed as there are no loads and stores
  /*  } else if (name == "strncpy/itoa/stdarg/scanf/fscanf/sscanf/fread/complex/strftime/strptime/asctime/ctime") { */
  } else if (name == "fscanf") {
    // TODO
    // In stead of parsing format string, can we use the type of the arguments??
  } else if (name == "sscanf") {
    // TODO
  } else if (name == "sprintf") {
    instrumentLock(&CI);
    // Get the pointer to the destination buffer.
    Value *dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);

    // Get the ID of the call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // Scan through the arguments looking for what appears to be a character
    // string.  Generate load records for each of these strings.
    for (unsigned index = 2; index < CI.getNumOperands(); ++index) {
      if (CI.getOperand(index)->getType() == VoidPtrType) {
        // Create the call to the run-time to record the load from the string.
        // What about other loads??
        Value *Ptr = CI.getOperand(index);
        std::vector<Value *> args = make_vector(CallID, Ptr, 0);
        CallInst::Create(RecordStrLoad, args, "", &CI);

        ++NumLoadStrings; // Update statistics
      }
    }

    // Create the call to the run-time to record the external call instruction.
    std::vector<Value *> args = make_vector(CallID, dstPointer, 0);
    CallInst *recStore = CallInst::Create(RecordStrStore, args, "", &CI);

    instrumentUnlock(&CI);
    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);
    ++NumStoreStrings; // Update statistics
    return true;
  } else if (name == "fgets") {
    instrumentLock(&CI);

    // Get the pointer to the destination buffer.
    Value * dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);

    // Get the ID of the ext fun call instruction.
    Value * CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // Create the call to the run-time to record the external call instruction.
    std::vector<Value *> args = make_vector(CallID, dstPointer, 0);
    CallInst *recStore = CallInst::Create(RecordStrStore, args, "", &CI);

    instrumentUnlock(&CI);
    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);
    // Update statistics
    ++NumStoreStrings;
    return true;
  } else if (name == "snprintf") {
    instrumentLock(&CI);

    // Get the destination pointer and cast it to a void pointer.
    Value *dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    // Get the number of bytes that will be written into the buffer.
    Value *NumElts = CI.getOperand(1);
    // Get the ID of the external funtion call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // record store
    std::vector<Value *> args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);

    instrumentUnlock(&CI);

    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);

    ++NumStores;
    return true;
  }

  return false;

}

bool CiriTracerPass::visitPmemCall(CallInst &CI, std::string name) {
  if (name == "pmem_flush") {
    // Instrument the code and add RecordFLush
    instrumentLock(&CI);
    // Cast the pointer into a void pointer type.
    Value * Pointer = CI.getArgOperand(0);
    Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &CI);
    // Get the number of bytes that will be flushed.
    Value *NumElts = CI.getOperand(1);
    // Get the ID of the call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the call instruction.
    std::vector<Value *> args=make_vector<Value *>(CallID, Pointer, NumElts, 0);
    CallInst *recFlush = CallInst::Create(RecordFlushWrapper, args, "", &CI);
    instrumentUnlock(&CI);

    CI.moveBefore(recFlush);

    ++NumFlushes;
    return true;
  }

  if (name == "pmem_drain") {
    // Instrument the code and add RecordFence
    instrumentLock(&CI);
    // Get the ID of the call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the call instruction.
    std::vector<Value *> args = make_vector<Value *>(CallID, 0);
    CallInst* recFence = CallInst::Create(RecordFence, args, "", &CI);
    instrumentUnlock(&CI);

    CI.moveBefore(recFence);

    ++NumFences;
    return true;
  }

  if (name == "pmem_persist" || name == "pmem_msync") {
    // Instrument the code and add RecordFLush
    instrumentLock(&CI);
    // Cast the pointer into a void pointer type.
    Value * Pointer = CI.getArgOperand(0);
    Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &CI);
    // Get the number of bytes that will be flushed.
    Value *NumElts = CI.getOperand(1);
    // Get the ID of the call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the call instruction.
    std::vector<Value *> args=make_vector<Value *>(CallID, Pointer, NumElts, 0);
    CallInst *recFlush = CallInst::Create(RecordFlushWrapper, args, "", &CI);
    args = make_vector<Value *>(CallID, 0);
    CallInst* recFence = CallInst::Create(RecordFence, args, "", &CI);

    instrumentUnlock(&CI);

    CI.moveBefore(recFlush);

    ++NumFlushes;
    ++NumFences;
    return true;

  }

  if (name == "pmem_memmove_nodrain" || name == "pmem_memcpy_nodrain") {
    instrumentLock(&CI);

    // Get two pointers
    Value *dstPointer = CI.getOperand(0);
    Value *srcPointer  = CI.getOperand(1);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);
    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // get the num elements to be transfered
    Value *NumElts = CI.getOperand(2);

    // record load
    std::vector<Value *> args = make_vector(CallID, srcPointer, NumElts, 0);
    CallInst *recLoad = CallInst::Create(RecordLoad, args, "", &CI);

    // record store
    args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);

    // record flush
    args = make_vector<Value *>(CallID, dstPointer, NumElts, 0);
    CallInst *recFlush = CallInst::Create(RecordFlushWrapper, args, "", &CI);

    instrumentUnlock(&CI);

    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);

    ++NumLoads;
    ++NumStores;
    ++NumFlushes;
    return true;
  }

  if (name == "pmem_memmove_persist" || name == "pmem_memcpy_persist") {
    instrumentLock(&CI);

    // Get two pointers
    Value *dstPointer = CI.getOperand(0);
    Value *srcPointer  = CI.getOperand(1);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);
    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // get the num elements to be transfered
    Value *NumElts = CI.getOperand(2);

    // record load
    std::vector<Value *> args = make_vector(CallID, srcPointer, NumElts, 0);
    CallInst *recLoad = CallInst::Create(RecordLoad, args, "", &CI);

    // record store
    args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);

    // record flush
    args = make_vector<Value *>(CallID, dstPointer, NumElts, 0);
    CallInst *recFlush = CallInst::Create(RecordFlushWrapper, args, "", &CI);

    // record fence
    args = make_vector<Value *>(CallID, 0);
    CallInst* recFence = CallInst::Create(RecordFence, args, "", &CI);

    instrumentUnlock(&CI);

    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);

    ++NumLoads;
    ++NumStores;
    ++NumFlushes;
    ++NumFences;
    return true;
  }

  if (name == "pmem_memmove" || name == "pmem_memcpy") {
    instrumentLock(&CI);

    // Get two pointers
    Value *dstPointer = CI.getOperand(0);
    Value *srcPointer  = CI.getOperand(1);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    srcPointer  = castTo(srcPointer,  VoidPtrType, srcPointer->getName(), &CI);
    // Get the ID of the ext fun call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // get the num elements to be transfered
    Value *NumElts = CI.getOperand(2);

    // record load
    std::vector<Value *> args = make_vector(CallID, srcPointer, NumElts, 0);
    CallInst *recLoad = CallInst::Create(RecordLoad, args, "", &CI);

    // record store
    args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);

    instrumentUnlock(&CI);

    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);

    ++NumLoads;
    ++NumStores;
    return true;
  }

  if (name == "pmem_memset_nodrain") {
    instrumentLock(&CI);

    // Get the destination pointer and cast it to a void pointer.
    Value *dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    // Get the number of bytes that will be written into the buffer.
    Value *NumElts = CI.getOperand(2);
    // Get the ID of the external funtion call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // record store
    std::vector<Value *> args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);

    // record flush
    args = make_vector<Value *>(CallID, dstPointer, NumElts, 0);
    CallInst *recFlush = CallInst::Create(RecordFlushWrapper, args, "", &CI);

    instrumentUnlock(&CI);

    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);

    ++NumStores;
    ++NumFlushes;
    return true;
  }

  if (name == "pmem_memset_persist") {
    instrumentLock(&CI);

    // Get the destination pointer and cast it to a void pointer.
    Value *dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    // Get the number of bytes that will be written into the buffer.
    Value *NumElts = CI.getOperand(2);
    // Get the ID of the external funtion call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // record store
    std::vector<Value *> args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);

    // record flush
    args = make_vector<Value *>(CallID, dstPointer, NumElts, 0);
    CallInst *recFlush = CallInst::Create(RecordFlushWrapper, args, "", &CI);

    // record fence
    args = make_vector<Value *>(CallID, 0);
    CallInst* recFence = CallInst::Create(RecordFence, args, "", &CI);

    instrumentUnlock(&CI);

    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);

    ++NumStores;
    ++NumFlushes;
    ++NumFences;
    return true;
  }

  if (name == "pmem_memset") {
    instrumentLock(&CI);

    // Get the destination pointer and cast it to a void pointer.
    Value *dstPointer = CI.getOperand(0);
    dstPointer = castTo(dstPointer, VoidPtrType, dstPointer->getName(), &CI);
    // Get the number of bytes that will be written into the buffer.
    Value *NumElts = CI.getOperand(2);
    // Get the ID of the external funtion call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));

    // record store
    std::vector<Value *> args = make_vector(CallID, dstPointer, NumElts, 0);
    CallInst *recStore = CallInst::Create(RecordStore, args, "", &CI);

    instrumentUnlock(&CI);

    // Insert RecordStore after the instruction so that we can get the value
    CI.moveBefore(recStore);

    ++NumStores;
    return true;
  }

  return false;
}

bool CiriTracerPass::visitLockCall(CallInst &CI, std::string name) {
  if (name == "pthread_rwlock_wrlock" ||
      name == "pthread_rwlock_trywrlock" ||
      name == "pthread_rwlock_timedwrlock" ||
      name == "pthread_rwlock_rdlock" ||
      name == "pthread_rwlock_tryrdlock" ||
      name == "pthread_rwlock_timedrdlock" ||
      name == "pthread_mutex_lock" ||
      name == "pthread_mutex_trylock" ||
      name == "pthread_mutex_timedlock" ||
      name == "pthread_spin_lock" ||
      name == "pthread_spin_trylock") {
      // name == "std::mutex::lock") {
    // Instrument the code and add RecordFLush
    // instrumentLock(&CI);
    // Cast the pointer into a void pointer type.
    Value * Pointer = CI.getArgOperand(0);
    Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &CI);
    // Get the ID of the call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the call instruction.
    std::vector<Value *> args=make_vector<Value *>(CallID, Pointer, 0);
    CallInst *recLock = CallInst::Create(RecordLock, args, "", &CI);
    // instrumentUnlock(&CI);

    CI.moveBefore(recLock);

    return true;
  }

  if (name == "pthread_rwlock_unlock" ||
      name == "pthread_mutex_unlock" ||
      name == "pthread_spin_unlock") {
      // name == "std::mutex::unlock") {
    // Instrument the code and add RecordFLush
    // instrumentLock(&CI);
    // Cast the pointer into a void pointer type.
    Value * Pointer = CI.getArgOperand(0);
    Pointer = castTo(Pointer, VoidPtrType, Pointer->getName(), &CI);
    // Get the ID of the call instruction.
    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    // Create the call to the run-time to record the call instruction.
    std::vector<Value *> args=make_vector<Value *>(CallID, Pointer, 0);
    CallInst *recUnlock = CallInst::Create(RecordUnlock, args, "", &CI);
    // instrumentUnlock(&CI);

    CI.moveBefore(recUnlock);

    return true;
  }

  return false;
}

bool CiriTracerPass::visitMmapCall(CallInst &CI, std::string name) {
  if (name == "mmap") {
    instrumentLock(&CI);

    Value *CallID = ConstantInt::get(Int32Type, lsNumPass->getID(&CI));
    Value *ptr = &CI;
    Value *size = CI.getOperand(1);
    std::vector<Value *> args = make_vector(CallID, ptr, size, 0);
    CallInst *recInst = CallInst::Create(RecordMmap, args, "", &CI);

    instrumentUnlock(&CI);
    CI.moveBefore(recInst);
    return true;
  }

  return false;
}
