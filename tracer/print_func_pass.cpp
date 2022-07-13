#include "llvm/Pass.h"
#include "llvm/IR/Function.h"

#include <cxxabi.h>
#include <iostream>

using namespace llvm;

class PrintFuncPass: public FunctionPass {

public:
  static char ID;
  PrintFuncPass() : FunctionPass(ID) {}

  virtual bool runOnFunction(Function &F);
};

char PrintFuncPass::ID = 0;

static RegisterPass<PrintFuncPass>
X("print-func-pass", "print function names");

bool PrintFuncPass::runOnFunction(Function &F) {
  std::string func_name_mangled = F.getName().str();
  int status = -1;
  std::unique_ptr<char, void(*)(void*)> res
          { abi::__cxa_demangle(func_name_mangled.c_str(), NULL, NULL, &status),
            std::free };
  std::string func_name_demangled_0;
  std::string func_name_demangled_1;
  if (status == 0) {
    func_name_demangled_0 = res.get();
    func_name_demangled_1= func_name_demangled_0.substr(0, func_name_demangled_0.find("("));
  } else {
    func_name_demangled_0 = func_name_mangled;
    func_name_demangled_1 = func_name_mangled;
  }

  std::cout << func_name_mangled << " " << func_name_demangled_0 << " " << func_name_demangled_1 << std::endl;
  return false;
}
