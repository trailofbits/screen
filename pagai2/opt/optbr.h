#include "llvm/Config/config.h"
#include "llvm/Support/CFG.h"
#include "llvm/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"


using namespace llvm;

class Optbr : public BasicBlockPass,
  public InstVisitor<Optbr, void> {
 public:
  static char ID;
  Optbr() : BasicBlockPass(ID) {}

  void visitBranchInst(BranchInst &I);

  bool runOnBasicBlock(BasicBlock &BB);
};

