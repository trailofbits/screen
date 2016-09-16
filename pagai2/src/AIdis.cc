/**
 * \file AIdis.cc
 * \brief Implementation of the AIdis pass
 * \author Julien Henry
 */
#include <vector>
#include <list>

#include "AIdis.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "Live.h"
#include "SMTpass.h"
#include "Pr.h"
#include "Debug.h"
#include "Analyzer.h"
#include "PathTree.h"
#include "PathTree_br.h"
#include "ModulePassWrapper.h"

using namespace llvm;

static RegisterPass<AIdis> X("AIdisPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIdis, 0> > Y0("AIdisPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIdis, 1> > Y1("AIdisPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIdis::ID = 0;

const char * AIdis::getPassName() const {
	return "AIdis";
}

void AIdis::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<Live>();
}

bool AIdis::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b = NULL;
	Node * n = NULL;
	int N_Pr = 0;
	LSMT = SMTpass::getInstance();

	*Dbg << "// analysis: DISJUNCTIVE\n";

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = &*mIt;

		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;
		if (definedMain() && !isMain(F)) continue;

		sys::TimeValue * time = new sys::TimeValue(0,0);
		*time = sys::TimeValue::now();
		Total_time[passID][F] = time;

		initFunction(F);


		// we create the new pathtree and Sigma
		Pr * FPr = Pr::getInstance(F);
		std::set<BasicBlock*>* Pr = FPr->getPr(); 
		for (std::set<BasicBlock*>::iterator it = Pr->begin(), et = Pr->end();
			it != et;
			it++) {
			pathtree[*it] = new PathTree_br(*it);
			S[*it] = new Sigma(*it,Max_Disj);
		}

		computeFunction(F);
		*Total_time[passID][F] = sys::TimeValue::now()-*Total_time[passID][F];
		
		TerminateFunction(F);
		printResult(F);

		// we delete the previous pathtree
		for (std::map<BasicBlock*,PathTree*>::iterator it = pathtree.begin(), et = pathtree.end();
			it != et;
			it++) {
			delete (*it).second;
		}
		pathtree.clear();

		// we delete the previous Sigma
		for (std::map<BasicBlock*,Sigma*>::iterator it = S.begin(), et = S.end();
			it != et;
			it++) {
			delete (*it).second;
		}
		S.clear();

		LSMT->reset_SMTcontext();
	}
	generateAnnotatedFiles(F->getParent(),OutputAnnotatedFile());
	return 0;
}



void AIdis::computeFunction(Function * F) {
	Node * const n = Nodes[&*F->begin()];
	Node * current;
	unknown = false;

	// A = {first basicblock}
	auto b = F->begin();
	if (b == F->end()) return;


	// get the information about live variables from the LiveValues pass
	LV = &(getAnalysis<Live>(*F));

	LSMT->push_context();
	PDEBUG(
	if (!quiet_mode())
		*Dbg << "Computing Rho...";
		);
	LSMT->SMT_assert(LSMT->getRho(*F));
	PDEBUG(
	if (!quiet_mode())
		*Dbg << "OK\n";
		);

	// add all function's arguments into the environment of the first bb
	for (Function::arg_iterator a = F->arg_begin(), e = F->arg_end(); a != e; ++a) {
		Argument * arg = &*a;
		if (!(arg->use_empty()))
			n->add_var(arg);
	}
	// first abstract value is top
	computeEnv(n);
	Environment env(n,LV);
	n->X_s[passID]->set_top(&env);
	n->X_d[passID]->set_top(&env);

	while (!A_prime.empty()) 
			A_prime.pop();
	while (!A.empty()) 
			A.pop();

	//A' <- initial state
	A_prime.push(n);

	// Abstract Interpretation algorithm
	START();
	while (!A_prime.empty() && !unknown) {
		
		// compute the new paths starting in a point in A'
		is_computed.clear();
		while (!A_prime.empty()) {
			Node * current = A_prime.top();
			A_prime.pop();
			computeNewPaths(current); // this method adds elements in A and A'
			TIMEOUT(unknown = true;);
			if (unknown) {
				while (!A_prime.empty()) A_prime.pop();
				goto end;
			}
		}

		is_computed.clear();
		ascendingIter(n, F, true);
		if (unknown) goto end;

		// we set X_d abstract values to bottom for narrowing
		Pr * FPr = Pr::getInstance(F);
		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			if (FPr->getPr()->count(&*i) && Nodes[&*b] != n) {
				Nodes[&*b]->X_d[passID]->set_bottom(&env);
			}
		}

		narrowingIter(n);
		if (unknown) goto end;

		// then we move X_d abstract values to X_s abstract values
		int step = 0;
		while (copy_Xd_to_Xs(F) && step <= 1) {
			narrowingIter(n);
			TIMEOUT(unknown = true;);
			if (unknown) goto end;
			step++;
		}
	}
