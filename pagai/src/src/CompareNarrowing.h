/**
 * \file CompareNarrowing.h
 * \brief Declaration of the CompareNarrowing pass
 * \author Julien Henry
 */
#ifndef COMPARENARROWING_H
#define COMPARENARROWING_H

#include "Compare.h"

#include "Pr.h"
#include "ModulePassWrapper.h"
#include "SMTpass.h"
#include "AIpf.h"
#include "AIpf_incr.h"
#include "AIopt.h"
#include "AIGopan.h"
#include "AIGuided.h"
#include "AIClassic.h"
#include "AIdis.h"
#include "Debug.h"

using namespace llvm;

/**
 * \class CompareNarrowing
 * \brief Compare the precision of the classical narrowing with Halbwach's
 * narrowing
 */
template<Techniques T>
class CompareNarrowing : public ModulePass {
	
	private:	
		SMTpass * LSMT;
	
		std::map<params, sys::TimeValue *> Time;
		std::map<params, sys::TimeValue *> Eq_Time;
		std::map<params, int> total_asc;
		std::map<params, int> total_desc;

		// count the number of warnings emitted by each technique
		std::map<params,int> Warnings;
		// count the number of safe properties emitted by each technique
		std::map<params,int> Safe_properties;

	public:
		/**
		 * \brief unique pass identifier
		 */
		static char ID;

		CompareNarrowing() : ModulePass(ID)
		{}

		~CompareNarrowing() {}
		
		void getAnalysisUsage(AnalysisUsage &AU) const;

		void AddTime(params P, Function * F);
		void AddEqTime(params P, Function * F);
		
		void printTime(params P);
		void printEqTime(params P);

		void ComputeIterations(params P, Function * F);
	
		void CountNumberOfWarnings(params P, Function * F);
		
		void printIterations(params P);

		bool runOnModule(Module &M);
	
		const char * getPassName() const;
};

template<Techniques T>
char CompareNarrowing<T>::ID = 0;
		
template<Techniques T>
const char * CompareNarrowing<T>::getPassName() const {
	return "CompareNarrowing";
}

