/**
 * \file Expr.cc
 * \brief Implementation of the Expr class
 * \author Julien Henry
 */

#define UNDEF_ADDRESS 0x0
#include <map>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"

#include "Expr.h"

#if LLVM_VERSION_MAJOR>3 || LLVM_VERSION_MAJOR==3 && LLVM_VERSION_MINOR>=6
#include "llvm/Analysis/CFG.h"
#else
#include "llvm/Analysis/CFG.h"
#endif

#include "apron.h"
#include "Debug.h"
#include "Analyzer.h"
#include "AIpass.h"
#include "Live.h"

std::map<Value*, ap_texpr1_t*> Exprs;
std::map<ap_var_t, ap_texpr1_t*> Exprs_var;

void Expr::set_expr(Value * val, Expr * exp) {
	if (Exprs.count(val))
		ap_texpr1_free(Exprs[val]);
	Exprs[val] = ap_texpr1_copy(exp->ap_expr);
}

void Expr::clear_exprs() {
	std::map<Value*, ap_texpr1_t*>::iterator it = Exprs.begin(), et = Exprs.end();
	for (; it != et; it++)
		ap_texpr1_free((*it).second);
	Exprs.clear();

	std::map<ap_var_t, ap_texpr1_t*>::iterator itt = Exprs_var.begin(), ett = Exprs_var.end();
	for (; itt != ett; itt++)
		ap_texpr1_free((*itt).second);
	Exprs_var.clear();
}

ap_texpr1_t * Expr::create_expression(Value * val) {
	ap_expr = NULL;

	if (Exprs.count(val)) {
		ap_expr = ap_texpr1_copy(Exprs[val]);
	} else if (isa<Constant>(val)) {
		ap_expr = create_ap_expr(dyn_cast<Constant>(val));
	} else if (Instruction * I = dyn_cast<Instruction>(val)) {
		ap_expr = visit(*I);
	} else {
		// in case the value is not a constant nor an Instruction (for instance, an
		// Argument), we create a variable
		ap_expr = create_ap_expr((ap_var_t)val);
	}
	return ap_expr;
}

Expr::Expr(Value * val) {
	assert(val != NULL);
	ap_expr = create_expression(val);
}

Expr::Expr(ap_var_t var) {
	if (! Exprs_var.count(var)) {
		Exprs_var[var] = create_ap_expr(var);
	}
	ap_expr = ap_texpr1_copy(Exprs_var[var]);
}

Expr::Expr(ap_texpr_op_t op, Expr * exp1, Expr * exp2, ap_texpr_rtype_t type, ap_texpr_rdir_t round) {
	ap_expr = ap_texpr1_binop(op,
			ap_texpr1_copy(exp1->ap_expr),
			ap_texpr1_copy(exp2->ap_expr),
			type,
			round);
}

Expr::~Expr() {
	ap_texpr1_free(ap_expr);
}

Expr::Expr(const Expr &exp) {
	ap_expr = ap_texpr1_copy(exp.ap_expr);
}

Expr::Expr(Expr * exp) {
	ap_expr = ap_texpr1_copy(exp->ap_expr);
}

Expr::Expr(double d) {
	Environment env;
	ap_expr = ap_texpr1_cst_scalar_double(env.getEnv(),d);
}

Expr & Expr::operator= (const Expr & exp) {
	ap_texpr1_free(ap_expr);
	ap_expr = ap_texpr1_copy(exp.ap_expr);
	return *this;
}

ap_texpr1_t * Expr::getExpr() {
	return ap_expr;
}

Environment * Expr::getEnv() {
	return new Environment(ap_expr->env);
}

void Expr::print() {
	texpr1_print(ap_expr);
}

