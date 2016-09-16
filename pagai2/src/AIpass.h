/**
 * \file AIpass.h
 * \brief Declaration of the AIpass class
 * \author Julien Henry
 */
#ifndef _AIPASS_H
#define _AIPASS_H

#include <queue>
#include <vector>

#include "llvm/Config/llvm-config.h"
#include "llvm/Support/FormattedStream.h"

#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"

#include "Analyzer.h"
#include "apron.h"
#include "PathTree.h"
#include "Constraint.h"
#include "AbstractMan.h"
#include "AnalysisPass.h"

using namespace llvm;

class SMTpass;
class Live;
class Node;

/**
 * \class AIPass
 * \brief Base class for abstract interpretation
 *
 * Base class factoring helper functions and data-structure to
 * perform Abstract Interpretation (i.e. graph traversal on the CFG,
 * Apron Manager, SMTpass solver, ...).
 */
class AIPass : public AnalysisPass, private InstVisitor<AIPass> {
	friend class InstVisitor<AIPass>;

	protected:
		/** 
		 * \brief access to the Live pass
		 */
		Live * LV;
		
		/* in case we want an alias analysis
		 * AliasAnalysis * AA;
		 * AliasSetTracker *AST; 
		 */

		/**
		 * \brief array of lincons we use to do widening with threshold
		 *
		 * this array is computed in computeTransform
		 */
		Constraint_array * threshold;

		/**
		 * \brief true iff threshold is empty and cannot be cleared
		 */
		bool threshold_empty;

		/** 
		 * \brief set to true when the analysis fails (timeout, ...)
		 */
		bool unknown;

		/**
		 * \brief if true, apply Halbwach's narrowing
		 */
		bool NewNarrowing;

		/**
		 * \brief if true, apply widening with threshold
		 * instead of classic widening
		 */
		bool use_threshold;

		/**
		 * \brief path we currently focus on
		 */
		std::vector<BasicBlock*> focuspath;

		/**
		 * \brief index in focuspath of the focuspath's basicblock we are working on
		 */
		unsigned focusblock;
		
		/**
		 * \brief list of all the constraints that need to be satisfied 
		 * along the path
		 */
		std::list<std::vector<Constraint*>*> constraints;

		/** 
		 * \brief set of Phi variables with their associated expression, 
		 * that are defined in the last basicblock of the path
		 */
		phivar PHIvars_prime;
		
		/**
		 * \brief set of Phi variables with their associated expression, 
		 * that are defined in the "middle" of the path 
		 * (i.e. not at the last basicblock)
		 */
		phivar PHIvars;

		/**
		 * \brief list of active Nodes, that still have to be computed
		 */
		std::priority_queue<Node*,std::vector<Node*>,NodeCompare> A;

		/** 
		 * \brief remembers the Nodes that don't need to be recomputed.
		 * This is used to remove duplicates in the A list.
		 */
		std::map<Node*,bool> is_computed;

		/**
		 * \brief apron manager we use along the pass
		 */
		ap_manager_t* man;

		/**
		 * \brief manager that creates abstract values
		 */
		AbstractMan* aman;

		/**
		 * \brief result of the SMTpass pass
		 */
		SMTpass * LSMT;


		/**
		 * \brief assert in the SMT formula the invariants found by a previous
		 * analysis
		 * \param P parameters of the previous analysis. This analysis has to be
		 * run before calling this function
		 * \param F the function
		 */
		void assert_invariant(
				params P,
				Function * F
				);

		/**
		 * \brief returns false iff the technique computes an invariant at
		 * each control point
		 */
		virtual bool is_SMT_technique() {return false;}

	public:

		AIPass (Apron_Manager_Type _man, bool use_New_Narrowing, bool _use_Threshold) : 
			LV(NULL),
			LSMT(NULL),
			unknown(false),
			NewNarrowing(use_New_Narrowing),
			use_threshold(_use_Threshold) {
				man = create_manager(_man);
				init();
			}

		AIPass () : 
			LV(NULL),
			LSMT(NULL),
			unknown(false) {
				man = create_manager(getApronManager());
				NewNarrowing = useNewNarrowing();
				use_threshold = useThreshold();
				init();
			}

