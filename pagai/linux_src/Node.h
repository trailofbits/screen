/**
 * \file Node.h
 * \brief Declaration of the Node class
 * \author Julien Henry
 */
#ifndef _NODE_H
#define _NODE_H

#include<stack>
#include<map>
#include<set>
#include<vector>

#include "llvm/IR/BasicBlock.h"

#include "ap_global1.h"

#include "Analyzer.h"
#include "Environment.h"
#include "Expr.h"

using namespace llvm;

class Abstract;
class Live;
class Expr;

struct phivar {
	std::vector<ap_var_t> name;
	std::vector<Expr*> expr;
};

struct params {
	Techniques T;
	Apron_Manager_Type D;
	// enhanced narrowing if N is true
	bool N;
	// widening with threshold
	bool TH;

	bool operator<(const params& A) const
	{ return (T<A.T) 
		|| (T==A.T && D < A.D) 
			|| (T==A.T && D==A.D && N < A.N)
			|| (T==A.T && D==A.D && N == A.N && TH < A.TH); }
};

/**
 * \class Node
 * \brief class that keeps information associated to a BasicBlock
 *
 * Information associated to a BasicBlock
 * (abstract values, ...). The BasicBlock <-> Node association is
 * maintained through Node::bb and the global variable Nodes.
 */
class Node {
	private:
		/**
		 * \{
		 * \name used by computeSCC
		 */
		int index;
		int lowlink;
		bool isInStack;

		/** 
		 * \brief recursive version of the tarjan's algorithm
		 * compute both the loop heads and the Strongly connected components
		 * Must be called with n=1 and and empty allocated stack
		 */
		void computeSCC_rec(int & n,std::stack<Node*> * S);

		/**
		 * \}
		 */

		Environment * env;
	public:

		/**
		 * \brief constructor
		 */
		Node(BasicBlock * _bb);

		~Node();

		/**
		 * \{
		 * \name methods for getting and setting internal apron environment
		 */
		Environment * getEnv();
		void setEnv(Environment * e);
		/**
		 * \}
		 */

		/**
		 * \brief BasicBlock associated to the Node
		 */
		BasicBlock * bb;

		/** 
		 * \brief sccId identifies the strongly connected component the node is in
		 */
		int sccId;
		int id;

		/**
		 * \brief Abstract value of the source state
		 */
		std::map<params,Abstract*> X_s;

		/**
		 * \brief  Abstract value of the destination state
		 */
		std::map<params,Abstract*> X_d;

		/**
		 * \brief  First Abstract value not bottom during the analysis
		 */
		std::map<params,Abstract*> X_i;

		std::map<params,Abstract*> X_f;


		/**
		 * contains all the int variables that have to be used as
		 * dimensions for the abstract value. Each variable is associated to a
		 * list of Values, that directly use this variable : if one of these
		 * Value is live, then the variable should not be removed from the Abstract
		 * domain's dimensions
		 */
		std::map<Value*,std::set<ap_var_t> > intVar;

		/**
		 * \brief same as intVar, but for real variables
		 */
		std::map<Value*,std::set<ap_var_t> > realVar;


		/**
		 * \brief compute the strongly connected components of the CFG 
		 * and loop headers
		 */
		void computeSCC();

		/**
		 * \brief add a new variable into the abstract domain
		 */
		void add_var(Value * val);
};

/** 
 * \brief Map that associate each BasicBlock of the Module with its Node
 * object
 */
extern std::map<BasicBlock *,Node *> Nodes;

/**
 * \class NodeCompare
 * \brief This class is used to order the Nodes such that they are
 * poped in the right order (following the SCC order) when treated by
 * the AI algorithm.
 */
class NodeCompare {
	public:
		bool operator() (Node * n1, Node * n2);
};
#endif
