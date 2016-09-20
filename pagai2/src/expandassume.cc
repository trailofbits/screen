#include <assert.h>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/IRBuilder.h"

#include "expandassume.h"

using namespace llvm;

bool ExpandAssume::runOnFunction(Function &F) {
	while (stepFunction(F)) {};
	return 0;
}

bool ExpandAssume::stepFunction(Function &F) {
	for (Function::iterator bi = F.begin(), be = F.end(); bi != be; ++bi) {
		for (BasicBlock::iterator i = bi->begin(), e = bi->end(); i != e; ++i) {
			if (CallInst * CI = dyn_cast<CallInst>(i)) {
				// cannot use the pattern visitor here, as the graph is modified on the fly
				if (visitCallInst(*CI)) return true;
			}
		}
	}
	return false;
}

bool ExpandAssume::visitCallInst(CallInst &CI) {

	const std::string SVcomp_assume ("__VERIFIER_assume");
	const std::string assume ("assume");
	const std::string pagai_assume ("pagai_assume");

	Function * F = CI.getCalledFunction();
	if (F == NULL) return false;
	std::string fname = F->getName();
	if (fname.compare(SVcomp_assume)
		&& fname.compare(assume)
		&& fname.compare(pagai_assume)) 
		return false;
	Value * cond = CI.getOperand(0);
	if (seen.count(cond)) return false;
	seen.insert(cond);
	if (ZExtInst * zext = dyn_cast<ZExtInst>(cond)) {
		cond = zext->getOperand(0);
	} else {
		return false;
	}
	SplitBlockAndInsertIfThen(cond,&CI,true);
	return true;
}

TerminatorInst *ExpandAssume::SplitBlockAndInsertIfThen(Value *Cond,
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
		BranchInst::Create(/*ifTrue*/Tail, /*ifFalse*/ThenBlock, Cond);
		//BranchInst::Create(/*ifTrue*/ThenBlock, /*ifFalse*/Tail, Cond);
	HeadNewTerm->setDebugLoc(SplitBefore->getDebugLoc());
	HeadNewTerm->setMetadata(LLVMContext::MD_prof, BranchWeights);
	ReplaceInstWithInst(HeadOldTerm, HeadNewTerm);
	return CheckTerm;
}

char ExpandAssume::ID = 0;
static RegisterPass<ExpandAssume> X("expandassume", "handles VERIFIER_assume and VERIFIER_assert function calls", false, false);
