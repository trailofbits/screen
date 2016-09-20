/**
 * \file PathTree_br.cc
 * \brief Implementation of the Pathtree class
 * \author Julien Henry
 */
#include <map>
#include <sstream>

#include "cuddObj.hh"

#include "PathTree_br.h"
#include "Analyzer.h"
#include "Pr.h"

void PathTree_br::createBDDVars(BasicBlock * Start, std::set<BasicBlock*> * Pr, std::map<BranchInst*,int> &map, std::set<BasicBlock*> * seen, bool start) {
	int n;
	BranchInst * br = getConditionnalBranch(Start,start);
	if (br != NULL)
		getBDDfromBranchInst(br,map,n);
	seen->insert(Start);
	if ( start || !Pr->count(Start)) {
		for (succ_iterator PI = succ_begin(Start), E = succ_end(Start); PI != E; ++PI) {
			BasicBlock *Succ = *PI;
			if (!seen->count(Succ))
				createBDDVars(Succ,Pr,BddVar,seen);
		}
	}
}

BranchInst * PathTree_br::getConditionnalBranch(BasicBlock * b, bool start) {
	// access the branch inst of the basicblock if exists
	for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {
          if (BranchInst* Inst = dyn_cast<BranchInst>(&*i)) {
			  if (Inst->isUnconditional() && !start) return NULL;
			  return Inst;
		  }
	}
	return NULL;
}

