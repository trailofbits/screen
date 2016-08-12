#include "RangeAnalysis.h"

#define ADD 8 //This is the opcode for the add instruction
#define FADD 9 //This is the opcode for the add instruction
#define SUB 10 //This is the opcode for the sub instruction
#define FSUB 11 //This is the opcode for the floating point sub instruction
#define MUL 12 //This is the opcode for the mul instruction
#define FMUL 13 //This is the opcode for the floating point mul instruction
#define SDIV 15 //This is the opcode for the signed div instruction
#define FDIV 16 //This is the opcode for the float div instruction
#define UREM 17 //This is the opcode for the unsigned mod instruction
#define SREM 18 //This is the opcode for the signed mod instruction
#define FREM 19 //This is the opcode for the floating point mod instruction
#define SHL 20 //This is the opcode for the Shift left (logical) instruction
#define LSHR 21 //This is the opcode for the Shift right (logical) instruction
#define ASHR 25 //This is the opcode for the Shift right (arithmetic) instruction
// Logical operators (integer operands)
/*122	HANDLE_BINARY_INST(20, Shl  , BinaryOperator) // Shift left  (logical)
 123	HANDLE_BINARY_INST(21, LShr , BinaryOperator) // Shift right (logical)
 124	HANDLE_BINARY_INST(22, AShr , BinaryOperator) // Shift right (arithmetic)
 125	HANDLE_BINARY_INST(23, And  , BinaryOperator)
 126	HANDLE_BINARY_INST(24, Or   , BinaryOperator)
 127	HANDLE_BINARY_INST(25, Xor  , BinaryOperator)
 128	  LAST_BINARY_INST(25)
 */

#define TRUNC 33 // Truncate integers
#define ZEXT 34 // Zero extend integers
#define SEXT 35 // Sign extend integers
#define FPTOUI 36 //This is the opcode for the int to float cast instruction
#define FPTOSI 37 //This is the opcode for the float to integer cast instruction
#define UITOFP 38 //UInt -> floating point
#define SITOFP 39 // SInt -> floating point
#define FPTRUNC 40 //Truncate floating point
#define FPEXT 41 // Extend floating point
#define PHI 53 // Extend floating point



Flow* RangeAnalysis::executeFlowFunction(Flow *in, Instruction *inst,
		int NodeId) {

	RangeAnalysisFlow* inFlow = static_cast<RangeAnalysisFlow*>(in);
	RangeAnalysisFlow * output;
	int loopCount;
	outs()<< inst->getOpcode()<<"\n";
	//Just need to check if we have seen this basic block a few times.
	//If we have seen this a few times, change ALL VARIABLES that are changing to TOP
	if (nodeCount.find(NodeId) != nodeCount.end())
	{
		loopCount = nodeCount[NodeId].nodeVisitCounter;
		if (loopCount >= 1000)
		{
			//ANY VARIABLES THAT DID NOT CHANGE MUST BE SET TO TOP!!!
			//THIS SHOULD SET SOME VARIABLES TO TOP!!!!
			RangeAnalysisFlow* f = new RangeAnalysisFlow(inFlow);
			RangeAnalysisFlow* ff = new RangeAnalysisFlow();
			RangeAnalysisFlow* tmp =
					static_cast<RangeAnalysisFlow*>(ff->join(f));//Contains the value of the variable from the old set
			delete ff;
			delete f;
			f = tmp;	//Keep tmp which has the variable from.. f?

			DeleteDifferentRanges(f, nodeCount[NodeId].nodeSet);

			return (f);	//In = Out
			//Just return the in Set (no Change)
		}
		nodeCount[NodeId].nodeVisitCounter = (nodeCount[NodeId].nodeVisitCounter
				+ 1);
	} else {
		nodeState thisNodeState;
		RangeAnalysisFlow* f = new RangeAnalysisFlow(inFlow);
		RangeAnalysisFlow* ff = new RangeAnalysisFlow();
		RangeAnalysisFlow* tmp = static_cast<RangeAnalysisFlow*>(ff->join(f));//Contains the value of the variable from the old set
		delete ff;
		delete f;
		f = tmp;

		thisNodeState.nodeVisitCounter = 1;
		thisNodeState.nodeSet = f;
		nodeCount[NodeId] = thisNodeState;	//Track this node

	}

	switch (inst->getOpcode()) {
	case ADD:
	case SUB:
	case MUL:
	case SDIV:
	case SREM:
	case SHL:
	case LSHR:
	case ASHR:
		//output = executeAddInst(inFlow, inst);
		output = executeOpInst(inFlow, inst, inst->getOpcode());
		break;
	case FADD:
	case FSUB:
	case FMUL:
	case FDIV:
	case FREM:
		//output = executeFDivInst(inFlow, inst);
		output = executeFOpInst(inFlow, inst, inst->getOpcode());
		break;

	case TRUNC:
	case ZEXT:
	case SEXT:
	case FPTOSI:
	case FPTOUI:
	case UITOFP:
	case SITOFP:
	case FPTRUNC:
	case FPEXT:
		output = executeCastInst(inFlow, inst);
		break;
	case PHI:
		output = executePhiInst(inFlow, inst);
		break;

	default:
		output = new RangeAnalysisFlow(inFlow);
		break;
	}
#ifdef DEBUGRANGE
	outs() << "Instruction : " << *inst << ", Flow value : " << output->jsonString() << "\n";
#endif
	return output;
}

