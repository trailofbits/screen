#include <assert.h>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/IRBuilder.h"

#include "NameAllValues.h"

using namespace llvm;

bool NameAllValues::runOnFunction(Function &F) {

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		BasicBlock * b = &*i;
		if (!b->hasName()) {
			Twine name("basicblock");
			b->setName(name);
		}
		for (BasicBlock::iterator ib = b->begin(), eb = b->end(); ib != eb; ++ib) {
			Instruction * I = &*ib;
			// this is not mandatory, but easier to read
			if (!I->hasName() && !(I->getType()->isVoidTy())) {
				Twine name("i");
				I->setName(name);
			}
		}
	}
	return 0;
}

char NameAllValues::ID = 0;
static RegisterPass<NameAllValues> X("nameallvalues", "give a name to every value", false, false);

