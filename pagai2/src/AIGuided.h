/**
 * \file AIGuided.h
 * \brief Declaration of the AIGuided pass (Guided Static Analysis)
 * \author Julien Henry
 */
#ifndef _AIGUIDED_H
#define _AIGUIDED_H

#include <queue>
#include <vector>

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CFG.h"

#include "AIpass.h"

using namespace llvm;

/**
 * \class AIGuided
 * \brief Implements the Guided Static Analysis algorithm.
 *
 * Implements Guided Static Analysis from Gopan and Reps, SAS 07.
 * Computes the fixpoint iterations over an ascending sequence of subsets of
 * transitions of the CFG. The subset in stored in a BDD using the PathTree
 * class.
 */
class AIGuided : public ModulePass, public AIPass {

	private:
		/**
		 * \brief remembers all the paths that have already been
		 * visited
		 */
		std::map<BasicBlock*,std::set<BasicBlock* > * > pathtree;

		/**
		 * \brief Temporary Pathtree that stores the transitions that have been visited
		 * in the same computeNode call
		 */
		PathTree * W;

		/**
		 * \brief Set of control points that have to be added in the working set at
		 * next iteration
		 */
		std::priority_queue<Node*,std::vector<Node*>,NodeCompare> A_prime;

		/**
		 * \brief Compute the new feasible transition
		 * \param n the starting point of the transitions
		 */
		void computeNewPaths(Node * n);

		void init()
			{
				aman = new AbstractManClassic();
				passID.T = GUIDED;
			}

	public:
		static char ID;	

		AIGuided(char &_ID, Apron_Manager_Type _man, bool _NewNarrow, bool _Threshold) : ModulePass(_ID), AIPass(_man,_NewNarrow, _Threshold) {
			init();
			passID.D = _man;
			passID.N = _NewNarrow;
			passID.TH = _Threshold;
		}
		
		AIGuided() : ModulePass(ID) {
			init();
			passID.D = getApronManager();
			passID.N = useNewNarrowing();
			passID.TH = useThreshold();
		}

		~AIGuided () {
			for (std::map<BasicBlock*,std::set<BasicBlock*>*>::iterator 
				it = pathtree.begin(),
				et = pathtree.end(); 
				it != et; 
				it++) {
				if ((*it).second != NULL)
					delete (*it).second;
				}
			}

		const char *getPassName() const;

		void getAnalysisUsage(AnalysisUsage &AU) const;

		bool runOnModule(Module &M);

		void computeFunction(Function * F);

		std::set<BasicBlock*> getPredecessors(BasicBlock * b) const;
		std::set<BasicBlock*> getSuccessors(BasicBlock * b) const;

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
};

#endif
