#include <assert.h>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "llvm/IR/IRBuilder.h"


#include "globaltolocal.h"
#include "Analyzer.h"

using namespace llvm;

bool GlobalToLocal::hasOnlyOneFunction(Module &M) {
	int N = 0;
	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		Function * F = &*mIt;
		// if the function is only a declaration, skip
		if (F->begin() == F->end()) continue;
		N++;
	}
	return (N <= 1);
}
  
bool GlobalToLocal::replaceAllUsesInFunction(Function * F, Value * oldvalue, Value * newvalue) {

	for (Function::iterator Fit = F->begin(), Fet = F->end(); Fit != Fet; Fit++) {
		BasicBlock * b = &*Fit;
		for (BasicBlock::iterator it = b->begin(), et = b->end(); it != et; it++) {
			Instruction * I = &*it;
			I->replaceUsesOfWith(oldvalue,newvalue);
		}
	}
	return true;
}

bool GlobalToLocal::runOnModule(Module &M) {

	// apply transformation only if there is one single function
	// otherwise semantics would not be preserved
	if (!SVComp() && !hasOnlyOneFunction(M)) return 0;

	ilist_iterator<llvm::GlobalVariable> it = M.global_begin(), et = M.global_end();
	for (; it != et; it++) {
		GlobalVariable * global = &*it;
		Constant * init = NULL;
		Type * ty = global->getType()->getElementType();
		// we only internalize integers, floats and Booleans
		if (ty->isIntegerTy() || ty->isFloatingPointTy()) {
			if (global->hasInitializer()) {
				init = global->getInitializer();
			}
			for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i) {
				if (i->begin() == i->end()) continue;
				BasicBlock * entry = &*i->begin();
				IRBuilder<> Builder(entry->getContext());
				Builder.SetInsertPoint(&*entry->begin());
				std::string name(global->getName());
				AllocaInst * A = Builder.CreateAlloca(ty,NULL,Twine(name));
				if (init) {
					Builder.CreateStore(init,global,false);
				}
				//we replace only inside the function
				replaceAllUsesInFunction(&*i,global,A);
				//global->replaceAllUsesWith(A);
				if (!init) {
					LoadInst * load = Builder.CreateLoad(global,false);
					Builder.CreateStore(load,A);
				}
			}
		}
	}
	return 0;
}

char GlobalToLocal::ID = 0;
static RegisterPass<GlobalToLocal> X("globaltolocal", "optimisation Pass", false, false);
