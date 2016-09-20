/**
 * \file Sigma.cc
 * \brief Implementation of the Sigma class
 * \author Julien Henry
 */
#include <sstream>

#include "cuddObj.hh"

#include "Analyzer.h"
#include "AIpass.h"
#include "Sigma.h"
#include "Pr.h"
#include "SMTpass.h"
#include "Debug.h"

//#define DUMP_ADD	


void Sigma::createADDVars(BasicBlock * Start, std::set<BasicBlock*> * Pr, std::map<BasicBlock*,int> &map, std::set<BasicBlock*> * seen, bool start) {
	int n;
	getADDfromBasicBlock(Start,map,n);
	seen->insert(Start);
	if (start || !Pr->count(Start)) {
		for (succ_iterator PI = succ_begin(Start), E = succ_end(Start); PI != E; ++PI) {
			BasicBlock *Succ = *PI;
			if (!seen->count(Succ))
				createADDVars(Succ,Pr,AddVar,seen);
		}
	}
}

void Sigma::init(BasicBlock * Start) {
	mgr = new Cudd(0,0);
	AddIndex=0;
	background = mgr->addZero().getNode();
	zero = mgr->addZero().getNode();

	// we compute all the levels of the ADD
	Function * F = Start->getParent();
	Pr * FPr = Pr::getInstance(F);
	std::set<BasicBlock*> * Pr = FPr->getPr();
	std::set<BasicBlock*> seen;
	createADDVars(Start,Pr,AddVarSource,&seen,true);
}

Sigma::Sigma(BasicBlock * Start, int _Max_Disj): Max_Disj(_Max_Disj) {
	init(Start);
}

Sigma::Sigma(BasicBlock * Start) {
	init(Start);
	Max_Disj = 5;
}

Sigma::~Sigma() {
	for (std::map<int,ADD*>::iterator it = Add.begin(), et = Add.end(); it != et; it++) {
		delete it->second;
	}
	delete mgr;	
}

ADD Sigma::getADDfromBasicBlock(BasicBlock * b, std::map<BasicBlock*,int> &map) {
	int n;
	return getADDfromBasicBlock(b,map,n);
}

ADD Sigma::getADDfromBasicBlock(BasicBlock * b, std::map<BasicBlock*,int> &map, int &n) {
	ADD res;
	if (!map.count(b)) {
		n = AddIndex;
		AddIndex++;
		map[b] = n;
		res = mgr->addVar(n);
		std::map<int,ADD*>::iterator it = Add.begin(), et = Add.end();
		for (;it != et; it++) {
			ADD* A = (*it).second;
			*A = *A * ~res;
		}
	} else {
		n = map[b];	
		res = mgr->addVar(n);
	}
	return res;
}

ADD Sigma::getADDfromAddIndex(int n) {
	return mgr->addVar(n);
}

void Sigma::insert(std::list<BasicBlock*> path, int start) {
	ADD f = computef(path,start);
	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	*Add[start] = *Add[start] + f;
}

void Sigma::remove(std::list<BasicBlock*> path, int start) {
	ADD f = computef(path,start);
	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	*Add[start] = *Add[start] * ~f;
}

void Sigma::clear() {
	for (std::map<int,ADD*>::iterator it = Add.begin(), et = Add.end(); it != et; it++) {
		*(it->second) = mgr->addZero();
	}
}

ADD Sigma::computef(std::list<BasicBlock*> path, int start) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	int n;

	workingpath.assign(path.begin(), path.end());

	std::set<int> seen;
	current = workingpath.front();
	workingpath.pop_front();
	ADD f = ADD(getADDfromBasicBlock(current, AddVarSource, n));
	seen.insert(n);

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar, n));
		seen.insert(n);
		f = f * block;
	}
	// we now have to * with the negations of the other ADD indexes
	for (int i = 0; i < AddIndex; i++) {
		if (!seen.count(i)) {
			f = f * ~getADDfromAddIndex(i);
		}
	}
	return f;
}

