/**
 * \file PathTree_bb.cc
 * \brief Implementation of the Pathtree class
 * \author Julien Henry
 */
#include <map>
#include <sstream>

#include "cuddObj.hh"

#include "PathTree_bb.h"
#include "Analyzer.h"
#include "Pr.h"

void PathTree_bb::createBDDVars(BasicBlock * Start, std::set<BasicBlock*> * Pr, std::map<BasicBlock*,int> &map, std::set<BasicBlock*> * seen, bool start) {
	int n;
	getBDDfromBasicBlock(Start,map,n);
	seen->insert(Start);
	if ( start || !Pr->count(Start)) {
		for (succ_iterator PI = succ_begin(Start), E = succ_end(Start); PI != E; ++PI) {
			BasicBlock *Succ = *PI;
			if (!seen->count(Succ))
				createBDDVars(Succ,Pr,BddVar,seen);
		}
	}
}

PathTree_bb::PathTree_bb(BasicBlock * Start) {
	mgr = new Cudd(0,0);
	//mgr->makeVerbose();
	Bdd = new BDD(mgr->bddZero());
	Bdd_prime = new BDD(mgr->bddZero());
	BddIndex=0;
	background = mgr->bddZero().getNode();
	zero = mgr->bddZero().getNode();

	// we compute all the levels of the BDD
	Function * F = Start->getParent();
	Pr * FPr = Pr::getInstance(F);
	std::set<BasicBlock*> * Pr = FPr->getPr();
	std::set<BasicBlock*> seen;
	createBDDVars(Start,Pr,BddVarStart,&seen,true);
}

PathTree_bb::~PathTree_bb() {
	delete Bdd;
	delete Bdd_prime;
	delete mgr;	
}

BDD PathTree_bb::getBDDfromBddIndex(int n) {
	return mgr->bddVar(n);
}

BDD PathTree_bb::getBDDfromBasicBlock(BasicBlock * b, std::map<BasicBlock*,int> &map, int &n) {
	if (!map.count(b)) {
		n = BddIndex;
		levels[n] = b;
		BddIndex++;
		map[b] = n;
	} else {
		n = map[b];	
	}
	return mgr->bddVar(n);
}

const std::string PathTree_bb::getStringFromLevel(int const i, SMTpass * smt) {
	BasicBlock * bb = levels[i]; 
	if (BddVarStart.count(bb) && BddVarStart[bb]==i)
		return SMTpass::getNodeName(bb,true);
	else
		return SMTpass::getNodeName(bb,false);
}

void PathTree_bb::DumpDotBDD(std::string filename, bool prime) {
	if (prime)
		DumpDotBDD(*Bdd_prime,filename);
	else
		DumpDotBDD(*Bdd,filename);
}

void PathTree_bb::DumpDotBDD(BDD graph, std::string filename) {
	std::ostringstream name;
	name << filename << ".dot";

	int n = BddVar.size() + BddVarStart.size();

	char * inames[n];
	for (std::map<BasicBlock*,int>::iterator it = BddVar.begin(), et = BddVar.end(); it != et; it++) {
		inames[it->second] = strdup(SMTpass::getNodeName(it->first,false).c_str());
	}
	for (std::map<BasicBlock*,int>::iterator it = BddVarStart.begin(), et = BddVarStart.end(); it != et; it++) {
		inames[it->second] = strdup(SMTpass::getNodeName(it->first,true).c_str());
	}

    char const* onames[] = {"B"};
    DdNode *Dds[] = {graph.getNode()};
    int NumNodes = sizeof(onames)/sizeof(onames[0]);
    FILE* fp = fopen(name.str().c_str(), "w");
	Cudd_DumpDot(mgr->getManager(), NumNodes, Dds, 
            const_cast<char**>(inames), const_cast<char**>(onames), fp);
	fclose(fp);
}

