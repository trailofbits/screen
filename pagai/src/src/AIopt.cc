/**
 * \file AIopt.cc
 * \brief Implementation of the AIopt pass (Combined Technique SAS12)
 * \author Julien Henry
 */
#include <vector>
#include <sstream>
#include <list>
#include <string>
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/IR/InstIterator.h"

#include "AIopt.h"
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

static RegisterPass<AIopt> X("AIOptPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIopt, 0> > Y0("AIOptPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIopt, 1> > Y1("AIOptPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIopt::ID = 0;

const char * AIopt::getPassName() const {
	return "AIopt";
}

void AIopt::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<Live>();
    //AU.addRequiredTransitive<AliasAnalysis>();
}

bool AIopt::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b = NULL;
	Node * n = NULL;
	int N_Pr = 0;
	LSMT = SMTpass::getInstance();

	*Dbg << "// analysis: " << getPassName() << "\n";

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = &*mIt;


		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;
		if (definedMain() && !isMain(F)) continue;
		Pr * FPr = Pr::getInstance(F);
		if (SVComp() && FPr->getAssert()->size() == 0) continue;

		sys::TimeValue * time = new sys::TimeValue(0,0);
		*time = sys::TimeValue::now();
		Total_time[passID][F] = time;

		initFunction(F);

		// we create the new pathtree
		std::set<BasicBlock*>* Pr = FPr->getPr();
		for (std::set<BasicBlock*>::iterator it = Pr->begin(), et = Pr->end();
			it != et;
			it++) {
			if ((*it)->getTerminator()->getNumSuccessors() > 0
					&& ! FPr->inUndefBehaviour(*it)
					&& ! FPr->inAssert(*it)
					) {
				pathtree[*it] = new PathTree_br(*it);
				U[*it] = new PathTree_br(*it);
				V[*it] = new PathTree_br(*it);
			}
		}

		computeFunction(F);
#if 0
		struct timespec max_wait;
        memset(&max_wait, 0, sizeof(max_wait));
        /* wait at most 2 seconds */
        max_wait.tv_sec = 1;
		int timeout = computeFunction_or_timeout(F,&max_wait);

		if (timeout) {
			continue;
		}
#endif
		*Total_time[passID][F] = sys::TimeValue::now()-*Total_time[passID][F];
		
		TerminateFunction(F);
		printResult(F);

		// deleting the pathtrees
		ClearPathtreeMap(pathtree);
		ClearPathtreeMap(U);
		ClearPathtreeMap(V);

		LSMT->reset_SMTcontext();
	}
	generateAnnotatedFiles(F->getParent(),OutputAnnotatedFile());
	
	SMTpass::releaseMemory();
	return 0;
}

void AIopt::computeFunction(Function * F) {
	Node * const n = Nodes[&*F->begin()];
	Node * current;

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

	// we assert b_i => I_i for each block
	params P;
	P.T = SIMPLE;
	P.D = getApronManager();
	P.N = useNewNarrowing();
	P.TH = useThreshold();
	assert_properties(P,F);

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

		W = new PathTree_br(n->bb);
		is_computed.clear();
		ascendingIter(n, F, true);
		if (unknown) {
			delete W;
			goto end;
		}

		if (SVComp() && asserts_proved(F)) {
			delete W;
			continue;
		}

		// we set X_d abstract values to bottom for narrowing
		Pr * FPr = Pr::getInstance(F);
		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			if (FPr->getPr()->count(&*i) && Nodes[&*b] != n) {
				Nodes[&*b]->X_d[passID]->set_bottom(&env);
			}
		}

		narrowingIter(n);
		if (unknown) {
			delete W;
			goto end;
		}
		// then we move X_d abstract values to X_s abstract values
		int step = 0;
		while (copy_Xd_to_Xs(F) && step <= 1 && !unknown) {
			narrowingIter(n);
			TIMEOUT(unknown = true;);
			if (unknown) {
				delete W;
				goto end;
			}
			step++;
		}
		delete W;
	}
end:
	LSMT->pop_context();
}

