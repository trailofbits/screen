/**
 * \file Compare.cc
 * \brief Implementation of the Compare class
 * \author Julien Henry
 */
#include "Compare.h"
#include "Pr.h"
#include "Expr.h"
#include "AIpf.h"
#include "AIpf_incr.h"
#include "AIopt.h"
#include "AIopt_incr.h"
#include "AIGopan.h"
#include "AIGuided.h"
#include "AIClassic.h"
#include "AIdis.h"
#include "Node.h"
#include "Debug.h"
#include "ModulePassWrapper.h"

using namespace llvm;

char Compare::ID = 0;
static RegisterPass<Compare>
X("compare", "Abstract values comparison pass", false, true);

const char * Compare::getPassName() const {
	return "Compare";
}

Compare::Compare() : ModulePass(ID) {}

Compare::Compare(std::vector<enum Techniques> * T) : ModulePass(ID) {
	std::vector<enum Techniques>::iterator it = T->begin(), et = T->end();
	for (; it!=et; it++) {
		ComparedTechniques.push_back(*it);
	}
}

void Compare::getAnalysisUsage(AnalysisUsage &AU) const {
	for (int i = 0; i < ComparedTechniques.size(); i++) {
		switch (ComparedTechniques[i]) {
			case SIMPLE:
				AU.addRequired<ModulePassWrapper<AIClassic, 0> >();
				break;
			case LOOKAHEAD_WIDENING:
				AU.addRequired<ModulePassWrapper<AIGopan, 0> >();
				break;
			case GUIDED:
				AU.addRequired<ModulePassWrapper<AIGuided, 0> >();
				break;
			case PATH_FOCUSING:
				AU.addRequired<ModulePassWrapper<AIpf, 0> >();
				break;
			case PATH_FOCUSING_INCR:
				AU.addRequired<ModulePassWrapper<AIpf_incr, 0> >();
				break;
			case LW_WITH_PF:
				AU.addRequired<ModulePassWrapper<AIopt, 0> >();
				break;
			case COMBINED_INCR:
				AU.addRequired<ModulePassWrapper<AIopt_incr, 0> >();
				break;
			case LW_WITH_PF_DISJ:
				AU.addRequired<ModulePassWrapper<AIdis, 0> >();
				break;
		}
	}
	AU.setPreservesAll();
}

int Compare::compareAbstract(SMTpass * LSMT, Abstract * A, Abstract * B) {
	bool f = false;
	bool g = false;

	if (A == NULL || B == NULL) return -3;
	Environment A_env(A);
	Environment B_env(B);
	Environment cenv = Environment::intersection(&A_env,&B_env);

	A->change_environment(&cenv);
	B->change_environment(&cenv);

	LSMT->push_context();
	SMT_expr A_smt = LSMT->AbstractToSmt(NULL,A);
	SMT_expr B_smt = LSMT->AbstractToSmt(NULL,B);

	LSMT->push_context();
	// f = A and not B
	std::vector<SMT_expr> cunj;
	cunj.push_back(A_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(B_smt));
	SMT_expr test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		f = true;
	}
	LSMT->pop_context();

	// g = B and not A
	cunj.clear();
	cunj.push_back(B_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(A_smt));
	test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		g = true;
	}
	LSMT->pop_context();

	if (!f && !g) {
		return 0;
	} else if (!f && g) {
		return 1;
	} else if (f && !g) {
		PDEBUG(
			*Dbg << "############################\n";
			B->print();
			*Dbg << "is leq than \n";
			A->print();
			*Dbg << "############################\n";
		);
		return -1;
	} else {
		return -2;
	}
}

void Compare::compareTechniques(Node * n, Techniques t1, Techniques t2) {

	params P1, P2;
	P1.T = t1;
	P2.T = t2;
	P1.D = getApronManager();
	P2.D = getApronManager();
	P1.N = useNewNarrowing();
	P2.N = useNewNarrowing();
	P1.TH = useThreshold();
	P2.TH = useThreshold();

	switch (compareAbstract(LSMT,n->X_s[P1],n->X_s[P2])) {
		case 0:
			results[t1][t2].eq++;
			results[t2][t1].eq++;
			break;
		case 1:
			results[t1][t2].lt++;
			results[t2][t1].gt++;
			break;
		case -1:
			results[t1][t2].gt++;
			results[t2][t1].lt++;
			break;
		case -2:
			results[t1][t2].un++;
			results[t2][t1].un++;
			break;
		default:
			break;
	}
}

