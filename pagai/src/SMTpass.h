/**
 * \file SMTpass.h
 * \brief Declaration of the SMTpass class
 * \author Julien Henry
 */
#ifndef SMT_H
#define SMT_H

#include <map>
#include <vector>
#include <list>
#include <set>

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/InstVisitor.h"

#include "Analyzer.h"
#include "Abstract.h"
#include "Node.h"
#include "AbstractDisj.h"
#include "SMT_manager.h"

using namespace llvm;

/**
 * \class SMTpass
 * \brief SMT-formula creation pass
 *
 * Uses SMTpass::man as an abstraction layer to access the SMT solver.
 * It is a singleton design : we use getInstance() to get an object of type
 * SMTpass * 
 */
class SMTpass : private InstVisitor<SMTpass> {
	friend class InstVisitor<SMTpass>;
	
	private:
		
		SMTpass();
		~SMTpass();

		static int nundef;

		int stack_level;

		/**
		 * \brief stores the rho formula associated to each function
		 */
		std::map<Function*,SMT_expr> rho;

		/**
		 * \brief stores the already computed varnames, since the computation of
		 * VarNames seems costly
		 */
		static std::map<Value*,std::string> VarNames;

		/**
		 * \brief when constructing rho, we use this vector
		 */
		std::vector<SMT_expr> rho_components;

		/**
		 * \brief when constructing instruction-related SMT formula, we
		 * use this vector
		 */
		std::vector<SMT_expr> instructions;

		/**
		 * \{
		 * \name get*Name functions 
		 * these following methods are used to create a variable name for
		 * edges, nodes, values, undeterministic choices, ...
		 */
		static const std::string getDisjunctiveIndexName(AbstractDisj * A, int index);
		static const std::string getUndeterministicChoiceName(Value * v);
		static const std::string getValueName(Value * v, bool primed);
		/**
		 * \}
		 */


		/**  
		 * \brief get the expression associated to a value
		 */
		SMT_expr getValueExpr(Value * v, bool primed);

		/**
		 * \return the SMT type of the value
		 */
		SMT_type getValueType(Value * v);

		/**
		 * \brief function to use for getting a variable from a value
		 */
		SMT_var getVar(Value * v, bool primed);

		SMT_var getBoolVar(Value * v, bool primed);

		/**
		 * \brief take a string name as input, and find if it
		 * is the name of an edge, a node, or a the index of a disjunctive
		 * invariant
		 *
		 * if it is an edge :
		 *  - src becomes the source of the edge
		 *  - dest becomes the destination of the edge
		 * if it is a Node :
		 *  - src becomes the basicblock of this node
		 *  - start is true iff the block is a start Node
		 * if it is an index for a disjunctive invariant :
		 * - isIndex is true
		 * - index is the associated index
		 */
		static void getElementFromString(	
			std::string name,
			bool &isEdge, 
			bool &isIndex,
			bool &start, 
			BasicBlock * &src, 
			BasicBlock * &dest,
			int &index);
		
		/**
		 * \brief called by visitPHINode
		 */
		SMT_expr construct_phi_ite(PHINode &I, unsigned i, unsigned n);

		/**
		 * \brief say if the value needs to be primed in the basicblock
		 */
		bool is_primed(BasicBlock * b, Instruction &I);

		/**
		 * \brief recursive function called by computeRho
		 */
		void computeRhoRec(	Function &F, 
							std::set<BasicBlock*> * visited,
							BasicBlock * dest);
		void computeRho(Function &F);
		void computePrSuccAndPred(Function &F);

	public:
		static char ID;

		/**
		 * \brief manager (Microsoft z3 or Yices)
		 */
		SMT_manager * man;

		static SMTpass * getInstance();
		static SMTpass * getInstanceForAbstract();
		static void releaseMemory();

		void reset_SMTcontext();

		/**
		 * \brief get the SMT formula Rho. Rho is computed only once
		 */
		SMT_expr getRho(Function &F);

