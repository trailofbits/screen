/*
 *
 * Requirements :
 * 	-	Every static analysis must extend the StaticAnalysis class.
 * 	-	The listNode structure is used to store the results of the analysis.
 *
 * 	Notice that we assume all static analyses use a function scope, in accordance with the Professor's instructions.
 */

#ifndef CONSTANT_PROP_ANALYSIS
#define CONSTANT_PROP_ANALYSIS
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "RangeAnalysisFlow.h"
#include "StaticAnalysis.h"
#include <map>
#include <vector>
#include <cstdlib>
#include <queue>
#include <sstream>
#include <set>
#include <llvm/IR/Constants.h>

using namespace llvm;
using namespace std;

typedef struct _nodeState
{
	int nodeVisitCounter;
	RangeAnalysisFlow* nodeSet;
	_nodeState(){nodeVisitCounter = 0;};
}nodeState;

//Static Analysis class
class RangeAnalysis : public StaticAnalysis {

public :

	RangeAnalysis(Function &F);
	/*
	 * This method is called by the run worklist algorithm.
	 * It has the responsability to figure out what kind of instruction is being used and how to generate the output flow from the input flow for
	 * that instruction.
	 * It is expected this function will call other functions created by the subclasses to deal with each type of instruction.
	 *
	 * The output is a Flow that is the result of the processing of in with respect to instruction inst.
	 */
	Flow* executeFlowFunction(Flow *in, Instruction *inst, int NodeId);

	Flow* initialize();

protected:
	RangeAnalysisFlow *executeFAddInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executeAddInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executeFSubInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executeSubInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executeFMulInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executeMulInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executeFDivInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executeSDivInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executeCastInst(RangeAnalysisFlow* in, Instruction* inst);


	RangeAnalysisFlow *executeFOpInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executeOpInst(RangeAnalysisFlow* in, Instruction* inst);
	RangeAnalysisFlow *executePhiInst(RangeAnalysisFlow* in, Instruction* inst);

	map<int,nodeState> nodeCount;	//Any node seen >2 times and some flags is considered a loop
public:
	RangeDomainElement computeOpRange(RangeDomainElement leftRange, RangeDomainElement rightRange, Instruction *inst);


};

//Helper function to set variables with a different range but the same name to top
//This is used during merge control flow as a way of dealing with infinite range analysis loops
void DeleteDifferentRanges(RangeAnalysisFlow* A, RangeAnalysisFlow* B);

//Used to resolve an operand value wherever possible to a float. Will give up in some cases
//Probably
RangeDomainElement getOperandValue(Value* Operand);


#endif