RangeAnalysisFlow* RangeAnalysis::executeCastInst(RangeAnalysisFlow* in,
		Instruction* instruction) {

	//outs()<<"HERE Cast\n";
	RangeAnalysisFlow* f = new RangeAnalysisFlow(in);
	map<string, RangeDomainElement> value;
	Value *retVal = instruction;
	string regName = retVal->getName();

	Value* casting = instruction->getOperand(0); //RO

	if (!dyn_cast<Constant>(retVal))	//If the instruction is not a constant
			{

		if (!dyn_cast<Constant>(casting))	//If operand 0 is not a constant
				{
			// Instruction and operand 0 are both variables. We just need to forward the value
			if (f->value.find(casting->getName()) == f->value.end()) {
				// Oh no! Read the error message!
#ifdef RANGEDEBUG
				outs() << "Undefined variable!\n";
				outs() << "Variable: " << casting->getName()
						<< " was not found \n";
#endif
			} else {
				// Hmm, I guess we're good...
				//SHOULD BE AN INTEGER TYPE....
				//Get the Range that we have for this variable
				RangeDomainElement forwardRange = f->value.find(
						casting->getName())->second;
				RangeAnalysisFlow* ff = new RangeAnalysisFlow();
				value[retVal->getName()] = forwardRange;//Move the value of the stored variable into...
				ff->value = value;							//A new set
				RangeAnalysisFlow* tmp =
						static_cast<RangeAnalysisFlow*>(ff->join(f));//Contains the value of the variable from the old set
				delete ff;
				delete f;
				f = tmp;			//Keep tmp which has the variable from.. f?
			}

		} else 					//Operand 0 is not a constant. Is it a float?
		{

			// Hmm, I guess we're good...
			if (ConstantFP *cfp = dyn_cast<ConstantFP>(casting)) {
				RangeDomainElement forwardRange;//A precise range of 0 can be created using the constant.
				forwardRange = getOperandValue(cfp);

				RangeAnalysisFlow* ff = new RangeAnalysisFlow();

				value[retVal->getName()] = forwardRange;
				ff->value = value;
				RangeAnalysisFlow* tmp =
						static_cast<RangeAnalysisFlow*>(ff->join(f));
				delete ff;
				delete f;
				f = tmp;
			} else if (ConstantInt *cfp = dyn_cast<ConstantInt>(casting)) {
				RangeDomainElement forwardRange;
				RangeAnalysisFlow* ff = new RangeAnalysisFlow();


				forwardRange = getOperandValue(cfp);
				value[retVal->getName()] = forwardRange;
				ff->value = value;
				RangeAnalysisFlow* tmp =
						static_cast<RangeAnalysisFlow*>(ff->join(f));
				delete ff;
				delete f;
				f = tmp;

			}

		}

	}
	return f;

}