end:
	LSMT->pop_context();
}

std::set<BasicBlock*> AIdis::getPredecessors(BasicBlock * b) const {
	Pr * FPr = Pr::getInstance(b->getParent());
	return FPr->getPrPredecessors(b);
}

std::set<BasicBlock*> AIdis::getSuccessors(BasicBlock * b) const {
	Pr * FPr = Pr::getInstance(b->getParent());
	return FPr->getPrSuccessors(b);
}

int AIdis::sigma(
		std::list<BasicBlock*> path, 
		int start,
		Abstract * Xtemp,
		bool source) {
	return S[path.front()]->getSigma(path,start,Xtemp,this,source);
}

void AIdis::computeNewPaths(Node * n) {
	Node * Succ;
	Abstract * Xtemp = NULL;
	std::vector<Abstract*> Join;

	// first, we set X_d abstract values to X_s
	Pr * FPr = Pr::getInstance(n->bb->getParent());
	std::set<BasicBlock*> successors = FPr->getPrSuccessors(n->bb);
	for (std::set<BasicBlock*>::iterator it = successors.begin(),
			et = successors.end();
			it != et;
			it++) {
		Succ = Nodes[*it];
		delete Succ->X_d[passID];
		Succ->X_d[passID] = aman->NewAbstract(Succ->X_s[passID]);
	}
	while (true) {
		PDEBUG(
			changeColor(raw_ostream::RED);
			*Dbg << "COMPUTENEWPATHS------ NEW SMT SOLVE -------------------------\n";
			resetColor();
		);
		// creating the SMTpass formula we want to check
		LSMT->push_context();
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,false,passID,
				pathtree[n->bb]->generateSMTformula(LSMT,true));
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			LSMT->man->SMT_print(smtexpr);
		);
		int res;
		int index = 0;
		res = LSMT->SMTsolve(smtexpr,&path,index,n->bb->getParent(),passID);
		LSMT->pop_context();

		// if the result is unsat, then the computation of this node is finished
		if (res != 1 || path.size() == 1) {
			if (res == -1) {
				unknown = true;
				return;
			}
			break;
		}

		TIMEOUT(unknown = true; return;);

		Succ = Nodes[path.back()];
		// computing the image of the abstract value by the path's tranformation
		AbstractDisj * Xdisj = dynamic_cast<AbstractDisj*>(n->X_s[passID]);
		Xtemp = Xdisj->man_disj->NewAbstract(Xdisj->getDisjunct(index));

		PDEBUG(
			*Dbg << "START\n";
			Xtemp->print();
		);
		computeTransform(Xdisj->man_disj,n,path,Xtemp);
		PDEBUG(
			*Dbg << "XTEMP\n";
			Xtemp->print();
		);
		AbstractDisj * SuccDisj = dynamic_cast<AbstractDisj*>(Succ->X_s[passID]);
		int Sigma = sigma(path,index,Xtemp,false);
		Join.clear();
		Join.push_back(SuccDisj->getDisjunct(Sigma));
		Join.push_back(Xdisj->man_disj->NewAbstract(Xtemp));
		Environment Xtemp_env(Xtemp);
		Xtemp->join_array(&Xtemp_env,Join);
		SuccDisj->setDisjunct(Sigma,Xtemp);
		Xtemp = NULL;

		// there is a new path that has to be explored
		pathtree[n->bb]->insert(path,false);
		PDEBUG(
			*Dbg << "INSERTING INTO P THE PATH\n";
			printPath(path);
			*Dbg << "RESULT\n";
			Succ->X_s[passID]->print();
		);
		A.push(n);
		A.push(Succ);
		//is_computed[Succ] = false;
		A_prime.push(Succ);
	}
}

