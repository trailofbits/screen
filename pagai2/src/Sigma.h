/**
 * \file Sigma.h
 * \brief Declaration of the Sigma class
 * \author Julien Henry
 */
#ifndef SIGMA_H
#define SIGMA_H

#include <list>
#include <map>

#include "llvm/IR/BasicBlock.h"

#include "cuddObj.hh"

using namespace llvm;

/**
 * \class Sigma
 * \brief class used by AIdis for computing the sigma function
 *
 * This class uses ADD from the CUDD library
 */
class Sigma {

	private:
		/**
		 * \brief manager of the CUDD library 
		 */
		Cudd * mgr;

		/**
		 * \brief maximum number of disjuncts
		 */
		int Max_Disj;

		void init(BasicBlock * Start);

		void createADDVars(BasicBlock * Start, std::set<BasicBlock*> * Pr, std::map<BasicBlock*,int> &map, std::set<BasicBlock*> * seen, bool start = false);

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
		 * \brief stores the index of the basicBlock in the ADD
		 */
		std::map<BasicBlock*,int> AddVar;

		/**
		 * \brief stores the index of the source basicBlock in the ADD
		 */
		std::map<BasicBlock*,int> AddVarSource;

		/**
		 * \brief number of levels in the ADD
		 */
		int AddIndex;

		ADD getADDfromAddIndex(int n);

		/**
		 * \brief get the ADD node associated to a specific basicblock	
		 *
		 * the map should be AddVar or AddVarSource, depending if we consider
		 * the starting point or not
		 */
		ADD getADDfromBasicBlock(BasicBlock * b,std::map<BasicBlock*,int> &map);
		ADD getADDfromBasicBlock(BasicBlock * b,std::map<BasicBlock*,int> &map, int &n);

		ADD computef(std::list<BasicBlock*> path, int start);

		/** 
		 * \brief Add that stores the various seen paths
		 */
		std::map<int,ADD*> Add;

		/**
		 * \brief insert a path in the Bdd
		 */
		void insert(std::list<BasicBlock*> path, int start);

		/**
		 * \brief remove a path from the Bdd
		 */
		void remove(std::list<BasicBlock*> path, int start);

		/**
		 * \brief check if the Bdd contains the path given as argument
		 */
		bool exist(std::list<BasicBlock*> path, int start);
	
		/**
		 * \brief get the actual value of sigma stored in the BDD for sigma(path,start)
		 */
		int getActualValue(std::list<BasicBlock*> path, int start);

		/**
		 * \brief set the value of sigma(path,start)
		 */
		void setActualValue(std::list<BasicBlock*> path, int start, int value);

		void DumpDotADD(ADD graph, std::string filename);

		bool isZero(int start);

	public:
		
		Sigma(BasicBlock * Start, int _Max_Disj);
		Sigma(BasicBlock * Start);
		~Sigma();

		/**
		 * \brief clear the Add. 
		 *
		 * The result will be an empty Add
		 */
		void clear();

		int getSigma(
			std::list<BasicBlock*> path, 
			int start,
			Abstract * Xtemp,
			AIPass * pass,
			bool source);

};
#endif
