/**
 * \file Live.cc
 * \brief Implementation of the Live class
 * \author Julien Henry
 */
#include <stack>

#include "llvm/IR/Instructions.h"
#include "llvm/Support/FormattedStream.h"

#include "Live.h"
#include "Analyzer.h"
#include "Debug.h"

using namespace llvm;

char Live::ID = 0;
static RegisterPass<Live>
X("live-values", "Value Liveness Analysis", false, true);


const char * Live::getPassName() const {
	return "Live";
}

Live::Live() : FunctionPass(ID) {}

void Live::getAnalysisUsage(AnalysisUsage &AU) const {
	//AU.addRequired<LoopInfo>();
	AU.setPreservesAll();
}

bool Live::runOnFunction(Function &F) {
	//LI = &getAnalysis<LoopInfo>();

	// This pass' values are computed lazily, so there's nothing to do here.
	return false;
}

void Live::releaseMemory() {
	Memos.clear();
}

bool Live::isUsedInBlock(Value *V, BasicBlock *BB) {
	Memo &M = getMemo(V);
	return M.Used.count(BB);
}

bool Live::isUsedInPHIBlock(Value *V, BasicBlock *BB) {
	Memo &M = getMemo(V);
	return M.UsedPHI.count(BB);
}


bool Live::isLiveByLinearityInBlock(Value *V, BasicBlock *BB, bool PHIblock) {
	if (Argument * arg = dyn_cast<Argument>(V))
		return true;
	if (isLiveThroughBlock(V,BB,PHIblock)) {
		return true;
	} else {
		for (Value::use_iterator I = V->use_begin(), E = V->use_end();
				I != E; ++I) {
		  User *U = I->getUser();
			// if the use is an operand of a linear binary operation, then we should keep
			// it live
			if (BinaryOperator * binop = dyn_cast<BinaryOperator>(U)) {
				switch (binop->getOpcode()) {
					case Instruction::Add : 
					case Instruction::FAdd: 
					case Instruction::Sub : 
					case Instruction::FSub: 
						if (isLiveByLinearityInBlock(binop,BB,PHIblock)) 
							return true;
						break;
					default:
						break;
				}
			}
			if (pointer_arithmetic()) {
				// IF WE USE POINTER ARITHMETIC:
				// if the use serves for computing a address using a getelementptr intstruction
				// then the result of this getelementptr will be a linear expression involving
				// the variable. Then, this case is similar to a standard addition, and we 
				// have to keep the variable live
				// (Recall that pointers are considered integers)
				if (GetElementPtrInst * geteltptr = dyn_cast<GetElementPtrInst>(U)) {
					if (isLiveByLinearityInBlock(geteltptr,BB,PHIblock)) 
						return true;
				}
			}

		}
	}
	//*Out << *V << "is NOT Live Through " << BB << "\n";
	return false;
}

bool Live::isLiveThroughBlock( Value *V,
		BasicBlock *BB, bool PHIblock) {
	Memo &M = getMemo(V);
	if (PHIblock)
		return M.LiveThroughPHI.count(BB);
	else
		return M.LiveThrough.count(BB);
}


Live::Memo &Live::getMemo( Value *V) {
	DenseMap< Value *, Memo>::iterator I = Memos.find(V);
	if (I != Memos.end())
		return I->second;
	return compute(V);
}


struct Block {
	BasicBlock * b;
	bool PHIblock;

	Block(BasicBlock * _b, bool _PHIblock) {b = _b; PHIblock = _PHIblock;}
};

Live::Memo &Live::compute( Value *V) {
	Memo &M = Memos[V];

	// if V is a PHINode, it is defined in the PHI block of DefBB
	bool PHIblock = isa<PHINode>(V);

	// Determine the block containing the definition.
	BasicBlock *DefBB;
	// Instructions define values with meaningful live ranges.
	if ( Instruction *I = dyn_cast<Instruction>(V))
		DefBB = I->getParent();
	// Arguments can be analyzed as values defined in the entry block.
	else if ( Argument *A = dyn_cast<Argument>(V))
		DefBB = &A->getParent()->getEntryBlock();
	// Constants and other things aren't meaningful here, so just
	// return having computed an empty Memo so that we don't come
	// here again. The assumption here is that client code won't
	// be asking about such values very often.
	else
		return M;

	Block DefB(DefBB,PHIblock);

	// Examine each use of the value.
	std::stack<Block> S;
	for (Value::use_iterator I = V->use_begin(), E = V->use_end();
			I != E; ++I) {
	  User *U = I->getUser();
		BasicBlock *UseBB = cast<Instruction>(U)->getParent();

		// Note the block in which this use occurs.
		if (isa<PHINode>(U)) {
			M.UsedPHI.insert(UseBB);
		} else {
			M.Used.insert(UseBB);
		}

		if (PHINode * phi = dyn_cast<PHINode>(U)) {
			// The value is used by a PHI, so it is live-out of the defining block.

			// we should add to LiveThrough Block the predecessors of the block,
			// which comes to the right path (phi-variable)

			//M.LiveThrough.insert(UseBB);

			Block Pred(phi->getIncomingBlock(*I),false);
			S.push(Pred);

		} else if (UseBB != DefB.b || DefB.PHIblock) {

			// We add to LiveThrough blocks all the blocks that are located
			// between the definition of the value and its use.
			Block B(UseBB,false);
			S.push(B);
		}
	}

	while (!S.empty()) {
		Block BB = S.top();
		S.pop();
		if (BB.PHIblock == true) {
			if (!M.LiveThroughPHI.count(BB.b)) {
				M.LiveThroughPHI.insert(BB.b);
				if (BB.b != DefB.b || BB.PHIblock != DefB.PHIblock) {
					for (pred_iterator p = pred_begin(BB.b), e = pred_end(BB.b); 
							p != e; 
							++p){
						Block Pred(*p,false);
						S.push(Pred);
					}
				}
			}
		} else {
			if (!M.LiveThrough.count(BB.b)) {
				M.LiveThrough.insert(BB.b);
				if (BB.b != DefB.b || BB.PHIblock != DefB.PHIblock) {
					Block Pred(BB.b,true);
					S.push(Pred);
				}
			}
		}
	}

	return M;
}