//For the tricky case of range arithmetic ops
RangeDomainElement RangeAnalysis::computeOpRange(RangeDomainElement leftRange,
		RangeDomainElement rightRange, unsigned opcode) {
	//outs()<<"HERE compute range\n";
	RangeDomainElement resRange;
	float mulDivCombos[4];
	int shiftCombos[4];
	int AshiftLower, AshiftHigher, BshiftLower, BshiftHigher;
	int lowerShiftSign, upperShiftSign;
	int comboCheckCtr = 0;
//CHECK THAT BOTH ELEMENTS ARE IN RANGE!!!
	if (rightRange.undefined || leftRange.undefined || rightRange.top
			|| leftRange.top) {
		resRange.top = true;//Oh dearrrrrrrr :( This guy is not gonna be helpful anymore :(
		resRange.bottom = false;
		resRange.upper = 0;
		resRange.lower = 0;
		return resRange;
	}

	switch (opcode) {

	case ADD:
	case FADD:
		//Get the lowest of the low and the highest of the high resVal = leftVal + rightVal;
		resRange.lower = leftRange.lower + rightRange.lower;
		resRange.upper = leftRange.upper + rightRange.upper;
		resRange.bottom = false;
		break;
	case SUB:
	case FSUB:
		//Get the highest of the low and the lowest of the high. Hu-hya! resVal = leftVal - rightVal;
		resRange.lower = leftRange.lower - rightRange.lower;
		resRange.upper = leftRange.upper - rightRange.upper;
		resRange.bottom = false;
		break;
	case FDIV:
	case SDIV:
		//Combo fiend! resVal = leftVal / rightVal;
		mulDivCombos[0] = leftRange.lower / rightRange.lower;
		mulDivCombos[1] = leftRange.lower / rightRange.upper;
		mulDivCombos[2] = leftRange.upper / rightRange.lower;
		mulDivCombos[3] = leftRange.upper / rightRange.upper;
		//get the lowest of all combos for the return lower bound
		//get the highest of all combos for the return upper bound
		resRange.lower = mulDivCombos[0];//Initialize, you must. Since we start with max in resRange.
		resRange.upper = mulDivCombos[0];
		resRange.bottom = false;
		while (comboCheckCtr < 4) {
			if (mulDivCombos[comboCheckCtr] < resRange.lower)
				resRange.lower = mulDivCombos[comboCheckCtr];
			if (mulDivCombos[comboCheckCtr] > resRange.upper)
				resRange.upper = mulDivCombos[comboCheckCtr];
			comboCheckCtr++;
		}
		break;
	case FMUL:
	case MUL:
		//Combo fiend! resVal = leftVal / rightVal;
		mulDivCombos[0] = leftRange.lower * rightRange.lower;
		mulDivCombos[1] = leftRange.lower * rightRange.upper;
		mulDivCombos[2] = leftRange.upper * rightRange.lower;
		mulDivCombos[3] = leftRange.upper * rightRange.upper;
		//get the lowest of all combos for the return lower bound
		//get the highest of all combos for the return upper bound
		resRange.lower = mulDivCombos[0];//Initialize, you must. Since we start with max in resRange.
		resRange.upper = mulDivCombos[0];
		resRange.bottom = false;
		while (comboCheckCtr < 4) {
			if (mulDivCombos[comboCheckCtr] < resRange.lower)
				resRange.lower = mulDivCombos[comboCheckCtr];
			if (mulDivCombos[comboCheckCtr] > resRange.upper)
				resRange.upper = mulDivCombos[comboCheckCtr];
			comboCheckCtr++;
		}
		break;
	case FREM:
	case SREM:
		//Combo fiend! resVal = leftVal / rightVal;
		mulDivCombos[0] = (int) leftRange.lower % (int) rightRange.lower;
		mulDivCombos[1] = (int) leftRange.lower % (int) rightRange.upper;
		mulDivCombos[2] = (int) leftRange.upper % (int) rightRange.lower;
		mulDivCombos[3] = (int) leftRange.upper % (int) rightRange.upper;
		//get the lowest of all combos for the return lower bound
		//get the highest of all combos for the return upper bound
		resRange.lower = mulDivCombos[0];//Initialize, you must. Since we start with max in resRange.
		resRange.upper = mulDivCombos[0];
		resRange.bottom = false;
		while (comboCheckCtr < 4) {
			if (mulDivCombos[comboCheckCtr] < resRange.lower)
				resRange.lower = mulDivCombos[comboCheckCtr];
			if (mulDivCombos[comboCheckCtr] > resRange.upper)
				resRange.upper = mulDivCombos[comboCheckCtr];
			comboCheckCtr++;
		}
		break;
	case SHL:
		//Combo fiend!
		mulDivCombos[0] = (int) leftRange.lower << (int) rightRange.lower;
		mulDivCombos[1] = (int) leftRange.lower << (int) rightRange.upper;
		mulDivCombos[2] = (int) leftRange.upper << (int) rightRange.lower;
		mulDivCombos[3] = (int) leftRange.upper << (int) rightRange.upper;
		//get the lowest of all combos for the return lower bound
		//get the highest of all combos for the return upper bound
		resRange.lower = mulDivCombos[0];//Initialize, you must. Since we start with max in resRange.
		resRange.upper = mulDivCombos[0];
		resRange.bottom = false;
		while (comboCheckCtr < 4) {
			if (mulDivCombos[comboCheckCtr] < resRange.lower)
				resRange.lower = mulDivCombos[comboCheckCtr];
			if (mulDivCombos[comboCheckCtr] > resRange.upper)
				resRange.upper = mulDivCombos[comboCheckCtr];
			comboCheckCtr++;
		}
		break;
	case LSHR:
		AshiftLower = (int) leftRange.lower;
		AshiftHigher = (int) leftRange.upper;
		BshiftLower = (int) rightRange.lower;
		BshiftHigher = (int) rightRange.upper;

		shiftCombos[0] = AshiftLower >> BshiftLower;
		shiftCombos[1] = AshiftLower >> BshiftHigher;
		shiftCombos[2] = AshiftHigher >> BshiftLower;
		shiftCombos[3] = AshiftHigher >> BshiftHigher;
		//get the lowest of all combos for the return lower bound
		//get the highest of all combos for the return upper bound
		resRange.lower = shiftCombos[0];//Initialize, you must. Since we start with max in resRange.
		resRange.upper = shiftCombos[0];
		resRange.bottom = false;
		while (comboCheckCtr < 4) {
			if (shiftCombos[comboCheckCtr] < resRange.lower)
				resRange.lower = (float) shiftCombos[comboCheckCtr];
			if (shiftCombos[comboCheckCtr] > resRange.upper)
				resRange.upper = (float) shiftCombos[comboCheckCtr];
			comboCheckCtr++;
		}
		break;
	case ASHR:	//Haaaanh? TO DO: Debug.
//		resVal = (int) leftVal >> (int) rightVal;
	//another combo fiender.
	//BUBAGAWG!!!!
		AshiftLower = (int) leftRange.lower;
		AshiftHigher = (int) leftRange.upper;
		BshiftLower = (int) rightRange.lower;
		BshiftHigher = (int) rightRange.upper;
		lowerShiftSign = AshiftLower & 0x80000000;//Save the sign bit of the left operand (low range)
		upperShiftSign = AshiftHigher & 0x80000000; //Save the sign bit of the left operand (high range)
		AshiftLower &= 0x7fffffff;//In a copy to be shifted: Destroy the sign bit of the left operand (low range)
		AshiftLower &= 0x7fffffff;

		AshiftLower >>= BshiftLower;
		AshiftLower |= lowerShiftSign;
		shiftCombos[0] = AshiftLower;

		AshiftLower >>= BshiftHigher;
		AshiftLower |= lowerShiftSign;
		shiftCombos[1] = AshiftLower;

		AshiftHigher >>= BshiftLower;
		AshiftHigher |= upperShiftSign;
		shiftCombos[2] = AshiftHigher;

		AshiftHigher >>= BshiftHigher;
		AshiftHigher |= BshiftHigher;
		shiftCombos[3] = AshiftHigher;
		//get the lowest of all combos for the return lower bound
		//get the highest of all combos for the return upper bound
		resRange.lower = shiftCombos[0];//Initialize, you must. Since we start with max in resRange.
		resRange.upper = shiftCombos[0];
		resRange.bottom = false;
		while (comboCheckCtr < 4) {
			if (shiftCombos[comboCheckCtr] < resRange.lower)
				resRange.lower = (float) shiftCombos[comboCheckCtr];
			if (shiftCombos[comboCheckCtr] > resRange.upper)
				resRange.upper = (float) shiftCombos[comboCheckCtr];
			comboCheckCtr++;
		}
		break;
	}

	outs()<<"VARIABLE RANGE "<< resRange.lower <<" "<<resRange.upper<<"\n";
	return resRange;
}