ap_texpr1_t * Expr::create_ap_expr(Constant * val) {
	ap_texpr1_t * res = NULL;
	ap_environment_t * emptyenv = ap_environment_alloc_empty();
	if (isa<ConstantInt>(val)) {
		ConstantInt * Int = dyn_cast<ConstantInt>(val);
		// it is supposed we use signed int
		int64_t i = Int->getSExtValue();
		res = ap_texpr1_cst_scalar_int(emptyenv,i);
	} 
	if (isa<ConstantFP>(val)) {
		ConstantFP * FP = dyn_cast<ConstantFP>(val);
		double x;
		if (FP->getType()->isFloatTy()) {
			x = (double)FP->getValueAPF().convertToFloat();
		} else if (FP->getType()->isDoubleTy()) {
			x = FP->getValueAPF().convertToDouble();
		}
		if (FP->getValueAPF().isNaN() 
				|| FP->getValueAPF().isInfinity()
				|| std::isnan(x)
				|| std::isinf(x)) {
			// x is nan or is actually infinity
			// in this case, we suppose it is an unknown value (as in the SMT
			// part)
			res = create_ap_expr((ap_var_t)val);
		} else {
			res = ap_texpr1_cst_scalar_double(emptyenv,x);
		}
	}
	if (isa<ConstantPointerNull>(val)) {
		res = ap_texpr1_cst_scalar_int(emptyenv,0);
	}
	if (isa<UndefValue>(val)) {
		//assert(false && "non-initialised value in the analysed code");
		UndefValue * undef = dyn_cast<UndefValue>(val);
		res = create_ap_expr(undef);
	}
	if (res == NULL)
		res = create_ap_expr((ap_var_t)val);
	ap_environment_free(emptyenv);
	return res;
}

int* undef_ai_counter = 0;
std::map<ap_var_t,ap_texpr_rtype_t> undef_ap_vars;

bool Expr::is_undef_ap_var(ap_var_t var) {
	return undef_ap_vars.count(var);
}

ap_texpr1_t * Expr::create_ap_expr(UndefValue * undef) {
	ap_texpr_rtype_t ap_type;
	if (get_ap_type(undef, ap_type)) return NULL;
	// EACH TIME we encounter an undefined value, we have to assign it an 
	// expression. However, in LLVM, there is only one UndefValue object and everyone
	// points to it. As a consequence, we cannot use the address of the UndefValue
	// to uniquely identify the undef value among the other undef uses in the code. 
	// The trick is to use a counter, incremented each time we see an Undefvalue, 
	// and use a fake address (UNDEF_ADDRESS + counter) which is unique to represent the 
	// particular use of undef.
	// TODO: this should be fixed: if we are unlucky, after some time the computed 
	// address might equal the real address of another variable...
	undef_ai_counter++;
	ap_var_t v = (ap_var_t)(UNDEF_ADDRESS + undef_ai_counter);
	undef_ap_vars.insert(std::pair<ap_var_t,ap_texpr_rtype_t>(v,ap_type));
	return create_ap_expr(v);
}

ap_texpr1_t * Expr::create_ap_expr(ap_var_t var) {
	ap_environment_t* env = NULL;
	ap_texpr_rtype_t ap_type;

	if (is_undef_ap_var(var)) {
		ap_type = undef_ap_vars[var];
	} else {
		if (get_ap_type((Value*)var, ap_type)) return NULL;
	}

	if (ap_type == AP_RTYPE_INT) { 
		env = ap_environment_alloc(&var,1,NULL,0);
	} else {
		env = ap_environment_alloc(NULL,0,&var,1);
	}
	ap_texpr1_t * res = ap_texpr1_var(env,var);
	ap_environment_free(env);
	return res;
}

void Expr::create_constraints (
		ap_constyp_t constyp,
		Expr * expr,
		Expr * nexpr,
		std::vector<Constraint*> * t_cons
		) {

	Constraint * cons;

	if (constyp == AP_CONS_DISEQ) {
		// we have a disequality constraint. We tranform it into 2 different
		// constraints: < and >, in order to create further 2 abstract values
		// instead of one.
		cons = new Constraint(AP_CONS_SUP, expr,NULL);
		t_cons->push_back(cons);

		cons = new Constraint(AP_CONS_SUP, nexpr,NULL);
		t_cons->push_back(cons);

	} else {
		cons = new Constraint(constyp, expr,NULL);
		t_cons->push_back(cons);
	}
}

void Expr::common_environment(Expr* exp1, Expr* exp2) {
	Environment common(Environment::common_environment(exp1,exp2));
#if 1
	ap_texpr1_extend_environment_with(exp1->ap_expr,common.getEnv());
	ap_texpr1_extend_environment_with(exp2->ap_expr,common.getEnv());
#else
	/* equivalent */
	ap_texpr1_t * exp1_new = ap_texpr1_extend_environment(exp1->ap_expr,common.getEnv());
	ap_texpr1_t * exp2_new = ap_texpr1_extend_environment(exp2->ap_expr,common.getEnv());

	ap_texpr1_free(exp1->ap_expr);
	ap_texpr1_free(exp2->ap_expr);
	exp1->ap_expr = exp1_new;
	exp2->ap_expr = exp2_new;
#endif
}