		void init() {
				init_apron();
				Environment empty_env;
				threshold = new Constraint_array();
				threshold_empty = false;
		}

		virtual ~AIPass () {
				ap_manager_free(man);
				if (!threshold_empty)
					delete threshold;
			}

		/**
		 * \brief print a path on standard output
		 */
		static void printPath(std::list<BasicBlock*> path);
	protected:

		virtual void computeFunction(Function * F) = 0;

		/** 
		 * \brief compute and update the Abstract value of the Node n
		 * This function should update the set A of active nodes to
		 * reflect changes performed on Node n.
		 */
		virtual void computeNode(Node * n) = 0;

		/**
		 * \brief apply narrowing at node n
		 * This function should update the set A of active nodes to
		 * reflect changes performed on Node n.
		 */
		virtual void narrowNode(Node * n) = 0;
		
		/**
		 * \brief Basic abstract interpretation ascending iterations
		 * (iterates over the nodes, calling computeNode for each of
		 * them)
		 */
		virtual void ascendingIter(Node * n, Function * F, bool dont_reset = false);

		/** 
		 * \brief Narrowing algorithm (iterates over the nodes, calling
		 * narrowNode() for each of them)
		 */
		virtual void narrowingIter(Node * n);

		void loopiter(
			Node * n, 
			Abstract * &Xtemp,
			std::list<BasicBlock*> * path,
			bool &only_join,
			PathTree * const U,
			PathTree * const V);
	
		/** 
		 * \brief delete all pathtrees inside the map and clear the map
		 */
		void ClearPathtreeMap(std::map<BasicBlock*,PathTree*> & pathtree);

		/** 
		 * \{
		 * \name copy methods
		 *
		 * \brief copy the elements in X_d into X_s abstract values
		 * return true iff there there some Xd values that were not equal to Xs
		 * same principle for the two other functions
		 */
		bool copy_Xd_to_Xs(Function * F);
		void copy_Xs_to_Xf(Function * F);
		void copy_Xf_to_Xs(Function * F);
		/**
		 * \}
		 */

		/** 
		 * \brief computes in Xtemp the polyhedra resulting from
		 * the transformation  of n->X through the path
		 */
		void computeTransform (	
			AbstractMan * aman,
			Node * n, 
			std::list<BasicBlock*> path, 
			Abstract *Xtemp);

		/** 
		 * \brief compute Seeds for Halbwach's narrowing
		 * returns true iff one ore more seeds have been found
		 */
		bool computeNarrowingSeed(Function * F);

		/** 
		 * \brief compute the new environment of Node n, based on 
		 * its intVar and RealVar maps
		 */
		void computeEnv(Node * n);
		
		/** 
		 * \brief creates the constraint arrays resulting from a
		 * value.
		 */
		bool computeCondition(Value * val, 
				bool result,
				int cons_index,
				std::vector< std::vector<Constraint*> * > * cons);

		/** 
		 * \brief creates the constraint arrays resulting from a
		 * comparison instruction.
		 */
		bool computeCmpCondition(CmpInst * inst, 
				bool result,
				int cons_index,
				std::vector< std::vector<Constraint*> * > * cons);

		/** 
		 * \brief creates the constraint arrays resulting from a
		 * Constant integer
		 */
		bool computeConstantCondition(ConstantInt * inst, 
				bool result,
				int cons_index,
				std::vector< std::vector<Constraint*> * > * cons);

		/** 
		 * \brief creates the constraint arrays resulting from a
		 * boolean PHINode
		 */
		bool computePHINodeCondition(PHINode * inst, 
				bool result,
				int cons_index,
				std::vector< std::vector<Constraint*> * > * cons);

		/** 
		 * \brief creates the constraint arrays resulting from a
		 * boolean Binary Operator
		 */
		bool computeBinaryOpCondition(BinaryOperator * inst, 
				bool result,
				int cons_index,
				std::vector< std::vector<Constraint*> * > * cons);
		
		/** 
		 * \brief creates the constraint arrays resulting from a
		 * cast between a boolean and an integer
		 */
		bool computeCastCondition(CastInst * inst, 
				bool result,
				int cons_index,
				std::vector< std::vector<Constraint*> * > * cons);