std::set<BasicBlock*> AIopt::getPredecessors(BasicBlock * b) const {
	Pr * FPr = Pr::getInstance(b->getParent());
	return FPr->getPrPredecessors(b);
}

std::set<BasicBlock*> AIopt::getSuccessors(BasicBlock * b) const {
	Pr * FPr = Pr::getInstance(b->getParent());
	return FPr->getPrSuccessors(b);
}

void AIopt::computeNewPaths(Node * n) {
	Node * Succ;
	Abstract * Xtemp = NULL;
	std::vector<Abstract*> Join;

	// if useXd = true, prevent the Xs abstract values to be updated during the
	// computation of the new paths.
	// this can be relaxed for decreasing the number of subgraphs to work on,
	// but the computation of new paths may be much longer in the presence of
	// self loops
	bool useXd = true;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}
		
	if (!pathtree.count(n->bb)) {
		// this is a block without any successors...
		is_computed[n] = true;
		return;
	}

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
		is_computed[n] = true;
		PDEBUG(
			changeColor(raw_ostream::RED);
			*Dbg << "COMPUTENEWPATHS-------------- SMT SOLVE -------------------------\n";
			resetColor();
		);
		// creating the SMTpass formula we want to check
		LSMT->push_context();
		SMT_expr pathtree_smt = pathtree[n->bb]->generateSMTformula(LSMT,true);
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,useXd,passID,pathtree_smt);
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			*Dbg
				<< "\n"
				<< "FORMULA"
				<< "(COMPUTENEWPATHS)"
				<< "\n\n";
			LSMT->man->SMT_print(smtexpr);
		);
		int res;
		res = LSMT->SMTsolve(smtexpr,&path,n->bb->getParent(),passID);

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
		Abstract * SuccX;
		if (useXd) SuccX = Succ->X_d[passID];
		else SuccX = Succ->X_s[passID];
		// computing the image of the abstract value by the path's tranformation
		Xtemp = aman->NewAbstract(n->X_s[passID]);
		computeTransform(aman,n,path,Xtemp);
		Environment Xtemp_env(Xtemp);
		SuccX->change_environment(&Xtemp_env);

		Join.clear();
		Join.push_back(SuccX);
		Join.push_back(aman->NewAbstract(Xtemp));
		Xtemp->join_array(&Xtemp_env,Join);

		// intersection with the previous invariant
		params P;
		P.T = SIMPLE;
		P.D = getApronManager();
		P.N = useNewNarrowing();
		P.TH = useThreshold();
		intersect_with_known_properties(Xtemp,Succ,P);

		//Succ->X_s[passID] = Xtemp;
		SuccX = Xtemp;
		Xtemp = NULL;

		// there is a new path that has to be explored
		pathtree[n->bb]->insert(path,false);
		PDEBUG(
			*Dbg << "THE FOLLOWING PATH IS INSERTED INTO P'\n";	
			printPath(path);
			*Dbg << "NEW SUCC:" << *SuccX << "\n";	
		);
		A.push(n);
		A.push(Succ);
		//is_computed[Succ] = false;
		A_prime.push(Succ);
		if (useXd) Succ->X_d[passID] = SuccX;
		else Succ->X_s[passID] = SuccX;
	}
}