void AIdis::loopiter(
		Node * n, 
		int index,
		int Sigma,
		Abstract * &Xtemp, 
		std::list<BasicBlock*> * path,
		bool &only_join, 
		PathTree * const U,
		PathTree * const V
		) {
	Node * Succ = n;
	AbstractDisj * SuccDis = dynamic_cast<AbstractDisj*>(Succ->X_s[passID]);

	std::vector<Abstract*> Join;
	if (U->exist(*path)) {
		if (V->exist(*path)) {
			only_join = false;
		} else {
			// backup the previous abstract value
			Abstract * Xpred = SuccDis->man_disj->NewAbstract(SuccDis->getDisjunct(index));

			Join.clear();
			Join.push_back(SuccDis->man_disj->NewAbstract(SuccDis->getDisjunct(Sigma)));
			Join.push_back(SuccDis->man_disj->NewAbstract(Xtemp));
			Environment Xtemp_env(Xtemp);
			Xtemp->join_array(&Xtemp_env,Join);

			PDEBUG(
				*Dbg << "BEFORE MINIWIDENING\n";	
				*Dbg << "Succ->X:\n";
				SuccDis->print();
				*Dbg << "Xtemp:\n";
				Xtemp->print();
			);

			PDEBUG(
				*Dbg << "THRESHOLD:\n";
				threshold->print();
			);
			if (use_threshold)
				Xtemp->widening_threshold(SuccDis->getDisjunct(Sigma),threshold);
			else
				Xtemp->widening(SuccDis->getDisjunct(Sigma));
			PDEBUG(
				*Dbg << "MINIWIDENING!\n";	
			);
			delete SuccDis->getDisjunct(Sigma);
			SuccDis->setDisjunct(Sigma,Xtemp);
			PDEBUG(
				*Dbg << "AFTER MINIWIDENING\n";	
				Xtemp->print();
			);

			Xtemp = SuccDis->man_disj->NewAbstract(SuccDis->getDisjunct(Sigma));
			computeTransform(SuccDis->man_disj,n,*path,Xtemp);
			PDEBUG(
				*Dbg << "POLYHEDRON AT THE STARTING NODE (AFTER MINIWIDENING)\n";
				SuccDis->print();
				*Dbg << "POLYHEDRON AFTER PATH TRANSFORMATION (AFTER MINIWIDENING)\n";
				Xtemp->print();
			);
			
			delete SuccDis->getDisjunct(index);
			SuccDis->setDisjunct(index,Xpred);
			only_join = true;
			V->insert(*path);
		}
	} else {
		only_join = true;
		U->insert(*path);
	}
}

void AIdis::computeNode(Node * n) {
	BasicBlock * const b = n->bb;
	Abstract * Xtemp = NULL;
	Node * Succ = NULL;
	std::vector<Abstract*> Join;
	bool only_join = false;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}
	std::map<int,PathTree*> U;
	std::map<int,PathTree*> V;

	PDEBUG (
		changeColor(raw_ostream::GREEN);
		*Dbg << "#######################################################\n";
		*Dbg << "Computing node: " << b << "\n";
		resetColor();
		*Dbg << *b << "\n";
	);

	while (true) {
		is_computed[n] = true;
		PDEBUG(
			changeColor(raw_ostream::RED);
			*Dbg << "COMPUTENODE------- NEW SMT SOLVE -------------------------\n";
			resetColor();
		);
		LSMT->push_context();
		// creating the SMTpass formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(b,false,passID,pathtree[b]->generateSMTformula(LSMT));
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res;
		int index;
		res = LSMT->SMTsolve(smtexpr,&path,index,n->bb->getParent(),passID);
		LSMT->pop_context();
		if (res != 1 || path.size() == 1) {
			if (res == -1) {
				unknown = true;
				goto end;
			}
			break;
		}

		TIMEOUT(unknown = true; goto end;);

		PDEBUG(
			printPath(path);
		);
		Succ = Nodes[path.back()];
		
		asc_iterations[passID][n->bb->getParent()]++;
		
		// computing the image of the abstract value by the path's tranformation
		AbstractDisj * Xdisj = dynamic_cast<AbstractDisj*>(n->X_s[passID]);
		Xtemp = Xdisj->man_disj->NewAbstract(Xdisj->getDisjunct(index));
		computeTransform(Xdisj->man_disj,n,path,Xtemp);
		int Sigma = sigma(path,index,Xtemp,true);
		PDEBUG(
			*Dbg << "POLYHEDRON AT THE STARTING NODE\n";
			n->X_s[passID]->print();
			*Dbg << "POLYHEDRON AFTER PATH TRANSFORMATION\n";
			Xtemp->print();
		);
		AbstractDisj * SuccDisj = dynamic_cast<AbstractDisj*>(Succ->X_s[passID]);
		Environment Xtemp_env(Xtemp);
		SuccDisj->change_environment(&Xtemp_env,Sigma);

		if (!U.count(index))
			U[index] = new PathTree_br(n->bb);
		if (!V.count(index))
			V[index] = new PathTree_br(n->bb);

		// if we have a self loop, we apply loopiter
		if (Succ == n) {
			loopiter(n,index,Sigma,Xtemp,&path,only_join,U[index],V[index]);
		} 
		Join.clear();
		Join.push_back(Xdisj->man_disj->NewAbstract(SuccDisj->getDisjunct(Sigma)));
		Join.push_back(Xdisj->man_disj->NewAbstract(Xtemp));
		Xtemp->join_array(&Xtemp_env,Join);

		Pr * FPr = Pr::getInstance(b->getParent());
		if (FPr->inPw(Succ->bb) && ((Succ != n) || !only_join)) {
			if (use_threshold)
				Xtemp->widening_threshold(SuccDisj->getDisjunct(Sigma),threshold);
			else
				Xtemp->widening(SuccDisj->getDisjunct(Sigma));
			PDEBUG(*Dbg << "WIDENING! \n";);
		} else {
			PDEBUG(*Dbg << "PATH NEVER SEEN BEFORE !!\n";);
		}
		PDEBUG(
			*Dbg << "BEFORE:\n";
			Succ->X_s[passID]->print();
		);
		delete SuccDisj->getDisjunct(Sigma);
		SuccDisj->setDisjunct(Sigma,Xtemp);
		Xtemp = NULL;
		PDEBUG(
			*Dbg << "RESULT:\n";
			Succ->X_s[passID]->print();
		);
		A.push(Succ);
		is_computed[Succ] = false;
		// we have to search for new paths starting at Succ, 
		// since the associated abstract value has changed
		A_prime.push(Succ);
	}