// returns:
// * 0 if the value has an interesting type (i.e. integer or float)
// * 1 if the value has an unknown type
// * 2 if the value is a boolean
int Expr::get_ap_type(Value * val,ap_texpr_rtype_t &ap_type) {

	switch (val->getType()->getTypeID()) {
		case Type::FloatTyID:
			ap_type = AP_RTYPE_REAL;
			break;
		case Type::DoubleTyID:
			ap_type = AP_RTYPE_REAL;
			break;
		case Type::IntegerTyID:
			ap_type = AP_RTYPE_INT;
			if (val->getType()->getPrimitiveSizeInBits() == 1) {
				// actually, this is a boolean variable
				return 2;
			}
			break;
		case Type::PointerTyID:
			if (pointer_arithmetic()) {
				// pointers are considered normal integers
				ap_type = AP_RTYPE_INT;
				if (val->getType()->getPrimitiveSizeInBits() == 1) {
					// actually, this is a boolean variable
					return 2;
				}
			} else { 
				// pointers are considered unknown type
				ap_type = AP_RTYPE_REAL;
				return 1;
			}
			break;
		case Type::X86_FP80TyID:
			ap_type = AP_RTYPE_REAL;
			break;
		case Type::PPC_FP128TyID:
			ap_type = AP_RTYPE_REAL;
			break;
		default:
			// unknown type
			ap_type = AP_RTYPE_REAL;
			return 1;
	}
	return 0;
}


void Expr::texpr1_print(ap_texpr1_t * expr) {

	FILE* tmp = tmpfile();
	if (tmp == NULL) {
		*Out << "ERROR: tmpfile has not been created\n";
		return;
	}

	ap_texpr1_fprint(tmp,expr);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
	fclose(tmp);
}

void Expr::tcons1_array_print(ap_tcons1_array_t * cons) {

	FILE* tmp = tmpfile();
	if (tmp == NULL) {
		*Out << "ERROR: tmpfile has not been created\n";
		return;
	}

	ap_tcons1_array_fprint(tmp,cons);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
	fclose(tmp);
}

