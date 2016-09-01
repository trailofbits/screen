/**
 * \file AIopt_incr.h
 * \brief Declaration of the AIopt_incr class
 * \author Julien Henry
 */
#ifndef _AIOPT_INCR_H
#define _AIOPT_INCR_H

#include <queue>
#include <vector>

#include "AIopt.h"

using namespace llvm;

/**
 * \class AIopt_incr
 * \brief AIopt Implementation that uses the result of a previous analysis
 */
class AIopt_incr : public AIopt {

	private:
		void init()
			{
				passID.T = COMBINED_INCR;
			}

	public:
		static char ID;	

	public:

		AIopt_incr(char &_ID, Apron_Manager_Type _man, bool _NewNarrow, bool _Threshold) : AIopt(_ID,_man,_NewNarrow,_Threshold) {
			init();
		}
		
		AIopt_incr() : AIopt(ID) {
			init();
		}

		~AIopt_incr () {}

		const char *getPassName() const;

		void getAnalysisUsage(AnalysisUsage &AU) const;

		void assert_properties(params P, Function * F);
		void intersect_with_known_properties(Abstract * Xtemp, Node * n, params P);
};

#endif