void AIopt::computeNode(Node * n) {
	BasicBlock * const b = n->bb;
	Abstract * Xtemp = NULL;
	Node * Succ = NULL;
	std::vector<Abstract*> Join;
	bool only_join = false;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	if (U.count(b)) {
		U[b]->clear();
		V[b]->clear();
	} else {
		// this is a block without any successors...
		is_computed[n] = true;
		return;
	}
	
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
			*Dbg << "COMPUTENODE-------------- NEW SMT SOLVE -------------------------\n";
			resetColor();
		);
		LSMT->push_context();
		// creating the SMTpass formula we want to check
		SMT_expr pathtree_smt = pathtree[n->bb]->generateSMTformula(LSMT);
		SMT_expr smtexpr = LSMT->createSMTformula(b,false,passID,pathtree_smt);
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			*Dbg
				<< "\n"
				<< "FORMULA"
				<< "(COMPUTENODE)"
				<< "\n\n";
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res;
		res = LSMT->SMTsolve(smtexpr,&path,n->bb->getParent(),passID);

		LSMT->pop_context();
		if (res != 1 || path.size() == 1) {
			if (res == -1) {
				unknown = true;
				return;
			}
			break;
		}

		TIMEOUT(unknown = true; return;);

		PDEBUG(
			printPath(path);
		);
		Succ = Nodes[path.back()];
		
		asc_iterations[passID][n->bb->getParent()]++;

		// computing the image of the abstract value by the path's tranformation
		Xtemp = aman->NewAbstract(n->X_s[passID]);
		computeTransform(aman,n,path,Xtemp);
		PDEBUG(
			*Dbg << "POLYHEDRON AT THE STARTING NODE\n";
			n->X_s[passID]->print();
			*Dbg << "POLYHEDRON AFTER PATH TRANSFORMATION\n";
			Xtemp->print();
		);
		
		Environment Xtemp_env(Xtemp);
		Succ->X_s[passID]->change_environment(&Xtemp_env);

		// if we have a self loop, we apply loopiter
		if (Succ == n) {
			loopiter(n,Xtemp,&path,only_join,U[n->bb],V[n->bb]);
		} 
		Join.clear();
		Join.push_back(aman->NewAbstract(Succ->X_s[passID]));
		Join.push_back(aman->NewAbstract(Xtemp));
		Xtemp->join_array(&Xtemp_env,Join);

		Pr * FPr = Pr::getInstance(b->getParent());
		if (FPr->inPw(Succ->bb) && ((Succ != n) || !only_join)) {
				if (W->exist(path)) {
					if (use_threshold)
						Xtemp->widening_threshold(Succ->X_s[passID],threshold);
					else
						Xtemp->widening(Succ->X_s[passID]);
					PDEBUG(*Dbg << "WIDENING! \n";);
					W->clear();
				} else {
					W->insert(path);
				}
		} else {
			PDEBUG(*Dbg << "NO WIDENING\n";);
		}
		PDEBUG(
			*Dbg << "BEFORE:\n";
			Succ->X_s[passID]->print();
		);
		delete Succ->X_s[passID];

		// intersection with the previous invariant
		params P;
		P.T = SIMPLE;
		P.D = getApronManager();
		P.N = useNewNarrowing();
		P.TH = useThreshold();
		intersect_with_known_properties(Xtemp,Succ,P);

		Succ->X_s[passID] = Xtemp;
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
}

void AIopt::narrowNode(Node * n) {
	Abstract * Xtemp = NULL;
	Node * Succ;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	if (!pathtree.count(n->bb)) {
		// this is a block without any successors...
		is_computed[n] = true;
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
		SMT_expr pathtree_smt = pathtree[n->bb]->generateSMTformula(LSMT);
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,true,passID,pathtree_smt);
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			*Dbg
				<< "\n"
				<< "FORMULA"
				<< "(NARROWNODE)"
				<< "\n\n";
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res;
		res = LSMT->SMTsolve(smtexpr,&path,n->bb->getParent(),passID);

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
		Xtemp = aman->NewAbstract(n->X_s[passID]);
		computeTransform(aman,n,path,Xtemp);

		PDEBUG(
			*Dbg << "POLYHEDRON TO JOIN\n";
			Xtemp->print();
			*Dbg << "POLYHEDRON TO JOIN WITH\n";
			Succ->X_d[passID]->print();
		);

		if (Succ->X_d[passID]->is_bottom()) {
			delete Succ->X_d[passID];
			Succ->X_d[passID] = Xtemp;
		} else {
			std::vector<Abstract*> Join;
			Join.push_back(aman->NewAbstract(Succ->X_d[passID]));
			Join.push_back(Xtemp);
			Environment Xtemp_env(Xtemp);
			Succ->X_d[passID]->join_array(&Xtemp_env,Join);
		}
		PDEBUG(
			*Dbg << "RESULT\n";
			Succ->X_d[passID]->print();
		);
		Xtemp = NULL;
		A.push(Succ);
		is_computed[Succ] = false;
	}
}
