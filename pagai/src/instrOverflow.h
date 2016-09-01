/**
 * \file instrOverflow.h
 * \brief LLVM pass that instruments LLVM IR for overflow checking
 * \author Julien Henry
 */
#ifndef _INSTROVERFLOW_H
#define _INSTROVERFLOW_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"

#include <map>

using namespace llvm;

class instrOverflow : public FunctionPass,
	public InstVisitor<instrOverflow, bool> {

		private:
			std::map<CallInst*, Instruction*> replaced;

			bool pass1;

		public:
			static char ID;
			instrOverflow() : FunctionPass(ID) {}
			void getAnalysisUsage(AnalysisUsage &AU) const;

			bool runOnFunction(Function &F);
			bool updateFunction(Function &F);

			void replaceWithUsualOp(
					Instruction * old, 
					unsigned intrinsicID,
					std::vector<Value*> * args,
					CallInst * intrinsicCall
					);

			void replaceWithCmp(
					Instruction * old, 
					unsigned intrinsicID,
					CallInst * intrinsicCall
					);

			bool visitExtractValueInst(ExtractValueInst &inst);
			bool visitBranchInst(BranchInst &inst);
			bool visitCallInst(CallInst &inst);

			bool visitInstruction(Instruction &inst) {return false;}
	};


#endif
