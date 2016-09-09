#ifndef GLOBALTOLOCAL_H
#define GLOBALTOLOCAL_H
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"

#include "llvm/Analysis/LoopInfo.h"

#include "IdentifyLoops.h"

using namespace llvm;

std::set<BasicBlock *> Loop_headers;

char IdentifyLoops::ID = 0;

void IdentifyLoops::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<LoopInfoWrapperPass>();
	AU.setPreservesAll();
}

bool IdentifyLoops::runOnFunction(Function &F) {
	LoopInfo * LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

	for (Function::iterator it = F.begin(), et = F.end(); it != et; it++) {
		BasicBlock * b = &*it;
		if (LI->isLoopHeader(b)) {
			Loop_headers.insert(b);
		}
	}
	return false;
}



static RegisterPass<IdentifyLoops>
X("identifyloops", "search for loop headers", false, true);
#endif
