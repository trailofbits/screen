#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "instrOverflow.h"
#include "Live.h"

using namespace llvm;

void instrOverflow::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<Live>();
}

bool instrOverflow::runOnFunction(Function &F) {
	// first pass replaces the binary instructions
	pass1 = true;
	while (updateFunction(F)) {}
	// second pass inserts the comparisons
	pass1 = false;
	while (updateFunction(F)) {}
	Live * LV = &(getAnalysis<Live>());
	LV->releaseMemory();
	return false;
}

bool instrOverflow::updateFunction(Function &F) {
	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		BasicBlock * b = &*i;

		for (BasicBlock::iterator it = b->begin(), et = b->end(); it != et; ++it) {
			if (visit(*it)) return true;
		}
	}
	return false;
}

bool instrOverflow::visitExtractValueInst(ExtractValueInst &inst) {
	Value * val = inst.getAggregateOperand();
	if (CallInst * call = dyn_cast<CallInst>(val)) {
		Function * f = call->getCalledFunction();	
		if (f != NULL && f->isIntrinsic()) {
			unsigned intrinsicID = f->getIntrinsicID();
			unsigned indice;
			switch (intrinsicID) {
				case llvm::Intrinsic::sadd_with_overflow:
				case llvm::Intrinsic::smul_with_overflow:
				case llvm::Intrinsic::ssub_with_overflow:
				case llvm::Intrinsic::uadd_with_overflow:
				case llvm::Intrinsic::umul_with_overflow:
				case llvm::Intrinsic::usub_with_overflow:
					assert(inst.getNumIndices() == 1);
					indice = inst.getIndices()[0];
					if (indice == 0) {
						// this is the instruction
						if (pass1) {
							std::vector<Value*> args;
							args.push_back(call->getArgOperand(0));
							args.push_back(call->getArgOperand(1));
							replaceWithUsualOp(&inst,intrinsicID,&args,call);
							return true;
						}
					} else {
						// this is the ofl flag
						if (!pass1) {
							replaceWithCmp(&inst,intrinsicID,call);
							return true;
						}
					}
					break;
				default:
					break;
			}
		}
	}
	return false;
}

bool instrOverflow::visitBranchInst(BranchInst &inst) {
	if (inst.isUnconditional()) return false;
	Value * cond = inst.getCondition();
	if (BinaryOperator * binop = dyn_cast<BinaryOperator>(cond)) {
		if (BinaryOperator::isNot(binop)) {
			Value * notarg = BinaryOperator::getNotArgument(binop);
			inst.setCondition(notarg);
			inst.swapSuccessors();
			// we should delete the useless instruction
			if (binop->use_empty())
				binop->eraseFromParent();
		}
	}
	return false;
}

bool instrOverflow::visitCallInst(CallInst &inst) {
		Function * f = inst.getCalledFunction();	
		if (f == NULL) return false;
		if (f->getName() == "__ubsan_handle_add_overflow" ||
			f->getName() == "__ubsan_handle_sub_overflow" ||
			f->getName() == "__ubsan_handle_mul_overflow" ||
			f->getName() == "__ubsan_handle_divrem_overflow" 
				) {
			LLVMContext &C = inst.getContext();
			FunctionType * ftype = FunctionType::get(Type::getVoidTy(inst.getContext()),true);
			Module * M = inst.getParent()->getParent()->getParent();
			Constant * assert_fail_func = M->getOrInsertFunction("__assert_fail_overflow",ftype);
			inst.setCalledFunction(assert_fail_func);
			inst.setDoesNotReturn();
			
			Constant * s = ConstantDataArray::getString(C,"overflow");
			for (unsigned i = 0; i < inst.getNumArgOperands(); i++) {
				inst.setArgOperand(i,s);
			}

			//CallInst * newcall = CallInst::Create(assert_fail_func);
			//ReplaceInstWithInst(&inst,newcall);
			
			// the following is turned off, because it is source of constraints
			// with huge coefficient in the polyhedra abstract domain
#if 0
			// get the terminatorinst and replace it by unreachable
			TerminatorInst * term = inst.getParent()->getTerminator();
			TerminatorInst * unreachable = new UnreachableInst(C);
			ReplaceInstWithInst(term,unreachable);
#endif
		}
		return false;
}