template<Techniques T>
void CompareNarrowing<T>::getAnalysisUsage(AnalysisUsage &AU) const {
	switch(T) {
		case LOOKAHEAD_WIDENING:
			AU.addRequired<ModulePassWrapper<AIGopan, 0> >();
			AU.addRequired<ModulePassWrapper<AIGopan, 1> >();
			break;
		case GUIDED:
			AU.addRequired<ModulePassWrapper<AIGuided, 0> >();
			AU.addRequired<ModulePassWrapper<AIGuided, 1> >();
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
void CompareNarrowing<T>::ComputeIterations(params P, Function * F) {
	
	if (total_asc.count(P)) {
		total_asc[P] = total_asc[P] + asc_iterations[P][F];
		total_desc[P] = total_desc[P] + desc_iterations[P][F];
	} else {
		total_asc[P] = asc_iterations[P][F];
		total_desc[P] = desc_iterations[P][F];
	}
}

template<Techniques T>
void CompareNarrowing<T>::CountNumberOfWarnings(params P, Function * F) {
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
void CompareNarrowing<T>::printIterations(params P) {
	if (!total_asc.count(P)) {
		total_asc[P] = 0;
		total_desc[P] = 0;
	}
	*Out << total_asc[P] << " " << total_desc[P] << "\n";
}

template<Techniques T>
void CompareNarrowing<T>::AddEqTime(params P, Function * F) {
	
	if (Eq_Time.count(P)) {
		*Eq_Time[P] = *Eq_Time[P]+*Total_time[P][F];
	} else {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Eq_Time[P] = zero;
		*Eq_Time[P] = *Total_time[P][F];
	}
}

template<Techniques T>
void CompareNarrowing<T>::AddTime(params P, Function * F) {
	
	if (Time.count(P)) {
		*Time[P] = *Time[P]+*Total_time[P][F];
	} else {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time[P] = zero;
		*Time[P] = *Total_time[P][F];
	}
}

template<Techniques T>
void CompareNarrowing<T>::printEqTime(params P) {
	if (!Eq_Time.count(P)) {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Eq_Time[P] = zero;
	}
	std::string tname;
	if (P.N == true) tname = "NEWNARROWING";
	else tname = "CLASSIC";
	*Out 
		<< Eq_Time[P]->seconds() 
		<< " " << Eq_Time[P]->microseconds() 
		<< " // " << tname << "\n";
}

template<Techniques T>
void CompareNarrowing<T>::printTime(params P) {
	if (!Time.count(P)) {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time[P] = zero;
	}
	std::string tname;
	if (P.N == true) tname = "NEWNARROWING";
	else tname = "CLASSIC";
	*Out 
		<< Time[P]->seconds() 
		<< " " << Time[P]->microseconds() 
		<< " // " << tname << "\n";
}

template<Techniques T>
bool CompareNarrowing<T>::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = SMTpass::getInstance();

	int gt = 0;
	int lt = 0;
	int eq = 0;
	int un = 0;

	int F_equal = 0;
	int F_distinct = 0;
	int F_noseeds = 0;
	int F_withseeds = 0;

	params P1, P2;
	P1.T = T;
	P2.T = T;
	P1.D = getApronManager(0);
	P2.D = getApronManager(1);
	P1.N = useNewNarrowing(0);
	P2.N = useNewNarrowing(1);
	P1.TH = useThreshold(0);
	P2.TH = useThreshold(1);

	changeColor(raw_ostream::BLUE);
	*Dbg << "\n\n\n"
			<< "---------------------------------\n"
			<< "-      COMPARING NARROWING      -\n"
			<< "---------------------------------\n";
	resetColor();

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		LSMT->reset_SMTcontext();
		F = &*mIt;
		
		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;

		if (ignored(F)) continue;
		
		AddTime(P1,F);
		AddTime(P2,F);
		ComputeIterations(P1,F);
		ComputeIterations(P2,F);
		CountNumberOfWarnings(P1,F);
		CountNumberOfWarnings(P2,F);

		bool distinct = false;
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
				switch (n->X_s[P1]->compare(n->X_s[P2])) {
					case 0:
						eq++;
						break;
					case 1:
						distinct = true;
						lt++;
						break;
					case -1:
						distinct = true;
						gt++;
						break;
					case -2:
						distinct = true;
						un++;
						break;
					default:
						distinct = true;
						break;
				}
			}
		}

		if (!distinct) {
			AddEqTime(P1,F);
			AddEqTime(P2,F);
			F_equal++;
		} else {
			F_distinct++;
		}

		if (numNarrowingSeedsInFunction[F] == 0) {
			F_noseeds++;
		} else {
			F_withseeds++;
		}
	}

	changeColor(raw_ostream::MAGENTA);
	*Out << ApronManagerToString(getApronManager(0)) << " ABSTRACT DOMAIN\n\n\n" 
		<< "IMPROVED NARROWING -- CLASSIC" << "\n";
	resetColor();
	*Out << "\nTIME:\n";
	printTime(P1);
	printTime(P2);
	*Out << "TIME_END\n";
	
	*Out << "\nTIME_EQ:\n";
	printEqTime(P1);
	printEqTime(P2);
	*Out << "TIME_EQ_END\n";

	*Out << "\nFUNCTIONS:\n";
	*Out << F_equal+F_distinct << "\n";
	*Out << "FUNCTIONS_END\n";

	*Out << "\nFUNCTIONS_SEEDS:\n";
	*Out 
		<< F_equal 
		<< " " << F_distinct 
		<< " " << F_withseeds
		<< " " << F_noseeds
		<<  " " << F_equal+F_distinct << "\n";
	*Out << "FUNCTIONS_SEEDS_END\n";

	*Out << "\nITERATIONS:\n";
	printIterations(P1);
	printIterations(P2);
	*Out << "ITERATIONS_END\n";

	*Out << "\nWARNINGS:\n";
	*Out << Warnings[P1] << " // NEWNARROWING\n";
	*Out << Warnings[P2] << " // CLASSIC\n";
	*Out << "WARNINGS_END\n";

	*Out << "\nSAFE_PROPERTIES:\n";
	*Out << Safe_properties[P1] << " // NEWNARROWING\n";
	*Out << Safe_properties[P2] << " // CLASSIC\n";
	*Out << "SAFE_PROPERTIES_END\n";

	*Out << "\n\nMATRIX:\n";
	*Out << eq << " " << lt << " " << gt << " " << un << " // NEWNARROWING / CLASSIC\n";
	*Out << "MATRIX_END\n";
	return true;
}
#endif