RangeAnalysisFlow* RangeAnalysis::executePhiInst(RangeAnalysisFlow* in,
		Instruction* instruction) {

	//outs()<<"HERE Pjhi\n";
	RangeAnalysisFlow* f = new RangeAnalysisFlow(in);

	RangeDomainElement leftVal;
	RangeDomainElement rightVal;
	RangeDomainElement maxRange;
	Value *leftOperand = instruction->getOperand(0);
	Value *rightOperand = instruction->getOperand(1);
	map<string, RangeDomainElement> value;
	Value *K = instruction;
	string regName = K->getName();
#ifdef RANGEDEBUG
	outs() << "Instruction : " << regName << " left " << leftOperand->getName()
			<< " right " << rightOperand->getName() << "\n";
#endif

//GET THE MAXIMUM RANGE FROM THE PHI NODE THAT YOU ARE ABLE
	// Ok, cool! Both the right and the left operand is a variable...

		//Get the leftVal from the phi node if possible

		if((f->value.find(leftOperand->getName()) == f->value.end()))
			{leftVal = getOperandValue(leftOperand);}
		else
			{leftVal = f->value.find(leftOperand->getName())->second;}

		if((f->value.find(rightOperand->getName()) == f->value.end()))
			{rightVal = getOperandValue(rightOperand);}
		else
			{rightVal = f->value.find(rightOperand->getName())->second;}
#ifdef RANGEDEBUG
		outs() << "leftVal: " << leftVal.upper << " , " << leftVal.lower
				<< "rightVal:" << rightVal.upper << " , " << rightVal.lower
				<< "\n";
#endif
		//GETTING THE MAX RANGE!!!
		maxRange = JoinRangeDomainElements(
				(const RangeDomainElement*) &leftVal,
				(const RangeDomainElement*) &rightVal);

		RangeAnalysisFlow* ff = new RangeAnalysisFlow();
#ifdef RANGEDEBUG
		outs() << "input" << leftVal.upper << " , " << leftVal.lower
				<< "rightVal:" << rightVal.upper << " , " << rightVal.lower
				<< "\n";
		outs() << "outcome: " << maxRange.upper << " , " << maxRange.lower
				<< "\n";
#endif
		value[regName] = maxRange;


//IF regName is NOT IN f (in) ADD regName TO f
		if((f->value.find(regName) == f->value.end()))
		{
			//f->value[regName] = maxRange;
			//Random mad shit
			ff->value = value;
			RangeAnalysisFlow* tmp = static_cast<RangeAnalysisFlow*>(ff->join(f));
			delete ff;
			delete f;
			f = tmp;
		}
		else
		{
			leftVal = maxRange;
			rightVal = f->value[regName];
			maxRange = JoinRangeDomainElements(
					(const RangeDomainElement*) &leftVal,
					(const RangeDomainElement*) &rightVal);
			value[regName] = maxRange;
			//Random mad shit
			ff->value = value;
			RangeAnalysisFlow* tmp = static_cast<RangeAnalysisFlow*>(ff->join(f));
			delete ff;
			delete f;
			f = tmp;
		}

	return f;
}

