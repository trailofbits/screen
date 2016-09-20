/**
 * \file AISimple.h
 * \brief Declaration of the AISimple pass
 * \author Julien Henry
 */
#ifndef _AISIMPLE_H
#define _AISIMPLE_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "AIpass.h"

using namespace llvm;

/**
 * \class AISimple
 * \brief Base class implementing the basic abstract interpretation algorithm
 *
 * This class contains the parts of the algorimth common to
 * algorithms that do not use SMTpass solving to chose the order in which
 * nodes are visited (i.e. Classic, and Gopan&Reps).
 */
class AISimple : public ModulePass, public AIPass {

	public:

		AISimple (char &_ID, Apron_Manager_Type _man, bool _NewNarrow, bool _Threshold) : ModulePass(_ID), AIPass(_man,_NewNarrow, _Threshold) {}

		AISimple (char & ID) : ModulePass(ID) {}

		~AISimple () {}
		/**
		 * \brief Apply the simple abstract interpretation algorithm
		 * (ascending iterations + narrowing) on function F.
		 */
		void computeFunc(Function * F);

		std::set<BasicBlock*> getPredecessors(BasicBlock * b) const;
		std::set<BasicBlock*> getSuccessors(BasicBlock * b) const;

		/**
		 * \{
		 * \name LLVM pass manager stuff
		 */
		virtual const char *getPassName() const = 0;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		/**
		 * \}
		 */

		bool runOnModule(Module &M);

		/**
		 * \brief compute and update the Abstract value of the Node n
		 * \param n the starting point
		 */
		void computeNode(Node * n);
		
		/**
		 * \brief apply narrowing at node n
		 * \param n the starting point
		 */
		void narrowNode(Node * n);

		void computeFunction(Function * F);
};

#endif