void Compare::ComputeTime(Techniques t, Function * F) {
	params P;
	P.T = t;
	P.D = getApronManager();
	P.N = useNewNarrowing();
	P.TH = useThreshold();

	if (Total_time[P].count(F) == 0) {
		sys::TimeValue * time = new sys::TimeValue(0,0);
		Total_time[P][F] = time;
	}
	
	if (Time.count(t)) {
		assert(Time[t] != NULL);
		assert(Total_time[P][F] != NULL);
		*Time[t] = *Time[t]+*Total_time[P][F];
	} else {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time[t] = zero;
		assert(Total_time[P][F] != NULL);
		*Time[t] = *Total_time[P][F];
	}

	if (Total_time_SMT[P].count(F) == 0) {
		sys::TimeValue * time_SMT = new sys::TimeValue(0,0);
		Total_time_SMT[P][F] = time_SMT;
	}

	if (Time_SMT.count(t)) {
		assert(Time_SMT[t] != NULL);
		assert(Total_time_SMT[P][F] != NULL);
		*Time_SMT[t] = *Time_SMT[t]+*Total_time_SMT[P][F];
	} else {
		assert(Total_time_SMT[P][F] != NULL);
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time_SMT[t] = zero;
		*Time_SMT[t] = *Total_time_SMT[P][F];
	}
}

void Compare::printTime(Techniques t) {
	if (!Time.count(t)) {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time[t] = zero;
	}
	if (!Time_SMT.count(t)) {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time_SMT[t] = zero;
	}
	*Dbg 
		<< Time[t]->seconds() 
		<< " " << Time[t]->microseconds() 
		<< " " << Time_SMT[t]->seconds() 
		<< " " << Time_SMT[t]->microseconds() 
		<< "  \t// " << TechniquesToString(t) 
		<<  "\n";
}

void Compare::printWarnings(Techniques t) {
	if (!Warnings.count(t)) {

		Warnings[t] = 0;
	}

	*Dbg 
		<< Warnings[t] 
		<< "  \t// " << TechniquesToString(t) 
		<< "\n";
}

void Compare::printSafeProperties(Techniques t) {
	if (!Safe_properties.count(t)) {

		Safe_properties[t] = 0;
	}

	*Dbg 
		<< Safe_properties[t] 
		<< "  \t// " << TechniquesToString(t) 
		<< "\n";
}

void Compare::printNumberSkipped(Techniques t) {
	params P;
	P.T = t;
	P.D = getApronManager();
	P.N = useNewNarrowing();
	P.TH = useThreshold();

	*Dbg 
		<< ignoreFunction[P].size() 
		<< "  \t// " << TechniquesToString(t) 
		<< "\n";
}

void Compare::printResults(Techniques t1, Techniques t2) {

	Out->changeColor(raw_ostream::MAGENTA,true);
	*Dbg << TechniquesToString(t1) << " - " << TechniquesToString(t2) << "\n";
	resetColor();
	*Dbg << "\n";
	*Dbg << "EQ " << results[t1][t2].eq << "\n";
	*Dbg << "LT " << results[t1][t2].lt << "\n";
	*Dbg << "GT " << results[t1][t2].gt << "\n";
	*Dbg << "UN " << results[t1][t2].un << "\n";
}