RangeAnalysisFlow* RangeAnalysis::executeFOpInst(RangeAnalysisFlow* in,
		Instruction* instruction, unsigned opcode) {

	//outs()<<"HERE FLOATING\n";
	RangeAnalysisFlow* f = new RangeAnalysisFlow(in);
	RangeDomainElement leftRange, rightRange;
	Value *leftOperand = instruction->getOperand(0);
	Value *rightOperand = instruction->getOperand(1);
	map<string, RangeDomainElement> value;
	Value *K = instruction;
	string regName = K->getName();
	rightOperand->dump();
	leftOperand->dump();

// Checking if left operand is a constant
	if (ConstantFP *CILeft = dyn_cast<ConstantFP>(leftOperand)) {

		if (ConstantFP *CIRight = dyn_cast<ConstantFP>(rightOperand)) {
			RangeDomainElement resRange;
			// Cool they are both constants. you can get a precise range.

			float leftVal = CILeft->getValueAPF().convertToFloat();
			float rightVal = CIRight->getValueAPF().convertToFloat();

			//float resVal = computeOp(leftVal, rightVal, opcode);
			leftRange.upper = leftVal;
			leftRange.lower = rightVal;
			leftRange.bottom = false;

			rightRange.upper = rightVal;
			rightRange.lower = rightVal;
			rightRange.bottom = false;

			resRange = computeOpRange(leftRange, rightRange, opcode);

			RangeAnalysisFlow* ff = new RangeAnalysisFlow();
			value[K->getName()] = resRange;
			ff->value = value;
			RangeAnalysisFlow* tmp =
					static_cast<RangeAnalysisFlow*>(ff->join(f));
			delete ff;
			delete f;
			f = tmp;
		} else {
			// ok so the right operand is a variable
			if (f->value.find(rightOperand->getName()) == f->value.end()) {
#ifdef RANGEDEBUG
				// Oh no! Read the error message!
				outs() << "Oh no! Something went wrong!\n";
				outs() << "Undefined variable!\n";
				outs() << "Apparently the right operand of the op is";
				outs() << " a variable but this is the first time we ";
				outs() << "come across this variable!!\n";
#endif
			}

			else {
				//Can still get a precise range
				RangeDomainElement leftRange, rightRange, resRange;

				leftRange = getOperandValue(CILeft);
				rightRange = f->value.find(rightOperand->getName())->second;

				resRange = computeOpRange(leftRange, rightRange, opcode);

				RangeAnalysisFlow* ff = new RangeAnalysisFlow();
				value[K->getName()] = resRange;
				ff->value = value;
				RangeAnalysisFlow* tmp =
						static_cast<RangeAnalysisFlow*>(ff->join(f));
				delete ff;
				delete f;
				f = tmp;
			}
		}
	} else {
		// So, the left part of the addition is a variable. We'll have to check the input set to get the value
		// this variable has at the moment.
		if (ConstantFP *CIRight = dyn_cast<ConstantFP>(rightOperand)) {
			// Ok, cool! the right part is a constant...
			//leftOperand->getName()
			outs()<<"Right is a constant\n";
			if (f->value.find(leftOperand->getName()) == f->value.end()) {
#ifdef DEBUGRANGE
				// Oh no! Read the error message!
				outs() << "Oh no! Something went terribly wrong!\n";
				outs() << "Undefined variable!\n";
				outs() << "Apparently the left operand of the op is";
				outs() << " a variable but this is the first time we ";
				outs() << "come across this variable!!\n";
#endif
			} else {
				// Hmm, I guess we're good...
				//HERE WE ARE COMPUTING OPS USING A RANGE OF VALUES, NOT THE PLAIN ABSOLUTES
				//Now we are working with a range in the left hand. This will introuduce some impreciseness
				RangeDomainElement resRange, rightRange, leftRange =
						f->value.find(leftOperand->getName())->second;

				rightRange = getOperandValue(CIRight);
				resRange = computeOpRange(leftRange, rightRange, opcode);

				RangeAnalysisFlow* ff = new RangeAnalysisFlow();

				value[K->getName()] = resRange;
				ff->value = value;
				RangeAnalysisFlow* tmp =
						static_cast<RangeAnalysisFlow*>(ff->join(f));
				delete ff;
				delete f;
				f = tmp;
			}
		} else {

			// Ok, cool! Both the right and the left operand is a variable...
			if ((f->value.find(leftOperand->getName()) == f->value.end())
					| (f->value.find(rightOperand->getName()) == f->value.end())) {
#ifdef RANGEDEBUG
				// Oh no! Read the error message!
				outs() << "Oh no! Something went terribly wrong!\n";
#endif
			} else {

				RangeDomainElement resRange, rightRange, leftRange =
						f->value.find(leftOperand->getName())->second;

				rightRange = f->value.find(rightOperand->getName())->second;

				resRange = computeOpRange(leftRange, rightRange, opcode);

				RangeAnalysisFlow* ff = new RangeAnalysisFlow();

				value[K->getName()] = resRange;
				ff->value = value;
				RangeAnalysisFlow* tmp =
						static_cast<RangeAnalysisFlow*>(ff->join(f));
				delete ff;
				delete f;
				f = tmp;

			}

		}

	}
	return f;
}