end:
	//delete U;
	for (std::map<int,PathTree*>::iterator it = U.begin(), et = U.end(); it != et; it++) {
		delete it->second;
	}
	for (std::map<int,PathTree*>::iterator it = V.begin(), et = V.end(); it != et; it++) {
		delete it->second;
	}
}

void AIdis::narrowNode(Node * n) {

	Abstract * Xtemp = NULL;
	Node * Succ;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	while (true) {
		is_computed[n] = true;

		PDEBUG(
			changeColor(raw_ostream::RED);
			*Dbg << "NARROWING----------- NEW SMT SOLVE -------------------------\n";
			resetColor();
		);
		LSMT->push_context();
		// creating the SMTpass formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,true,passID,pathtree[n->bb]->generateSMTformula(LSMT));
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res;
		int index = 0;
		res = LSMT->SMTsolve(smtexpr,&path,index,n->bb->getParent(),passID);
		LSMT->pop_context();
		if (res != 1 || path.size() == 1) {
			if (res == -1) unknown = true;
			return;
		}

		TIMEOUT(unknown = true; return;);

		PDEBUG(
			printPath(path);
		);
		
		Succ = Nodes[path.back()];

		desc_iterations[passID][n->bb->getParent()]++;

		// computing the image of the abstract value by the path's tranformation
		AbstractDisj * Xdisj = dynamic_cast<AbstractDisj*>(n->X_s[passID]);
		Xtemp = Xdisj->man_disj->NewAbstract(Xdisj->getDisjunct(index));
		computeTransform(Xdisj->man_disj,n,path,Xtemp);
		
		int Sigma = sigma(path,index,Xtemp,false);

		PDEBUG(
			*Dbg << "POLYHEDRON TO JOIN\n";
			Xtemp->print();
		);

		AbstractDisj * SuccDisj = dynamic_cast<AbstractDisj*>(Succ->X_d[passID]);

		if (SuccDisj->getDisjunct(Sigma)->is_bottom()) {
			delete SuccDisj->getDisjunct(Sigma);
			SuccDisj->setDisjunct(Sigma, Xtemp);
		} else {
			std::vector<Abstract*> Join;
			Join.push_back(SuccDisj->man_disj->NewAbstract(SuccDisj->getDisjunct(Sigma)));
			Join.push_back(Xtemp);
			Environment Xtemp_env(Xtemp);
			SuccDisj->getDisjunct(Sigma)->join_array(&Xtemp_env,Join);
		}
		Xtemp = NULL;
		A.push(Succ);
		is_computed[Succ] = false;
	}
}
