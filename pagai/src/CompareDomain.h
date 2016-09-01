/**
 * \file CompareDomain.h
 * \brief Declaration of the CompareDomain template class
 * \author Julien Henry
 */
#ifndef COMPAREDOMAIN_H
#define COMPAREDOMAIN_H

#include "Compare.h"

#include "Pr.h"
#include "ModulePassWrapper.h"
#include "SMTpass.h"
#include "AIpf.h"
#include "AIpf_incr.h"
#include "AIopt.h"
#include "AIGopan.h"
#include "AIClassic.h"
#include "AIdis.h"
#include "Debug.h"
#include "Compare.h"

using namespace llvm;

/**
 * \class CompareDomain
 * \brief compare the precision of two abstract domains
 */
template<Techniques T>
class CompareDomain : public ModulePass {
	
	private:	
		SMTpass * LSMT;
	
		std::map<params, sys::TimeValue *> Time;
		std::map<params, sys::TimeValue *> Time_SMT;
		void ComputeTime(params P, Function * F);
		void CountNumberOfWarnings(params P, Function * F);
		void printTime(params P);

		// count the number of warnings emitted by each technique
		std::map<params,int> Warnings;
		// count the number of safe properties emitted by each technique
		std::map<params,int> Safe_properties;

	public:
		/**
		 * \brief unique pass identifier
		 *
		 * It is crucial for LLVM's pass manager that
		 * this ID is different (in address) from a class to another,
		 * but the template instantiation mechanism will make sure it
		 * is the case.
		 */
		static char ID;

		CompareDomain() : ModulePass(ID)
		{}

		~CompareDomain() {}
		
		void getAnalysisUsage(AnalysisUsage &AU) const;

		const char * getPassName() const;

		bool runOnModule(Module &M);
};

template<Techniques T>
char CompareDomain<T>::ID = 0;
		
template<Techniques T>
const char * CompareDomain<T>::getPassName() const {
	return "CompareDomain";
}

template<Techniques T>
void CompareDomain<T>::getAnalysisUsage(AnalysisUsage &AU) const {
	switch(T) {
		case LOOKAHEAD_WIDENING:
			AU.addRequired<ModulePassWrapper<AIGopan, 0> >();
			AU.addRequired<ModulePassWrapper<AIGopan, 1> >();
			break;
		case PATH_FOCUSING:
			AU.addRequired<ModulePassWrapper<AIpf, 0> >();
			AU.addRequired<ModulePassWrapper<AIpf, 1> >();
			break;
		case PATH_FOCUSING_INCR:
			AU.addRequired<ModulePassWrapper<AIpf_incr, 0> >();
			AU.addRequired<ModulePassWrapper<AIpf_incr, 1> >();
			break;
		case LW_WITH_PF:
			AU.addRequired<ModulePassWrapper<AIopt, 0> >();
			AU.addRequired<ModulePassWrapper<AIopt, 1> >();
			break;
		case COMBINED_INCR:
			AU.addRequired<ModulePassWrapper<AIopt_incr, 0> >();
			AU.addRequired<ModulePassWrapper<AIopt_incr, 1> >();
			break;
		case SIMPLE:
			AU.addRequired<ModulePassWrapper<AIClassic, 0> >();
			AU.addRequired<ModulePassWrapper<AIClassic, 1> >();
			break;
		case LW_WITH_PF_DISJ:
			AU.addRequired<ModulePassWrapper<AIdis, 0> >();
			AU.addRequired<ModulePassWrapper<AIdis, 1> >();
			break;
	}
	AU.setPreservesAll();
}

template<Techniques T>
void CompareDomain<T>::ComputeTime(params P, Function * F) {

	if (Total_time[P].count(F) == 0) {
		sys::TimeValue * time = new sys::TimeValue(0,0);
		Total_time[P][F] = time;
	}
	
	if (Time.count(P)) {
		assert(Time[P] != NULL);
		assert(Total_time[P][F] != NULL);
		*Time[P] = *Time[P]+*Total_time[P][F];
	} else {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time[P] = zero;
		assert(Total_time[P][F] != NULL);
		*Time[P] = *Total_time[P][F];
	}

	if (Total_time_SMT[P].count(F) == 0) {
		sys::TimeValue * time_SMT = new sys::TimeValue(0,0);
		Total_time_SMT[P][F] = time_SMT;
	}

	if (Time_SMT.count(P)) {
		assert(Time_SMT[P] != NULL);
		assert(Total_time_SMT[P][F] != NULL);
		*Time_SMT[P] = *Time_SMT[P]+*Total_time_SMT[P][F];
	} else {
		assert(Total_time_SMT[P][F] != NULL);
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time_SMT[P] = zero;
		*Time_SMT[P] = *Total_time_SMT[P][F];
	}
}