RangeAnalysisFlow* RangeAnalysis::executeOpInst(RangeAnalysisFlow* in,
		Instruction* instruction, unsigned opcode) {
	//outs()<<"HERE execute ASHR\n";
	RangeDomainElement leftRange, rightRange, resRange;
	RangeAnalysisFlow* f = new RangeAnalysisFlow(in);
	Value *leftOperand = instruction->getOperand(0);
	Value *rightOperand = instruction->getOperand(1);
	map<string, RangeDomainElement> value;
	Value *K = instruction;
	string regName = K->getName();
	rightOperand->dump();
	leftOperand->dump();

// Checking if left operand is a constant
	if (ConstantInt *CILeft = dyn_cast<ConstantInt>(leftOperand)) {

		if (ConstantInt *CIRight = dyn_cast<ConstantInt>(rightOperand)) {
			// Cool they are both constants.

			leftRange = getOperandValue(CILeft);
			rightRange = getOperandValue(CIRight);

			resRange = computeOpRange(leftRange, rightRange, opcode);//Get precise information
			outs()<<resRange.upper<<" "<<resRange.lower<<"\n";
			//float resVal = leftVal + rightVal;
			RangeAnalysisFlow* ff = new RangeAnalysisFlow();

			value[K->getName()] = resRange;
			ff->value = value;
			RangeAnalysisFlow* tmp =
					static_cast<RangeAnalysisFlow*>(ff->join(f));
			delete ff;
			delete f;
			f = tmp;
		} else {
			// ok so the right operand is a variable
			rightOperand->dump();
			CILeft->dump();
			if (f->value.find(rightOperand->getName()) == f->value.end()) {
#ifdef DEBUGRANGE
				// Oh no! Read the error message!
				outs() << "Oh no! Something went wrong!\n";
				outs() << "Undefined variable!\n";
				outs() << "Apparently the right operand of the op is";
				outs() << " a variable but this is the first time we ";
				outs() << "come across this variable!!\n";
#endif
			}

			else {
				// Hmm, I guess we're good...

				leftRange = getOperandValue(CILeft);
				rightRange = f->value.find(rightOperand->getName())->second;
				resRange = computeOpRange(leftRange, rightRange, opcode);

				RangeAnalysisFlow* ff = new RangeAnalysisFlow();

				value[K->getName()] = resRange;
				ff->value = value;
				RangeAnalysisFlow* tmp =
						static_cast<RangeAnalysisFlow*>(ff->join(f));
				delete ff;
				delete f;
				f = tmp;
			}
		}
	} else {
		// So, the left part of the addition is a variable. We'll have to check the input set to get the value
		// this variable has at the moment.
		if (ConstantInt *CIRight = dyn_cast<ConstantInt>(rightOperand)) {
			// Ok, cool! the right part is a constant...
		//	outs()<<"RIGHT SIDE IS A CONSTANT\n";
			if (f->value.find(leftOperand->getName()) == f->value.end()) {
				// Oh no! Read the error message!
				outs()<< f->value.size() << "\n";
				outs() << "Oh no! Something went terribly wrong!\n";

			} else {
				// Hmm, I guess we're good...
				leftRange = f->value.find(leftOperand->getName())->second;

				rightRange = getOperandValue(CIRight);
				resRange = computeOpRange(leftRange, rightRange, opcode);

				RangeAnalysisFlow* ff = new RangeAnalysisFlow();

				value[K->getName()] = resRange;
				ff->value = value;
				RangeAnalysisFlow* tmp =
						static_cast<RangeAnalysisFlow*>(ff->join(f));
				delete ff;
				delete f;
				f = tmp;
			}
		} else {

			// Ok, cool! Both the right and the left operand is a variable...
			if ((f->value.find(leftOperand->getName()) == f->value.end())
					| (f->value.find(rightOperand->getName()) == f->value.end())) {
#ifdef RANGEDEBUG
				// Oh no! Read the error message!
				outs() << "Oh no! Something went terribly wrong!\n";
#endif
			} else {
				// Hmm, I guess we're good...
				leftRange = f->value.find(leftOperand->getName())->second;

				rightRange = f->value.find(rightOperand->getName())->second;
				resRange = computeOpRange(leftRange, rightRange, opcode);

				RangeAnalysisFlow* ff = new RangeAnalysisFlow();

				value[K->getName()] = resRange;
				ff->value = value;
				RangeAnalysisFlow* tmp =
						static_cast<RangeAnalysisFlow*>(ff->join(f));
				delete ff;
				delete f;
				f = tmp;

			}
			//break;

		}

	}
	//outs()<<"HERE \n";
	return f;
}

