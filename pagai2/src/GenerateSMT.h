/**
 * \file GenerateSMT.h
 * \brief Declaration of the GenerateSMT class
 * \author Julien Henry
 */
#ifndef GENERATE_SMT_H
#define GENERATE_SMT_H


#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CFG.h"

#include "SMT_manager.h"
#include "SMTpass.h"

using namespace llvm;

/**
 * \class GenerateSMT
 * \brief Pass that only computes the SMT-formula and outputs it
 */
class GenerateSMT : public ModulePass {
	
	private:
		SMTpass * LSMT;
		DominatorTree * DT;

	public:
		static char ID;

		GenerateSMT();
		~GenerateSMT();

		const char * getPassName() const;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool runOnModule(Module &M);
		bool runOnFunction(Function &F);

		void printBasicBlock(BasicBlock* b);

};
#endif