template<Techniques T>
void CompareDomain<T>::printTime(params P) {
	if (!Time.count(P)) {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time[P] = zero;
	}
	if (!Time_SMT.count(P)) {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time_SMT[P] = zero;
	}
	*Out 
		<< Time[P]->seconds() 
		<< " " << Time[P]->microseconds() 
		<< " " << Time_SMT[P]->seconds() 
		<< " " << Time_SMT[P]->microseconds() 
		<< " " << ApronManagerToString(P.D)
		<<  "\n";
}

template<Techniques T>
void CompareDomain<T>::CountNumberOfWarnings(params P, Function * F) {
	BasicBlock * b;
	Node * n;
	Pr * FPr = Pr::getInstance(F);
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = &*i;
		n = Nodes[b];
		if (FPr->getAssert()->count(b) || FPr->getUndefinedBehaviour()->count(b)) {
			if (!n->X_s[P]->is_bottom()) {
				if (Warnings.count(P))
					Warnings[P]++;
				else
					Warnings[P] = 1;
			} else {
				if (Safe_properties.count(P))
					Safe_properties[P]++;
				else
					Safe_properties[P] = 1;
			}
		}
	}
}

template<Techniques T>
bool CompareDomain<T>::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = SMTpass::getInstance();

	int gt = 0;
	int lt = 0;
	int eq = 0;
	int un = 0;

	changeColor(raw_ostream::BLUE);
	*Out << "\n\n\n"
			<< "---------------------------------\n"
			<< "-   COMPARING ABSTRACT DOMAINS  -\n"
			<< "---------------------------------\n";
	resetColor();

	params P1, P2;
	P1.T = T;
	P2.T = T;
	P1.D = getApronManager(0);
	P2.D = getApronManager(1);
	P1.N = useNewNarrowing(0);
	P2.N = useNewNarrowing(1);
	P1.TH = useThreshold(0);
	P2.TH = useThreshold(1);

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		LSMT->reset_SMTcontext();
		F = &*mIt;
		
		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;

		if (ignored(F)) continue;
		
		ComputeTime(P1,F);
		ComputeTime(P2,F);
		CountNumberOfWarnings(P1,F);
		CountNumberOfWarnings(P2,F);

		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = &*i;
			n = Nodes[b];
			Pr * FPr = Pr::getInstance(F);
			if (FPr->getPw()->count(b)) {
				PDEBUG(
				*Out << "Comparing the two abstracts :\n";
				n->X_s[P1]->print();
				n->X_s[P2]->print();
				);
				switch (Compare::compareAbstract(LSMT,n->X_s[P1],n->X_s[P2])) {
					case 0:
						eq++;
						break;
					case 1:
						lt++;
						break;
					case -1:
						gt++;
						break;
					case -2:
						un++;
						break;
					default:
						break;
				}
			}
		}
	}

	changeColor(raw_ostream::MAGENTA);
	*Out << ApronManagerToString(getApronManager(0)) << " - " 
		<< ApronManagerToString(getApronManager(1)) << "\n";
	resetColor();
	*Out << "\n";
	*Out << "EQ " << eq << "\n";
	*Out << "LT " << lt << "\n";
	*Out << "GT " << gt << "\n";
	*Out << "UN " << un << "\n";

	*Out << "\nTIME:\n";
	printTime(P1);
	printTime(P2);
	*Out << "TIME_END\n";

	*Out << "\n\nTECHNIQUE:\n";
	*Out << TechniquesToString(T);
	*Out << "\nTECHNIQUE_END\n";

	*Out << "\nWARNINGS:\n";
	*Out << Warnings[P1] << " // " << ApronManagerToString(getApronManager(0)) << "\n";
	*Out << Warnings[P2] << " // " << ApronManagerToString(getApronManager(1)) << "\n";
	*Out << "WARNINGS_END\n";

	*Out << "\nSAFE_PROPERTIES:\n";
	*Out << Safe_properties[P1] << " // " << ApronManagerToString(getApronManager(0)) << "\n";
	*Out << Safe_properties[P2] << " // " << ApronManagerToString(getApronManager(1)) << "\n";
	*Out << "SAFE_PROPERTIES_END\n";

	*Out << "\n\nMATRIX:\n";
	*Out << eq << " " << lt << " " << gt << " " << un << " ";
	*Out << ApronManagerToString(getApronManager(0)) << " // " 
		<< ApronManagerToString(getApronManager(1)) << "\n";
	*Out << "MATRIX_END\n";
	return true;
}
#endif
