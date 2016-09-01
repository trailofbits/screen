#ifndef IDENTIFYLOOPS_H
#define IDENTIFYLOOPS_H
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"

#include <set>

using namespace llvm;

extern std::set<BasicBlock *> Loop_headers;

class IdentifyLoops : public FunctionPass {
 public:
  static char ID;
  IdentifyLoops() : FunctionPass(ID) {}

  bool runOnFunction(Function &F);
		
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	
};

#endif

