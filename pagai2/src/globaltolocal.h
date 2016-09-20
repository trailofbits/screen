#ifndef GLOBALTOLOCAL_H
#define GLOBALTOLOCAL_H
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"


using namespace llvm;

class GlobalToLocal : public ModulePass {
 public:
  static char ID;
  GlobalToLocal() : ModulePass(ID) {}

  bool runOnModule(Module &M);
	
  bool hasOnlyOneFunction(Module &M);

  bool replaceAllUsesInFunction(Function * F, Value * oldvalue, Value * newvalue);
};

#endif