Flow * RangeAnalysis::initialize() {
	//outs()<<"HERE initialize\n";
	return new RangeAnalysisFlow(RangeAnalysisFlow::BOTTOM);
}

RangeAnalysis::RangeAnalysis(Function & F) :
		StaticAnalysis() {
	this->top = new RangeAnalysisFlow(RangeAnalysisFlow::TOP);//Should be changed by subclasses of Flow to an instance of the subclass
	this->bottom = new RangeAnalysisFlow(RangeAnalysisFlow::BOTTOM);//Should be changed by subclasses of Flow to an instance of the subclass
	this->functionName = F.getName();
	buildCFG(F);
}

//Utility function
//Delete (set to top) all variables with different ranges. This is a utility function for merging, specifically for looping
//control structures in the range analysis
void DeleteDifferentRanges(RangeAnalysisFlow* A, RangeAnalysisFlow* B) {
	//outs()<<"HERE 3\n";
	for (map<string, RangeDomainElement>::iterator it = B->value.begin();
			it != B->value.end(); it++) {

		if (!(A->value.find(it->first) == A->value.end())) {
			// Oh no! They do have the same key! We need to check if they have
			// the same values! if they do then we're good
			RangeDomainElement thisVal = A->value.find(it->first)->second;
			RangeDomainElement BVal = B->value.find(it->first)->second;

			//if (BVal == thisVal)
			if (!RangeDomainElementisEqual((const RangeDomainElement*) &BVal,
					(const RangeDomainElement*) &thisVal)) {
				// Both branches had different value for this variable
				//f->value[it->first] = BVal;
				thisVal.top = true;
				thisVal.bottom = false;
				thisVal.lower = 0;
				thisVal.undefined = true;
				thisVal.upper = 0;
				B->value[it->first] = thisVal;
				A->value[it->first] = thisVal;

			}
		}
	}
}