		/** 
		 * \brief returns a name for a string
		 * this name is unique for the Value *
		 */
		static const std::string getVarName(Value * v);
		static const std::string getEdgeName(BasicBlock* b1, BasicBlock* b2);

		/**
		 * \brief push the context of the SMT manager
		 */
		void push_context();

		/**
		 * \brief pop the context of the SMT manager
		 */
		void pop_context();

		/**
		 * \brief assert an expression in the solver
		 */
		void SMT_assert(SMT_expr expr);
		
		/**
		 * \brief compute and return the SMT formula associated to
		 * the BasicBlock source
		 *
		 * See description in the paper SAS12
		 * if we are in narrowing phase, use_X_d has to be true.
		 * We can cunjunct this formula with an SMT_expr formula given as
		 * parameter of the function: constraint
		 */
		SMT_expr createSMTformula(
			BasicBlock * source, 
			bool use_X_d, 
			params t,
			SMT_expr constraint);

		/**
		 * \brief solve the SMT expression expr
		 * \return return true iff expr is
		 * satisfiable. In this case, path containts the path extracted from
		 * the model
		 */
		int SMTsolve(
				SMT_expr expr, 
				std::list<BasicBlock*> * path, 
				Function * F, 
				params passID);

		/** 
		 * \brief solve an SMT formula and computes its model in case of a 'sat'
		 * formula. 
		 *
		 * In the case of a pass using disjunctive invariants, index is set to
		 * the associated index of the disjunct to focus on.
		 */
		int SMTsolve(
				SMT_expr expr, 
				std::list<BasicBlock*> * path, 
				int &index,
				Function * F, 
				params passID);

		/**
		 * \brief solve the SMT expression
		 * \return 1 if satisfiable, 0 if not, -1 if unknown
		 */
		int SMTsolve_simple(SMT_expr expr);

		/**
		 * \brief gets the name of the node associated to a specific basicblock
		 */
		static const std::string getNodeName(BasicBlock* b, bool src);

		static const std::string getNodeSubName(BasicBlock* b);
		static BasicBlock * getNodeBasicBlock(std::string name);

		/**
		 * \{
		 * \name XToSmt - transform an apron object of type X into an SMT expression
		 */
		SMT_expr texpr1ToSmt(ap_texpr1_t texpr);
		SMT_expr linexpr1ToSmt(BasicBlock * b, ap_linexpr1_t linexpr, bool &integer, bool &skip);
		SMT_expr scalarToSmt(ap_scalar_t * scalar, bool integer, double &value, bool &infinity);
		SMT_expr tcons1ToSmt(ap_tcons1_t tcons);
		SMT_expr lincons1ToSmt(BasicBlock * b, ap_lincons1_t lincons,bool &skip);
		SMT_expr AbstractToSmt(BasicBlock * b, Abstract * A);
		/** 
		 * \}
		 */

		/**
		 * \brief Creates an SMT formula associated to a disjunctive invariant. 
		 *
		 * If insert_booleans is true, each disjunct is cunjunct with a boolean
		 * predicate, as detailed in the paper, so that we can deduce which
		 * disjunct to choose.
		 */
		SMT_expr AbstractDisjToSmt(BasicBlock * b, AbstractDisj * A, bool insert_booleans);

		/** 
		 * \{
		 * \name Visit methods
		 */
	private:
		void visitReturnInst (ReturnInst &I);
		void visitBranchInst (BranchInst &I);
		void visitSwitchInst (SwitchInst &I);
		void visitIndirectBrInst (IndirectBrInst &I);
		void visitInvokeInst (InvokeInst &I);
#if LLVM_VERSION_MAJOR < 3 || LLVM_VERSION_MINOR == 0
		void visitUnwindInst (UnwindInst &I);
#endif
		void visitUnreachableInst (UnreachableInst &I);
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
			//ferrs() << I.getOpcodeName();
			//assert(0 && ": Instruction not interpretable yet!");
		}
		/**
		 * \}
		 */
};
#endif
