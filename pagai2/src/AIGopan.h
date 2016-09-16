/**
 * \file AIGopan.h
 * \brief Declaration of the AIGopan class
 * \author Julien Henry
 */
#ifndef _AIGOPAN_H
#define _AIGOPAN_H

#include "AISimple.h"

using namespace llvm;

/**
 * \class AIGopan
 * \brief Gopan&Reps Implementation.
 *
 * This class is almost identical to AIClassic, the only difference
 * being the abstract domain (which uses a main value to decide which
 * paths have to be explored, and a pilot value to actually compute
 * the invariants).
 */
class AIGopan : public AISimple {

	public:
		/**
		 * \brief Pass Identifier
		 *
		 * It is crucial for LLVM's pass manager that
		 * this ID is different (in address) from a class to another,
		 * hence this cannot be factored in the base class.
		 */
		static char ID;	

	public:

		AIGopan(char &_ID, Apron_Manager_Type _man, bool _NewNarrow, bool _Threshold) : AISimple(_ID,_man,_NewNarrow, _Threshold) {
			init();
			passID.D = _man;
			passID.N = _NewNarrow;
			passID.TH = _Threshold;
		}
		
		AIGopan (): AISimple(ID) {
			init();
			passID.D = getApronManager();
			passID.N = useNewNarrowing();
			passID.TH = useThreshold();
		}

		void init()
			{
				aman = new AbstractManGopan();
				passID.T = LOOKAHEAD_WIDENING;
			}

		~AIGopan () {
			}

		const char *getPassName() const;
};

#endif
