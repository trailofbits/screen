/**
 * \file Live.h
 * \brief Declaration of the Live class
 * \author Julien Henry
 */
#ifndef LIVE_H
#define LIVE_H


#if LLVM_VERSION_MAJOR>3 || LLVM_VERSION_MAJOR==3 && LLVM_VERSION_MINOR>=6
#include "llvm/Analysis/CFG.h"
#else
#include "llvm/Analysis/CFG.h"
#endif

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

/**
 * \class Live
 * \brief Liveness analysis
 *
 * Analysis that provides liveness information for
 * LLVM IR Values.
 */
class Live : public FunctionPass {

	private:

		//LoopInfo *LI;

		/** 
		 * \brief A bunch of state to be associated with a value.
		 */
		struct Memo {
			/** 
			 * \brief The set of blocks which contain a use of the value.
			 */
			SmallPtrSet< BasicBlock *, 4> Used;
			SmallPtrSet< BasicBlock *, 4> UsedPHI;

			/** 
			 * \brief A conservative approximation of the set of blocks in
			 * which the value is live-through, meaning blocks dominated
			 * by the definition, and from which blocks containing uses of the
			 * value are reachable.
			 */
			SmallPtrSet< BasicBlock *, 4> LiveThrough;
			SmallPtrSet< BasicBlock *, 4> LiveThroughPHI;
		};

		/** 
		 * \brief Remembers the Memo for each Value. This is populated on
		 * demand.
		 */
		DenseMap< Value *, Memo> Memos;

		/** 
		 * \brief Retrieve an existing Memo for the given value if one
		 * is available, otherwise compute a new one.
		 */
		Memo &getMemo( Value *V);

		/** 
		 * \brief Compute a new Memo for the given value.
		 */
		Memo &compute( Value *V);

	public:
		static char ID;
		Live();

		const char * getPassName() const;
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnFunction(Function &F);
		virtual void releaseMemory();

		/** 
		 * \brief Test if the given value is used in the given block.
		 */
		bool isUsedInBlock( Value *V,  BasicBlock *BB);
		bool isUsedInPHIBlock( Value *V,  BasicBlock *BB);

		bool isLiveByLinearityInBlock(Value *V, BasicBlock *BB, bool PHIblock);

		/** 
		 * \brief Test if the given value is known to be
		 * live-through the given block
		 *
		 * Live through means that the block is properly
		 * dominated by the value's definition, and there exists a block
		 * reachable from it that contains a use. 	
		 */
		bool isLiveThroughBlock( Value *V,  BasicBlock *BB, bool PHIblock);
};


#endif

