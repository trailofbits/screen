#include "llvm/Config/config.h"
#include "llvm/Support/CFG.h"
#include "llvm/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"


using namespace llvm;

class BoolSimpl : public BasicBlockPass,
  public InstVisitor<BoolSimpl, void> {
 public:
  static char ID;
  BoolSimpl() : BasicBlockPass(ID) {}

  void visitICmpInst(ICmpInst &I);
  void visitAnd(BinaryOperator &I);
  void visitOr(BinaryOperator &I);
  void visitXor(BinaryOperator &I);

  virtual bool runOnBasicBlock(BasicBlock &F);
};