bool Sigma::exist(std::list<BasicBlock*> path, int start) {
	ADD f = computef(path,start);
	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	return (f <= *Add[start] * f);
}

bool Sigma::isZero(int start) {
	return !(*Add[start] > mgr->addZero());
}

void Sigma::setActualValue(std::list<BasicBlock*> path, int start, int value) {
	ADD f = computef(path,start);
	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	// f corresponds to (start,path)
	for (int k = 0; k < value; k++) {
		*Add[start] += f;	
	}
}

int Sigma::getActualValue(std::list<BasicBlock*> path, int start) {
	ADD f = computef(path,start);
	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	// f corresponds to (start,path)
	ADD res = f * *Add[start];

	DdNode *node;
	DdGen *gen;
	DdNode *N;

	for (gen = Cudd_FirstNode (mgr->getManager(), res.getNode(), &node);
	!Cudd_IsGenEmpty(gen);
	(void) Cudd_NextNode(gen,&node)) {
		N = Cudd_Regular(node);

		if (Cudd_IsConstant(N)) {
			if (node != zero) {
				return Cudd_V(N);
			}
		}
	}
	// if we are here, it means the path does not exist in the diagram
	return -1;
}

int Sigma::getSigma(
		std::list<BasicBlock*> path, 
		int start,
		Abstract * Xtemp,
		AIPass * pass,
		bool source) {

	int res = -1;
		
	res = getActualValue(path,start);
	if (res != -1) {
		PDEBUG(
		*Out << start << " is already assigned to " << res-1 << "\n";
		);
	} else {

		AbstractDisj * D  = dynamic_cast<AbstractDisj*>(Nodes[path.back()]->X_d[pass->passID]);
		// we iterate on the already existing abstract values of the disjunct
		std::vector<Abstract*>::iterator it = D->disj.begin(), et = D->disj.end();	
		int index = 0;
		for (; it != et; it++, index++) {
			if (Xtemp->CanJoinPrecisely(D->man_disj,*it)) {
				res = index+1;
				break;
			}
		}
		if (res == -1) {
			// there is no abstract value that fits well
			int N = D->disj.size();
			if (N < Max_Disj)
				res = N+1;
			else
				res = N;
		}

	//	if (path.front() == path.back())
	//		res = 2;
	//	else 
	//		res = 1;
		setActualValue(path,start,res);	
		PDEBUG(
			AIPass::printPath(path);
			*Out << start << " is assigned to " << res-1 << "\n";
		);
	}
#ifdef DUMP_ADD
	std::ostringstream filename;
	filename << "ADD_" << path.front() << "_" << start;
	*Out << "ADD IS "<< filename.str() << "\n";
	DumpDotADD(*Add[start],filename.str());
#endif
	return res -1;
}

void Sigma::DumpDotADD(ADD graph, std::string filename) {
	std::ostringstream name;
	name << filename << ".dot";

	int n = AddVar.size() + AddVarSource.size();
	char * inames[n];
	for (std::map<BasicBlock*,int>::iterator it = AddVar.begin(), et = AddVar.end(); it != et; it++) {
		inames[it->second] = strdup(SMTpass::getNodeName(it->first,false).c_str());
	}
	for (std::map<BasicBlock*,int>::iterator it = AddVarSource.begin(), et = AddVarSource.end(); it != et; it++) {
		inames[it->second] = strdup(SMTpass::getNodeName(it->first,true).c_str());
	}

    char const* onames[] = {"A"};
    DdNode *Dds[] = {graph.getNode()};
    int NumNodes = sizeof(onames)/sizeof(onames[0]);
    FILE* fp = fopen(name.str().c_str(), "w");
	Cudd_DumpDot(mgr->getManager(), NumNodes, Dds, 
            const_cast<char**>(inames), const_cast<char**>(onames), fp);
	fclose(fp);
}
