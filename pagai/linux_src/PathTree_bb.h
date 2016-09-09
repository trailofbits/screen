/**
 * \file PathTree_bb.h
 * \brief Declaration of the PathTree_bb class
 * \author Julien Henry
 */
#ifndef _PATHTREE_BB_H
#define _PATHTREE_BB_H

#include <list>
#include <map>
#include <vector>
#include <string>
#include "llvm/IR/BasicBlock.h"

#include "cuddObj.hh"

#include "SMTpass.h"
#include "PathTree.h"

using namespace llvm;

/**
 * \class PathTree_bb
 * \brief represents sets of paths in the graph, by storing the corresponding
 *  basicblocks into a BDD 
 */
class PathTree_bb : public PathTree {

	private:
		/**
		 * \brief manager of the CUDD library 
		 */
		Cudd * mgr;
		
		/**
		 * \{
		 * \name variables needed by some methods
		 */
		DdNode * background;
		DdNode * zero;
		/** 
		 * \}
		 */

		/**
		 * \brief stores the index of the basicBlock in the BDD
		 */
		std::map<BasicBlock*,int> BddVar;
		/**
		 * \brief stores the index of the source basicBlock in the BDD
		 */
		std::map<BasicBlock*,int> BddVarStart;

		std::map<int, BasicBlock*> levels;

		BDD computef(std::list<BasicBlock*> path);

		/**
		 * \brief Bdd that stores the various seen paths
		 */
		BDD * Bdd;

		/** 
		 * \brief Bdd that stores the paths that need to be added in Bdd
		 * in the next step
		 */
		BDD * Bdd_prime;

		/**
		 * \brief number of levels in the BDD
		 */
		int BddIndex;

		BDD getBDDfromBddIndex(int n);
		/**
		 * \brief returns the BDD node associated to a specific
		 * BasicBlock. 
		 *
		 * When considering the source BasicBlock, the map should be
		 * BddVarStart, else it should be BddVar
		 */
		BDD getBDDfromBasicBlock(BasicBlock * b,std::map<BasicBlock*,int> &map, int &n);

		/**
		 * \brief returns the name of the basicBlock associated
		 * to the level i of the Bdd.
		 *
		 * If smt != NULL, this name is exactly the same as the one 
		 * used in the SMTpass pass
		 */
		const std::string getStringFromLevel(
			int i,
			SMTpass * smt = NULL);

		void createBDDVars(BasicBlock * Start, std::set<BasicBlock*> * Pr, std::map<BasicBlock*,int> &map, std::set<BasicBlock*> * seen, bool start = false);

		/**
		 * \brief dump the BDD "graph" in a .dot file. 
		 * \param filename Name of the .dot file
		 */
		void DumpDotBDD(BDD graph, std::string filename);

	public:
		PathTree_bb(BasicBlock * Start);

		~PathTree_bb();

		/**
		 * \brief insert a path in the Bdd
		 */
		void insert(std::list<BasicBlock*> path, bool primed = false);

		/**
		 * \brief remove a path from the Bdd
		 */
		void remove(std::list<BasicBlock*> path, bool primed = false);

		/** 
		 * \brief clear the Bdd. The result will be an empty Bdd
		 */
		void clear(bool primed = false);

		/** 
		 * \brief check if the Bdd contains the path given as argument
		 */
		bool exist(std::list<BasicBlock*> path, bool primed = false);

		/** 
		 * \brief merge the two Bdds into Bdd. Bdd_prime is cleared
		 */
		void mergeBDD();

		bool isZero(bool primed = false);

		/** 
		 * \brief dump the graph
		 */
		void DumpDotBDD(std::string filename, bool prime);

		/** 
		 * \brief generate the SMTpass formula associated to the Bdd
		 */
		SMT_expr generateSMTformula(
			SMTpass * smt, bool neg = false);
};
#endif