ap_texpr1_t * Expr::visitReturnInst (ReturnInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitBranchInst (BranchInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitSwitchInst (SwitchInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitIndirectBrInst (IndirectBrInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitInvokeInst (InvokeInst &I){
	return visitInstAndAddVar(I);
}

#if LLVM_VERSION_MAJOR < 3 || LLVM_VERSION_MINOR == 0
ap_texpr1_t * Expr::visitUnwindInst (UnwindInst &I){
	return visitInstAndAddVar(I);
}
#endif

ap_texpr1_t * Expr::visitUnreachableInst (UnreachableInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitICmpInst (ICmpInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFCmpInst (FCmpInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitAllocaInst (AllocaInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitLoadInst (LoadInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitStoreInst (StoreInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitGetElementPtrInst (GetElementPtrInst &I){
	if (pointer_arithmetic()) {
		const PointerType* ptrType = static_cast<PointerType*>(I.getPointerOperandType());
		Type* type = ptrType -> getElementType();

		const BasicBlock *bb = I.getParent();
		const Function *fn = bb->getParent();
		const Module *mod = fn->getParent();
		const DataLayout layout(mod);
		const ::uint64_t size=layout.getTypeAllocSize(type);

		Value * op1 = I.getOperand(0);
		Value * op2 = I.getOperand(1);
		ap_texpr1_t * exp1 = create_expression(op1);
		ap_texpr1_t * exp2 = create_expression(op2);
		Environment::common_environment(exp1,exp2);
		ap_texpr1_t * exp3 = ap_texpr1_cst_scalar_int(exp1->env, size);
		ap_texpr1_t * exp4 = ap_texpr1_binop(AP_TEXPR_MUL, exp2, exp3, AP_RTYPE_INT, AP_RDIR_RND);

		ap_texpr1_t * exp = ap_texpr1_binop(AP_TEXPR_ADD, exp1, exp4, AP_RTYPE_INT, AP_RDIR_RND);

		return exp;
	} else {
		return visitInstAndAddVar(I);
	}
}

ap_texpr1_t * Expr::visitPHINode (PHINode &I){
	Value * pv;
	// if the PHINode has actually one single incoming edge, we just say the
	// value is equal to its associated expression
	// There is no need to introduce PHIvars...
	if (I.getNumIncomingValues() == 1) {
		pv = I.getIncomingValue(0);
		ap_texpr1_t * res;
		if (isa<UndefValue>(pv)) {
			res = create_ap_expr(dyn_cast<Constant>(pv));
		}
		if (isa<Constant>(pv)) {
			res = create_ap_expr(dyn_cast<Constant>(pv));
		}

		if (Instruction * inst = dyn_cast<Instruction>(pv)) {
			res = visit(inst);
		}
		return res;
	} else {
		return visitInstAndAddVar(I);
	}
}

ap_texpr1_t * Expr::visitTruncInst (TruncInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitZExtInst (ZExtInst &I){

	Value * pv;
	Node * nb;
	if(I.getSrcTy()->isIntegerTy(1) && I.getDestTy()->isIntegerTy()) {
		// we cast a boolean to an integer
		// we overapproximate here...
	} else if(I.getSrcTy()->isIntegerTy() && I.getDestTy()->isIntegerTy()) {
		return create_expression(I.getOperand(0));
	}
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitSExtInst (SExtInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFPTruncInst (FPTruncInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFPExtInst (FPExtInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFPToUIInst (FPToUIInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFPToSIInst (FPToSIInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitUIToFPInst (UIToFPInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitSIToFPInst (SIToFPInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitPtrToIntInst (PtrToIntInst &I){
	if (pointer_arithmetic())
		return create_expression(I.getOperand(0));
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitIntToPtrInst (IntToPtrInst &I){
	if (pointer_arithmetic())
		return create_expression(I.getOperand(0));
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitBitCastInst (BitCastInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitSelectInst (SelectInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitCallInst(CallInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitVAArgInst (VAArgInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitExtractElementInst (ExtractElementInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitInsertElementInst (InsertElementInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitShuffleVectorInst (ShuffleVectorInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitExtractValueInst (ExtractValueInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitInsertValueInst (InsertValueInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitTerminatorInst (TerminatorInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitBinaryOperator (BinaryOperator &I){
	ap_texpr_op_t op;
	switch(I.getOpcode()) {
		// Standard binary operators...
		case Instruction::Add : 
		case Instruction::FAdd: 
			op = AP_TEXPR_ADD;
			break;
		case Instruction::Sub : 
		case Instruction::FSub: 
			op = AP_TEXPR_SUB;
			break;
		case Instruction::Mul : 
		case Instruction::FMul: 
			if (skipNonLinear()) return visitInstAndAddVar(I);
			op = AP_TEXPR_MUL;
			break;
		case Instruction::UDiv: 
		case Instruction::SDiv: 
		case Instruction::FDiv: 
			if (skipNonLinear()) return visitInstAndAddVar(I);
			op = AP_TEXPR_DIV;
			break;
		case Instruction::URem: 
		case Instruction::SRem: 
		case Instruction::FRem: 
			if (skipNonLinear()) return visitInstAndAddVar(I);
			op = AP_TEXPR_MOD;
			break;
			// Logical operators
		case Instruction::Shl : // Shift left  (logical)
		case Instruction::LShr: // Shift right (logical)
		case Instruction::AShr: // Shift right (arithmetic)
		case Instruction::And :
		case Instruction::Or  :
		case Instruction::Xor :
		case Instruction::BinaryOpsEnd:
			// we consider the result is unknown
			// so we create a new variable
			return visitInstAndAddVar(I);
	}
	ap_texpr_rtype_t type;
	get_ap_type(&I,type);
	ap_texpr_rdir_t dir = AP_RDIR_RND;
	Value * op1 = I.getOperand(0);
	Value * op2 = I.getOperand(1);

	ap_texpr1_t * exp1 = create_expression(op1);
	ap_texpr1_t * exp2 = create_expression(op2);
	Environment::common_environment(exp1,exp2);

	// we create the expression associated to the binary op
	ap_texpr1_t * exp = ap_texpr1_binop(op, ap_texpr1_copy(exp1), ap_texpr1_copy(exp2), type, dir);

	ap_texpr1_free(exp1);
	ap_texpr1_free(exp2);
	return exp;
}

ap_texpr1_t * Expr::visitCmpInst (CmpInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitCastInst (CastInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitInstAndAddVar(Instruction &I) {
	ap_environment_t* env = NULL;
	ap_var_t var = (Value *) &I; 
	ap_texpr_rtype_t ap_type;

	if (get_ap_type((Value*)&I, ap_type)) {
		return NULL;
	}

	if (ap_type == AP_RTYPE_INT) { 
		env = ap_environment_alloc(&var,1,NULL,0);
	} else {
		env = ap_environment_alloc(NULL,0,&var,1);
	}
	ap_texpr1_t * exp = ap_texpr1_var(env,var);
	ap_environment_free(env);
	return exp;
}
