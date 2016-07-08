#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {

	struct screen : public FunctionPass {
		screen() : FunctionPass(ID) { }

		static char ID; 

		virtual const char *getPassName() const 
		{
			return "screen_pass";
		}

		virtual bool runOnFunction(Function &F) 
		{
			// runOnFunction is run on every function in each compilation unit
			
			for (BasicBlock &B: F)
			{
				// Within each function, iterate over each basic block

				for (Instruction &I: B)
				{
					// Within each basic block, iterate over each instruction
					I.dump();	
				}
			}

			return true;
		}

	};

}


char screen::ID = 0;
static RegisterPass<screen> X("screen", "screen", false, false);