SMT_expr PathTree_bb::generateSMTformula(SMTpass * smt, bool neg) {
	std::map<DdNode*,SMT_expr> Bdd_expr;
	DdNode *node;
	DdNode *N, *Nv, *Nnv;
	DdGen *gen;
	DdNode * true_node;
	std::vector<SMT_expr> formula;
	std::vector<SMT_expr> factorized;
	for(gen = Cudd_FirstNode (mgr->getManager(), Bdd->getNode(), &node);
	!Cudd_IsGenEmpty(gen);
	(void) Cudd_NextNode(gen, &node)) {
	
		// remove the extra 1 from the adress if exists
	    N = Cudd_Regular(node);
	
	    if (Cudd_IsConstant(N)) {
			// Terminal node
			if (node != background && node != zero) {
				// this is the 1 node
				Bdd_expr[node] = smt->man->SMT_mk_true();	
				// we remember the adress of this node for future simplification of
				// the formula
				true_node = N;
			} else {
				Bdd_expr[node] = smt->man->SMT_mk_false();	
			}
		} else {
			std::string bb = getStringFromLevel(N->index,smt);
			SMT_var bbvar = smt->man->SMT_mk_bool_var(bb);
			SMT_expr bbexpr = smt->man->SMT_mk_expr_from_bool_var(bbvar);
	
			Nv  = Cudd_T(N);
			Nnv = Cudd_E(N);
			bool Nnv_comp = false;
			if (Cudd_IsComplement(Nnv)) {
				Nnv = Cudd_Not(Nnv);
				Nnv_comp = true;
			}
			SMT_expr Nv_expr = Bdd_expr[Nv];
			SMT_expr Nnv_expr = Bdd_expr[Nnv];
			if (Nnv_comp)
				Nnv_expr = smt->man->SMT_mk_not(Nnv_expr);
		
			if (Nnv == true_node && Nnv_comp) {
				// we don't need to create an ite formula, because the else condition
				// is always false
				std::vector<SMT_expr> cunj;
				cunj.push_back(bbexpr);
				cunj.push_back(Nv_expr);
				Bdd_expr[node] = smt->man->SMT_mk_and(cunj);
			} else {
				Bdd_expr[node] = smt->man->SMT_mk_ite(bbexpr,Nv_expr,Nnv_expr);
			}

			// if the node is used multiple times, we create a variable for this
			// node
			if (node->ref > 1) {
				std::ostringstream name;
				name << "Bdd_" << node;
				SMT_var var = smt->man->SMT_mk_bool_var(name.str());
				SMT_expr vexpr = smt->man->SMT_mk_expr_from_bool_var(var);
				factorized.push_back(smt->man->SMT_mk_eq(vexpr,Bdd_expr[node]));
				Bdd_expr[node] = vexpr;	
			}
		}
	}
	Cudd_GenFree(gen);
	
	if (Cudd_IsComplement(Bdd->getNode())) 
		// sometimes, Bdd is complemented, especially when Bdd = false
		formula.push_back(smt->man->SMT_mk_not(Bdd_expr[Cudd_Regular(Bdd->getNode())]));
	else
		formula.push_back(Bdd_expr[Cudd_Regular(Bdd->getNode())]);

	SMT_expr res = smt->man->SMT_mk_and(formula);
	if (neg)
		res = smt->man->SMT_mk_not(res);

	factorized.push_back(res);
	return smt->man->SMT_mk_and(factorized);
} 

BDD PathTree_bb::computef(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	int n;

	workingpath.assign(path.begin(), path.end());

	std::set<int> seen;
	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlock(current, BddVarStart, n));
	seen.insert(n);

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlock(current, BddVar, n));
		seen.insert(n);
		f = f * block;
	}
	// we now have to * with the negations of the other BDD indexes
	for (int i = 0; i < BddIndex; i++) {
		if (!seen.count(i)) {
			f = f * !getBDDfromBddIndex(i);
		}
	}
	return f;
}
void PathTree_bb::insert(std::list<BasicBlock*> path, bool primed) {
	BDD f = computef(path);
	if (primed) {
		*Bdd_prime = *Bdd_prime + f;
	} else {
		*Bdd = *Bdd + f;
	}
}

void PathTree_bb::remove(std::list<BasicBlock*> path, bool primed) {
	BDD f = computef(path);
	if (primed) {
		*Bdd_prime = *Bdd_prime * !f;
	} else {
		*Bdd = *Bdd * !f;
	}
}

void PathTree_bb::clear(bool primed) {
	if (primed) {
		*Bdd_prime = mgr->bddZero();
	} else {
		*Bdd = mgr->bddZero();
	}
}

bool PathTree_bb::exist(std::list<BasicBlock*> path, bool primed) {
	BDD f = computef(path);
	bool res;
	if (primed) {
		res = f <= *Bdd_prime;
	} else {
		res = f <= *Bdd;
	}
	return res;
}

void PathTree_bb::mergeBDD() {
	*Bdd = *Bdd + *Bdd_prime;
	*Bdd_prime = mgr->bddZero();
}

bool PathTree_bb::isZero(bool primed) {
	if (primed) {
		return !(*Bdd_prime > mgr->bddZero());
	} else {
		return !(*Bdd > mgr->bddZero());
	}
}