		/** 
		 * \brief Insert all the dimensions of the environment into the node
		 * variables of n
		 */
		void insert_env_vars_into_node_vars(Environment * env, Node * n, Value * V);

		/**
		 * \brief initialize the function by creating the Node
		 * objects, and computing the strongly connected components.
		 */
		void initFunction(Function * F);

		/** 
		 * \brief free internal data after the analysis of a
		 * function
		 * Has to be called after the analysis of each function
		 */
		void TerminateFunction(Function * F);
		
		/** 
		 * \brief print a basicBlock on standard output
		 */
		static void printBasicBlock(BasicBlock * b);

		/** 
		 * \brief process the sequence of positions where an invariant has to be
		 * displayed
		 */
		void computeResultsPositions(
			Function * F,
			std::map<std::string,std::multimap<std::pair<int,int>,BasicBlock*> > * files 
		);

		/** 
		 * \brief inserts pagai invariants into the LLVM Module
		 */
		void InstrumentLLVMBitcode(Function * F);

		/** 
		 * \brief print an invariant on oss, with an optional padding
		 */
		void printInvariant(BasicBlock * b, std::string left, llvm::raw_ostream * oss);

		/** 
		 * \brief computes the set of predecessors for a BasicBlock
		 */
		virtual std::set<BasicBlock*> getPredecessors(BasicBlock * b) const = 0;

		/** 
		 * \brief computes the set of Successors for a BasicBlock
		 */
		virtual std::set<BasicBlock*> getSuccessors(BasicBlock * b) const = 0;

	private:
		void visitInstAndAddVarIfNecessary(Instruction &I);
		/** \{
		 *  \name Visit methods
		 */
		void visitReturnInst (ReturnInst &I);
		void visitBranchInst (BranchInst &I);
		void visitSwitchInst (SwitchInst &I);
		void visitIndirectBrInst (IndirectBrInst &I);
		void visitInvokeInst (InvokeInst &I);
#if LLVM_VERSION_MAJOR < 3 || LLVM_VERSION_MINOR == 0
		void visitUnwindInst (UnwindInst &I);
#endif
		void visitUnreachableInst (UnreachableInst &I);
		void visitICmpInst (ICmpInst &I);
		void visitFCmpInst (FCmpInst &I);
		void visitAllocaInst (AllocaInst &I);
		void visitLoadInst (LoadInst &I);
		void visitStoreInst (StoreInst &I);
		void visitGetElementPtrInst (GetElementPtrInst &I);
		void visitPHINode (PHINode &I);
		void visitTruncInst (TruncInst &I);
		void visitZExtInst (ZExtInst &I);
		void visitSExtInst (SExtInst &I);
		void visitFPTruncInst (FPTruncInst &I);
		void visitFPExtInst (FPExtInst &I);
		void visitFPToUIInst (FPToUIInst &I);
		void visitFPToSIInst (FPToSIInst &I);
		void visitUIToFPInst (UIToFPInst &I);
		void visitSIToFPInst (SIToFPInst &I);
		void visitPtrToIntInst (PtrToIntInst &I);
		void visitIntToPtrInst (IntToPtrInst &I);
		void visitBitCastInst (BitCastInst &I);
		void visitSelectInst (SelectInst &I);
		void visitCallInst(CallInst &I);
		void visitVAArgInst (VAArgInst &I);
		void visitExtractElementInst (ExtractElementInst &I);
		void visitInsertElementInst (InsertElementInst &I);
		void visitShuffleVectorInst (ShuffleVectorInst &I);
		void visitExtractValueInst (ExtractValueInst &I);
		void visitInsertValueInst (InsertValueInst &I);
		void visitTerminatorInst (TerminatorInst &I);
		void visitBinaryOperator (BinaryOperator &I);
		void visitCmpInst (CmpInst &I);
		void visitCastInst (CastInst &I);


		void visitInstruction(Instruction &I) {
			ferrs() << I;
			assert(0 && "Instruction not interpretable yet!");
		}
		/** 
		 * \}
		 */
};

extern AIPass * CurrentAIpass;

#endif
