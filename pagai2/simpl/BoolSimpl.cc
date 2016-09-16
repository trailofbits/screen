#include "BoolSimpl.h"
#include <iostream>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;

void BoolSimpl::visitAnd(BinaryOperator &I) {
  Value *op0 = I.getOperand(0), *op1 = I.getOperand(1);
  if (CastInst *cop0 = dyn_cast<CastInst>(op0))
    if (CastInst *cop1 = dyn_cast<CastInst>(op1))
      if (cop0 -> isIntegerCast() &&
	  cop1 -> isIntegerCast() &&
	  cop0 -> getSrcTy() == cop1 -> getSrcTy()) {
	std::cerr << "replace AND" << std::endl;
	ReplaceInstWithInst(&I,
	  CastInst::CreateIntegerCast(
	    BinaryOperator::Create(BinaryOperator::And,
	      cop0 -> getOperand(0),
	      cop1 -> getOperand(0),
	      "andb", &I),
	    cop0 -> getDestTy(),
            false,
            "postcast"));
      }
}

void BoolSimpl::visitOr(BinaryOperator &I) {
  Value *op0 = I.getOperand(0), *op1 = I.getOperand(1);
  if (CastInst *cop0 = dyn_cast<CastInst>(op0))
    if (CastInst *cop1 = dyn_cast<CastInst>(op1))
      if (cop0 -> isIntegerCast() &&
	  cop1 -> isIntegerCast() &&
	  cop0 -> getSrcTy() == cop1 -> getSrcTy()) {
	std::cerr << "replace OR" << std::endl;
	ReplaceInstWithInst(&I,
	  CastInst::CreateIntegerCast(
	    BinaryOperator::Create(BinaryOperator::Or,
	      cop0 -> getOperand(0),
	      cop1 -> getOperand(0),
	      "orb", &I),
	    cop0 -> getDestTy(),
            false,
            "postcast"));
      }
}

void BoolSimpl::visitXor(BinaryOperator &I) {
  Value *op0 = I.getOperand(0), *op1 = I.getOperand(1);
  if (CastInst *cop0 = dyn_cast<CastInst>(op0))
    if (CastInst *cop1 = dyn_cast<CastInst>(op1))
      if (cop0 -> isIntegerCast() &&
	  cop1 -> isIntegerCast() &&
	  cop0 -> getSrcTy() == cop1 -> getSrcTy()) {
	std::cerr << "replace XOR" << std::endl;
	ReplaceInstWithInst(&I,
	  CastInst::CreateIntegerCast(
	    BinaryOperator::Create(BinaryOperator::Xor,
	      cop0 -> getOperand(0),
	      cop1 -> getOperand(0),
	      "xorb", &I),
	    cop0 -> getDestTy(),
            false,
            "postcast"));
      }
}

void BoolSimpl::visitICmpInst(ICmpInst &I) {
  Value *op0 = I.getOperand(0), *op1 = I.getOperand(1);
  if (CastInst *cop0 = dyn_cast<CastInst>(op0))
    if (ConstantInt *cop1 = dyn_cast<ConstantInt>(op1))
      if (cop0 -> getSrcTy() -> isIntegerTy(1) &&
	  cop1 -> isZero())
	if (Instruction *src  = dyn_cast<Instruction>(cop0 -> getOperand(0)))
	  switch (I.getPredicate()) {
	  case ICmpInst::ICMP_NE:
	    ReplaceInstWithInst(&I, src -> clone());
	    std::cerr << "replace NE" << std::endl;
	    break;
	  }
}

bool BoolSimpl::runOnBasicBlock(BasicBlock &bb) {
  visit(bb);
  return false;
}

char BoolSimpl::ID = 0;
static RegisterPass<BoolSimpl> X("BoolSimpl", "Simplify Boolean expressions", false, false);