void instrOverflow::replaceWithUsualOp(
		Instruction * old, 
		unsigned intrinsicID,
		std::vector<Value*> * args,
		CallInst * intrinsicCall
		){

	Instruction * new_inst = NULL;
	Value* op1 = (*args)[0];
	Value* op2 = (*args)[1];
	switch (intrinsicID) {
		case llvm::Intrinsic::sadd_with_overflow:
		case llvm::Intrinsic::uadd_with_overflow:
			{
			new_inst = BinaryOperator::Create(llvm::Instruction::Add,op1,op2);
			}
			break;
		case llvm::Intrinsic::smul_with_overflow:
		case llvm::Intrinsic::umul_with_overflow:
			{
			new_inst = BinaryOperator::Create(llvm::Instruction::Mul,op1,op2);
			}
			break;
		case llvm::Intrinsic::ssub_with_overflow:
		case llvm::Intrinsic::usub_with_overflow:
			{
			new_inst = BinaryOperator::Create(llvm::Instruction::Sub,op1,op2);
			}
			break;
		default:
			assert(false && "not implemented");
			break;
	}
	// the old and the new instruction share the same dbg info
	if (old->hasMetadata()) {
		MDNode * dbg = old->getMetadata(LLVMContext::MD_dbg);
		new_inst->setMetadata(LLVMContext::MD_dbg,dbg);
	}
	ReplaceInstWithInst(old, new_inst);
	replaced.insert(std::pair<CallInst*,Instruction*>(intrinsicCall,new_inst));
}

void instrOverflow::replaceWithCmp(
		Instruction * old, 
		unsigned intrinsicID,
		CallInst * intrinsicCall
		){
	// the old and the new instructions all share the same dbg info
	MDNode * dbg = NULL;
		
	if (old->hasMetadata()) {
		dbg = old->getMetadata(LLVMContext::MD_dbg);
	}

	Instruction * new_inst_max = NULL;
	Instruction * new_inst_min = NULL;
	Instruction * new_inst = NULL;
	Instruction * re = replaced[intrinsicCall];
	assert(re != NULL);
	unsigned bitwidth = re->getType()->getIntegerBitWidth();
	LLVMContext& C = re->getContext();
	Constant * maxsignedval = ConstantInt::get(re->getType(),APInt::getSignedMaxValue(bitwidth));
	Constant * maxunsignedval = ConstantInt::get(re->getType(),APInt::getMaxValue(bitwidth));
	Constant * minsignedval = ConstantInt::get(re->getType(),APInt::getSignedMinValue(bitwidth));
	Constant * minunsignedval = ConstantInt::get(re->getType(),APInt::getMinValue(bitwidth));
	switch (intrinsicID) {
		case llvm::Intrinsic::sadd_with_overflow:
		case llvm::Intrinsic::smul_with_overflow:
		case llvm::Intrinsic::ssub_with_overflow:
			new_inst_max = CmpInst::Create(llvm::AddrSpaceCastInst::ICmp,llvm::CmpInst::ICMP_SGT,re,maxsignedval,"",old);
			new_inst_min = CmpInst::Create(llvm::AddrSpaceCastInst::ICmp,llvm::CmpInst::ICMP_SLT,re,minsignedval,"",old);
			break;
		case llvm::Intrinsic::uadd_with_overflow:
		case llvm::Intrinsic::umul_with_overflow:
		case llvm::Intrinsic::usub_with_overflow:
			// maxunsignedvalue is actually -1 in LLVM
			// then, the first test cannot be handled by Pagai since it does not
			// distinguish between signed and unsigned when interpreting
			// conditions...
			new_inst_max = CmpInst::Create(llvm::AddrSpaceCastInst::ICmp,llvm::CmpInst::ICMP_ULT,re,maxunsignedval,"",old);
			//new_inst_max = CmpInst::Create(llvm::AddrSpaceCastInst::ICmp,llvm::CmpInst::ICMP_UGT,re,maxunsignedval,"",old);
			new_inst_min = CmpInst::Create(llvm::AddrSpaceCastInst::ICmp,llvm::CmpInst::ICMP_ULT,re,minunsignedval,"",old);
			break;
		default:
			break;
	}
	new_inst = BinaryOperator::Create(llvm::Instruction::Or,new_inst_max,new_inst_min);

	if (dbg != NULL) {
		new_inst->setMetadata(LLVMContext::MD_dbg,dbg);
		new_inst_min->setMetadata(LLVMContext::MD_dbg,dbg);
		new_inst_max->setMetadata(LLVMContext::MD_dbg,dbg);
	}

	ReplaceInstWithInst(old, new_inst);

	// we should now delete the intrinsic function with overflow
	if (intrinsicCall->use_empty())
		intrinsicCall->eraseFromParent();
}

char instrOverflow::ID = 0;
static RegisterPass<instrOverflow> X("instrOverflow", "Overflow Instrumentation Pass", false, false);
