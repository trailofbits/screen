#include <assert.h>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/IRBuilder.h"

#include "expandequalities.h"

using namespace llvm;

bool ExpandEqualities::runOnFunction(Function &F) {

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		visit(*i);
	}
	return 0;
}

/// SplitBlockAndInsertIfThen - Split the containing block at the
/// specified instruction - everything before and including SplitBefore
/// in the old basic block, and everything after SplitBefore is moved to
/// new block. The two blocks are connected by a conditional branch
/// (with value of Cmp being the condition).
/// Before:
///   Head
///   SplitBefore
///   Tail
/// After:
///   Head
///   if (Cond)
///     ThenBlock
///   SplitBefore
///   Tail
///
/// If Unreachable is true, then ThenBlock ends with
/// UnreachableInst, otherwise it branches to Tail.
/// Returns the NewBasicBlock's terminator.

TerminatorInst *ExpandEqualities::SplitBlockAndInsertIfThen(Value *Cond,
		Instruction *SplitBefore,
		bool Unreachable,
		MDNode *BranchWeights) {
	BasicBlock *Head = SplitBefore->getParent();
	BasicBlock *Tail = Head->splitBasicBlock(SplitBefore);
	TerminatorInst *HeadOldTerm = Head->getTerminator();
	LLVMContext &C = Head->getContext();
	BasicBlock *ThenBlock = BasicBlock::Create(C, "", Head->getParent(), Tail);
	TerminatorInst *CheckTerm;
	if (Unreachable)
		CheckTerm = new UnreachableInst(C, ThenBlock);
	else
		CheckTerm = BranchInst::Create(Tail, ThenBlock);
	CheckTerm->setDebugLoc(SplitBefore->getDebugLoc());
	BranchInst *HeadNewTerm =
		BranchInst::Create(/*ifTrue*/ThenBlock, /*ifFalse*/Tail, Cond);
	HeadNewTerm->setDebugLoc(SplitBefore->getDebugLoc());
	HeadNewTerm->setMetadata(LLVMContext::MD_prof, BranchWeights);
	ReplaceInstWithInst(HeadOldTerm, HeadNewTerm);
	return CheckTerm;
}

void ExpandEqualities::visitBranchInst(BranchInst &I) {
	if (seen.count(&I)) return;
	seen.insert(&I);
	if (I.isUnconditional()) return;
	CmpInst * cond;
	if (!(cond = dyn_cast<CmpInst>(I.getCondition())))
		return;
	BasicBlock * eqblock;
	BasicBlock * neblock;
	Value * leftop = cond->getOperand(0);
	Value * rightop = cond->getOperand(1);
	bool isFloat = false;
	switch (cond->getPredicate()) {
		case CmpInst::FCMP_OEQ:
			isFloat = true;
		case CmpInst::ICMP_EQ:
			eqblock = I.getSuccessor(0);
			neblock = I.getSuccessor(1);
			break;
		case CmpInst::FCMP_ONE:
			isFloat = true;
		case CmpInst::ICMP_NE:
			eqblock = I.getSuccessor(1);
			neblock = I.getSuccessor(0);
			break;
		default:
			return;
	}
	// we create the structure:
	// if leftop < rightop then
	// ...
	// else if leftop > rightop then
	// ...
	IRBuilder<> Builder(I.getContext());
	Builder.SetInsertPoint(&I);
	Value * cmpSLT;
	Value * cmpSGT;
	if (isFloat) {
		cmpSLT = Builder.CreateFCmpOLT(leftop,rightop);
		cmpSGT = Builder.CreateFCmpOGT(leftop,rightop);
	} else {
		cmpSLT = Builder.CreateICmpSLT(leftop,rightop);
		cmpSGT = Builder.CreateICmpSGT(leftop,rightop);
	}
	SplitBlockAndInsertIfThen(
			cmpSLT, 
			cond->getParent()->getTerminator(),
			false
			);
	SplitBlockAndInsertIfThen(
			cmpSGT, 
			cond->getParent()->getTerminator(),
			false
			);
}

char ExpandEqualities::ID = 0;
static RegisterPass<ExpandEqualities> X("expandequalities", "Expand = and != to < and > ", false, false);