PathTree_br::PathTree_br(BasicBlock * Start) {
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

PathTree_br::~PathTree_br() {
	delete Bdd;
	delete Bdd_prime;
	delete mgr;	
}

BDD PathTree_br::getBDDfromBddIndex(int n) {
	return mgr->bddVar(n);
}

BDD PathTree_br::getBDDfromBranchInst(BranchInst * b, std::map<BranchInst*,int> &map, int &n) {
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

BranchInst * PathTree_br::getBranchFromLevel(int const i) {
	BranchInst * br = levels[i]; 
	return br;
}

const std::string PathTree_br::getStringFromLevel(int const i, SMTpass * smt) {
	BranchInst * br = levels[i]; 
	BasicBlock * bb = br->getParent();
	if (BddVarStart.count(br) && BddVarStart[br]==i)
		return SMTpass::getNodeName(bb,true);
	else
		return SMTpass::getNodeName(bb,false);
}

void PathTree_br::DumpDotBDD(std::string filename, bool prime) {
	if (prime)
		DumpDotBDD(*Bdd_prime,filename);
	else
		DumpDotBDD(*Bdd,filename);
}

void PathTree_br::DumpDotBDD(BDD graph, std::string filename) {
	std::ostringstream name;
	name << filename << ".dot";

	int n = BddVar.size() + BddVarStart.size();

	char * inames[n];
	for (std::map<BranchInst*,int>::iterator it = BddVar.begin(), et = BddVar.end(); it != et; it++) {
		BasicBlock * origin = it->first->getParent();
		BasicBlock * dest = it->first->getSuccessor(0);
		std::string edge = SMTpass::getEdgeName(origin,dest);
		inames[it->second] = strdup(edge.c_str());
	}
	for (std::map<BranchInst*,int>::iterator it = BddVarStart.begin(), et = BddVarStart.end(); it != et; it++) {
		BasicBlock * origin = it->first->getParent();
		BasicBlock * dest = it->first->getSuccessor(0);
		std::string edge = SMTpass::getEdgeName(origin,dest);
		inames[it->second] = strdup(edge.c_str());
	}

    char const* onames[] = {"B"};
    DdNode *Dds[] = {graph.getNode()};
    int NumNodes = sizeof(onames)/sizeof(onames[0]);
    FILE* fp = fopen(name.str().c_str(), "w");
	Cudd_DumpDot(mgr->getManager(), NumNodes, Dds, 
            const_cast<char**>(inames), const_cast<char**>(onames), fp);
	fclose(fp);
}

#if 0
int print_bdd_index = 0;
#endif
SMT_expr PathTree_br::generateSMTformula(SMTpass * smt, bool neg) {
#if 0
	std::ostringstream s;
	s << "bdd" << print_bdd_index;
	print_bdd_index++;
	DumpDotBDD(s.str(),false);
#endif
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
				Bdd_expr.insert(std::pair<DdNode*,SMT_expr>(node,smt->man->SMT_mk_true()));	
				// we remember the adress of this node for future simplification of
				// the formula
				true_node = N;
			} else {
				Bdd_expr.insert(std::pair<DdNode*,SMT_expr>(node,smt->man->SMT_mk_false()));	
			}
		} else {
			BranchInst * br = getBranchFromLevel(N->index);
			BasicBlock * origin = br->getParent();
			BasicBlock * dest = br->getSuccessor(0);
			std::string edge = SMTpass::getEdgeName(origin,dest);
			SMT_var bbvar = smt->man->SMT_mk_bool_var(edge);
			SMT_expr bbexpr(smt->man->SMT_mk_expr_from_bool_var(bbvar));
	
			Nv  = Cudd_T(N);
			Nnv = Cudd_E(N);
			bool Nnv_comp = false;
			if (Cudd_IsComplement(Nnv)) {
				Nnv = Cudd_Not(Nnv);
				Nnv_comp = true;
			}
			SMT_expr Nv_expr(Bdd_expr[Nv]);
			SMT_expr Nnv_expr(Bdd_expr[Nnv]);
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
				SMT_expr vexpr(smt->man->SMT_mk_expr_from_bool_var(var));
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

	SMT_expr res(smt->man->SMT_mk_and(formula));
	if (neg)
		res = smt->man->SMT_mk_not(res);

	factorized.push_back(res);
	return smt->man->SMT_mk_and(factorized);
} 

BDD PathTree_br::computef(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	BranchInst * br;
	int n;

	workingpath.assign(path.begin(), path.end());
	std::set<int> seen;
	current = workingpath.front();
	workingpath.pop_front();
	br = getConditionnalBranch(current,true);
	BDD f = BDD(getBDDfromBranchInst(br, BddVarStart, n));
	if (br->getSuccessor(0) != workingpath.front())
		f = !f;
	seen.insert(n);

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		br = getConditionnalBranch(current);
		if (br != NULL) {
			BDD block = BDD(getBDDfromBranchInst(br, BddVar, n));
			
			seen.insert(n);
			if (!workingpath.empty()) {
				if (br->getSuccessor(0) == workingpath.front()) {
					f *= block;
				} else {
					f *= (!block);
				}
			}
		}
	}
	// we now have to * with the negations of the other BDD indexes
	//for (int i = 0; i < BddIndex; i++) {
	//	if (!seen.count(i)) {
	//		f = f * !getBDDfromBddIndex(i);
	//	}
	//}
	return f;
}

void PathTree_br::insert(std::list<BasicBlock*> path, bool primed) {
	BDD f = computef(path);
	if (primed) {
		*Bdd_prime = *Bdd_prime + f;
	} else {
		*Bdd = *Bdd + f;
	}
}

void PathTree_br::remove(std::list<BasicBlock*> path, bool primed) {
	BDD f = computef(path);
	if (primed) {
		*Bdd_prime = *Bdd_prime * !f;
	} else {
		*Bdd = *Bdd * !f;
	}
}

void PathTree_br::clear(bool primed) {
	if (primed) {
		*Bdd_prime = mgr->bddZero();
	} else {
		*Bdd = mgr->bddZero();
	}
}

bool PathTree_br::exist(std::list<BasicBlock*> path, bool primed) {
	BDD f = computef(path);
	bool res;
	if (primed) {
		res = f <= *Bdd_prime;
	} else {
		res = f <= *Bdd;
	}
	return res;
}

void PathTree_br::mergeBDD() {
	*Bdd = *Bdd + *Bdd_prime;
	*Bdd_prime = mgr->bddZero();
}

bool PathTree_br::isZero(bool primed) {
	if (primed) {
		return !(*Bdd_prime > mgr->bddZero());
	} else {
		return !(*Bdd > mgr->bddZero());
	}
}