void Compare::printAllResults() {

	*Dbg << "\nSKIPPED:\n";
	for (int i = 0; i < ComparedTechniques.size(); i++) {
		printNumberSkipped(ComparedTechniques[i]);
	}
	*Dbg << "SKIPPED_END\n";

	*Dbg << "\nTIME:\n";
	for (int i = 0; i < ComparedTechniques.size(); i++) {
		printTime(ComparedTechniques[i]);
	}
	*Dbg << "TIME_END\n";

	*Dbg << "\nWARNINGS:\n";
	for (int i = 0; i < ComparedTechniques.size(); i++) {
		printWarnings(ComparedTechniques[i]);
	}
	*Dbg << "WARNINGS_END\n";

	*Dbg << "\nSAFE_PROPERTIES:\n";
	for (int i = 0; i < ComparedTechniques.size(); i++) {
		printSafeProperties(ComparedTechniques[i]);
	}
	*Dbg << "SAFE_PROPERTIES_END\n";


	*Dbg	<< "\n";
	*Dbg	<< "MATRIX:\n";

	for (int i = 0; i < ComparedTechniques.size(); i++) {
		for (int j = i+1; j < ComparedTechniques.size(); j++) {
			*Dbg	<< results[ComparedTechniques[i]][ComparedTechniques[j]].eq << " "
					<< results[ComparedTechniques[i]][ComparedTechniques[j]].lt << " "
					<< results[ComparedTechniques[i]][ComparedTechniques[j]].gt << " "
					<< results[ComparedTechniques[i]][ComparedTechniques[j]].un << " "
					<< "  \t// "<< TechniquesToString(ComparedTechniques[i]) << " / " << TechniquesToString(ComparedTechniques[j]) 
					<< "\n";
		}
	}
	*Dbg	<< "MATRIX_END\n";
}

void Compare::CompareTechniquesByPair(Node * n) {
	for (int i = 0; i < ComparedTechniques.size(); i++) {
		for (int j = i+1; j < ComparedTechniques.size(); j++) {
			compareTechniques(n,ComparedTechniques[i],ComparedTechniques[j]);
		}
	}
}

void Compare::PrintResultsByPair() {
	for (int i = 0; i < ComparedTechniques.size(); i++) {
		for (int j = i+1; j < ComparedTechniques.size(); j++) {
			printResults(ComparedTechniques[i],ComparedTechniques[j]);
		}
	}
}


void Compare::CountNumberOfWarnings(Techniques t, Function * F) {
	Node * n;
	params P;
	P.D = getApronManager();
	P.N = useNewNarrowing();
	P.TH = useThreshold();
	P.T = t;
	Pr * FPr = Pr::getInstance(F);
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		auto b = i;
		n = Nodes[&*b];
		if (FPr->getAssert()->count(&*b) || FPr->getUndefinedBehaviour()->count(&*b)) {
			if (!n->X_s[P]->is_bottom()) {
				if (Warnings.count(t))
					Warnings[t]++;
				else
					Warnings[t] = 1;
			} else {
				if (Safe_properties.count(t))
					Safe_properties[t]++;
				else
					Safe_properties[t] = 1;
			}
		}
	}
}

bool Compare::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	int Function_number = 0;
	LSMT = SMTpass::getInstanceForAbstract();

	Out->changeColor(raw_ostream::BLUE,true);
	*Dbg << "\n\n\n"
			<< "------------------------------------------\n"
			<< "-         COMPARING RESULTS              -\n"
			<< "------------------------------------------\n";
	resetColor();

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		//LSMT->reset_SMTcontext();
		F = &*mIt;
		
		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;
		Function_number++;

		if (ignored(F)) continue;

		// we now count the computing time and the number of warnings
		for (int i = 0; i < ComparedTechniques.size(); i++) {
			ComputeTime(ComparedTechniques[i],F);
			CountNumberOfWarnings(ComparedTechniques[i],F);
		}

		Pr * FPr = Pr::getInstance(F);
		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = &*i;
			n = Nodes[b];
			if (FPr->getPw()->count(b) 
					|| FPr->getUndefinedBehaviour()->count(b)
					|| FPr->getAssert()->count(b)
					) {
				CompareTechniquesByPair(n);
			}
		}
	}
	PrintResultsByPair();

	*Dbg << "\nFUNCTIONS:\n";
	*Dbg << Function_number << "\nFUNCTIONS_END\n";
	*Dbg << "\nIGNORED:\n";
	*Dbg << nb_ignored() << "\nIGNORED_END\n";
	printAllResults();
	return true;
}