RangeDomainElement getOperandValue(Value* Operand)
{
	RangeDomainElement OpValue; //init to max range automatically
	float FloatVal;
	int IntVal;

	ConstantFP *ConstFpOp = dyn_cast<ConstantFP>(Operand);
	ConstantInt *ConstIntOp = dyn_cast<ConstantInt>(Operand);

	if(ConstFpOp)
	{
		OpValue.bottom = false;
		FloatVal = ConstFpOp->getValueAPF().convertToFloat();
		OpValue.upper = FloatVal;
		OpValue.lower = FloatVal;
	}
	if(ConstIntOp)
	{
		OpValue.bottom = false;
		if(ConstIntOp->isNegative())
		{
			if(ConstIntOp->getBitWidth() <= 32)
				{
					IntVal = ConstIntOp->getSExtValue();
					OpValue.upper = (float)IntVal;
					OpValue.lower = OpValue.upper;
				}
			else//TOO BIG FOR US! HACK!
				{
					OpValue.upper = -std::numeric_limits<float>::infinity();
					OpValue.lower = OpValue.upper;
				}
		}
		else
		{
			OpValue.bottom = false;
			FloatVal = ConstIntOp->getZExtValue();
			OpValue.upper = FloatVal;
			OpValue.lower = FloatVal;
		}
	}

	//outs()<<"HERE 4\n";
return OpValue;
}

//End Utility function
