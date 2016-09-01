/**
 * \file SMTpass.cc
 * \brief Implementation of the SMTpass pass
 * \author Julien Henry
 */
#include <sstream>
#include <vector>
#include <queue>
#include <iostream>
#include <string>
#include <limits>
#include <cmath>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "llvm/Analysis/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"

#include "Pr.h"
#include "SMTpass.h"
#include "z3_manager.h"
#include "yices.h"
#include "SMTlib.h"
#include "Expr.h"
#include "apron.h"
#include "Debug.h"

/*
DM: If set to 0, modulo (grid) constraints are not converted to SMT.
If set to 1, they are - this causes segfaults in Z3 3.8 (but not 4.0+).
*/
#define SMT_HAS_WORKING_MODULO 1

using namespace std;
using namespace boost;

std::map<BasicBlock*,int> NodeNames;
std::map<int,BasicBlock*> NodeAddress;

int CurrentNodeName;

int SMTpass::nundef = 0;

SMTpass::SMTpass() {
	switch (getSMTSolver()) {
#if HAS_Z3
		case API_Z3:
			man = new z3_manager();
			break;
#endif
#ifdef HAS_YICES
		case API_YICES: 
			man = new yices();
			break;
#endif
		default:
			man = new SMTlib();
	}
	stack_level = 0;
	CurrentNodeName = 0;
}

SMTpass::~SMTpass() {
	delete man;
}

SMTpass * instance = NULL;
SMTpass * instanceforAbstract = NULL;

SMTpass * SMTpass::getInstance() {
	if (instance == NULL)
		instance = new SMTpass();
	return instance;
}

SMTpass * SMTpass::getInstanceForAbstract() {
	if (instanceforAbstract == NULL)
		instanceforAbstract = new SMTpass();
	return instanceforAbstract;
}

void SMTpass::releaseMemory() {
	if (instance != NULL) {
		delete instance;
		instance = NULL;
	}
	if (instanceforAbstract != NULL) {
		delete instanceforAbstract;
		instanceforAbstract = NULL;
	}
}

SMT_expr SMTpass::getRho(Function &F) {
	if (!rho.count(&F))
		computeRho(F);
	return rho[&F];
}

void SMTpass::reset_SMTcontext() {
	rho.clear();
#if 0
	while (stack_level > 0)
		pop_context();
#else
	switch (getSMTSolver()) {
#ifdef HAS_Z3
		case API_Z3:
			stack_level = 0;
			delete man;
			man = new z3_manager();
			break;
#endif
#ifdef HAS_YICES
		case API_YICES: 
			stack_level = 0;
			delete man;
			man = new yices();
			break;
#endif
		default:
			while (stack_level > 0)
				pop_context();
			//man = new SMTlib();
	}
#endif
}

SMT_expr SMTpass::texpr1ToSmt(ap_texpr1_t texpr) {
	// NOT IMPLEMENTED
	SMT_expr NULL_expr;
	return NULL_expr;
}

SMT_expr SMTpass::linexpr1ToSmt(BasicBlock* b, ap_linexpr1_t linexpr, bool &integer, bool &skip) {
	std::vector<SMT_expr> elts;
	SMT_expr val;
	SMT_expr coefficient;
	bool infinity;

	skip = false;
	integer = true;
	double value;
	size_t i;
	ap_var_t var;
	ap_coeff_t* coeff;

	// first, we figure out whether the expression has to be of type real instead of
	// integer
	// we iterate over the terms and find if there is some real variables
	ap_linexpr1_ForeachLinterm1(&linexpr,i,var,coeff){ 
		if (!((Value*)var)->getType()->isIntegerTy()) {
			// check if the coeff associated to this non-int variable is != 0
			// if it is, then the expression has to be of type real
			coefficient = scalarToSmt(coeff->val.scalar,integer,value,infinity);
			if (infinity) {
				skip = true;
				return man->SMT_mk_num(0);
			}
			if (value != 0) {
				integer = false;
			}
		}
	}

	ap_linexpr1_ForeachLinterm1(&linexpr,i,var,coeff){ 
		bool primed;
		if (Instruction * I = dyn_cast<Instruction>((Value*)var)) {
			primed = is_primed(b,*I);
		} else {
			primed = false;
		}
		val = getValueExpr((Value*)var, primed);

		if ( !integer && ((Value*)var)->getType()->isIntegerTy()) {
			val = man->SMT_mk_int2real(val);
		}

		coefficient = scalarToSmt(coeff->val.scalar,integer,value,infinity);
		if (infinity) {
			skip = true;
			return man->SMT_mk_num(0);
		}

		switch ((int)value) {
			case 0:
				break;
			case 1:
				elts.push_back(val);
				break;
			default:
				elts.push_back(man->SMT_mk_mul(val,coefficient));
				break;
		}
	}
	coeff = ap_linexpr1_cstref(&linexpr);
	coefficient = scalarToSmt(coeff->val.scalar,integer,value,infinity);
	if (infinity) {
		skip = true;
		return man->SMT_mk_num(0);
	}
	if (value != 0)
		elts.push_back(coefficient);

	return man->SMT_mk_sum(elts);
}

SMT_expr SMTpass::scalarToSmt(ap_scalar_t * scalar, bool integer, double &value, bool &infinity) {
	mp_rnd_t round = GMP_RNDU;
	ap_double_set_scalar(&value,scalar,round);
	if (std::isinf(value)) {
		infinity = true;
		// will be unused, but we must return something
		return man->SMT_mk_real(0);
	}
	infinity = false;
	if (integer) {
		mpq_t mpq;
		mpq_init(mpq);
		ap_mpq_set_scalar(mpq,scalar,round);
		SMT_expr res = man->SMT_mk_num_mpq(mpq);
		mpq_clear(mpq);
		return res;
	} else {
		return man->SMT_mk_real(value);
	}
}

SMT_expr SMTpass::lincons1ToSmt(BasicBlock * b, ap_lincons1_t lincons, bool &skip) {
	ap_constyp_t* constyp = ap_lincons1_constypref(&lincons);
	ap_linexpr1_t linexpr = ap_lincons1_linexpr1ref(&lincons);
	//ap_coeff_t * coeff = ap_lincons1_cstref(&lincons);
	SMT_expr scalar_smt;
	bool integer;
	SMT_expr linexpr_smt = linexpr1ToSmt(b, linexpr, integer, skip);

	if (skip)
		return man->SMT_mk_true();

	if (integer || *constyp == AP_CONS_EQMOD)
		scalar_smt = man-> SMT_mk_int0();
	else
		scalar_smt = man-> SMT_mk_real0();

	switch (*constyp) {
		case AP_CONS_EQ:
			return man->SMT_mk_eq(linexpr_smt,scalar_smt);
		case AP_CONS_SUPEQ:
			return man->SMT_mk_ge(linexpr_smt,scalar_smt);
		case AP_CONS_SUP: 
			return man->SMT_mk_gt(linexpr_smt,scalar_smt);
		case AP_CONS_EQMOD:
			{
				double value; // TODO double bof
				bool infinity;
				SMT_expr modulo = scalarToSmt(ap_lincons1_lincons0ref(&lincons)->scalar,true,value,infinity);
				assert(!(value == 0));
				assert(infinity == false);

				if (integer) {
					return man->SMT_mk_divides(modulo, linexpr_smt); // assumes scalar_smt is 0
				} else {
#if SMT_HAS_WORKING_MODULO
					// This segfaults in Z3 3.8
					SMT_expr test = man->SMT_mk_is_int(linexpr_smt);
					if (value == 1) {
						return test;
					} else {
						SMT_expr intexpr = man->SMT_mk_real2int(linexpr_smt);
						std::vector<SMT_expr> args;
						args.push_back(test);
						args.push_back( man->SMT_mk_divides(modulo, intexpr)); // assumes scalar_smt is zero
						return man->SMT_mk_and(args);
					}
#else
					return man->SMT_mk_true();
#endif
				}
			}
		case AP_CONS_DISEQ:
			return man->SMT_mk_diseq(linexpr_smt,scalar_smt);
	}
	// unreachable
	SMT_expr NULL_expr;
	return NULL_expr;
}

SMT_expr SMTpass::tcons1ToSmt(ap_tcons1_t tcons) {
	ap_constyp_t* constyp = ap_tcons1_constypref(&tcons);
	ap_texpr1_t texpr = ap_tcons1_texpr1ref(&tcons);
	ap_scalar_t* scalar = ap_tcons1_scalarref(&tcons);
	bool integer = true;
	double value;
	bool infinity;
	SMT_expr texpr_smt = texpr1ToSmt(texpr);
	SMT_expr scalar_smt = scalarToSmt(scalar,integer,value,infinity);
	if (infinity)
		return man->SMT_mk_num(0);

	switch (*constyp) {
		case AP_CONS_EQ:
			return man->SMT_mk_eq(texpr_smt,scalar_smt);
		case AP_CONS_SUPEQ:
			return man->SMT_mk_ge(texpr_smt,scalar_smt);
		case AP_CONS_SUP: 
			return man->SMT_mk_gt(texpr_smt,scalar_smt);
		case AP_CONS_EQMOD:
			// TODO : this is not correct 
			return man->SMT_mk_eq(texpr_smt,scalar_smt);
		case AP_CONS_DISEQ:
			return man->SMT_mk_diseq(texpr_smt,scalar_smt);
	}
	// unreachable
	SMT_expr NULL_expr;
	return NULL_expr;
}


SMT_expr SMTpass::AbstractDisjToSmt(BasicBlock * b, AbstractDisj * A, bool insert_booleans) {
	std::vector<SMT_expr> disj;
	int N = A->getMaxIndex();
	if (insert_booleans) {
		std::vector<SMT_expr> D;
		// we create a boolean predicate for each disjunct
		for (int index = 0;index <= N; index++) {
			SMT_var dvar = man->SMT_mk_bool_var(getDisjunctiveIndexName(A,index));
			D.push_back(man->SMT_mk_expr_from_bool_var(dvar));
		}
		for (int index = 0;index <= N; index++) {
			std::vector<SMT_expr> cunj;
			cunj.push_back(AbstractToSmt(b,A->getDisjunct(index)));
			for (int j = 0;j <= N; j++) {
				if (index == j)
					cunj.push_back(D[j]);
				else
					cunj.push_back(man->SMT_mk_not(D[j]));
			}
			disj.push_back(man->SMT_mk_and(cunj));
		}
	} else {
		for (int index = 0;index <= N; index++) {
			disj.push_back(AbstractToSmt(b,A->getDisjunct(index)));
		}
	}
	return man->SMT_mk_or(disj);
}

SMT_expr SMTpass::AbstractToSmt(BasicBlock * b, Abstract * A) {
	if (A->is_bottom()) return man->SMT_mk_false();
	if (AbstractDisj * Adis = dynamic_cast<AbstractDisj*>(A)) 
		return AbstractDisjToSmt(b,Adis,false);

	std::vector<SMT_expr> constraints;
	ap_lincons1_t lincons;
	ap_lincons1_array_t lincons_array = A->to_lincons_array();
	size_t n = ap_lincons1_array_size(&lincons_array);
	bool skip;
	for (size_t i = 0; i < n; i++) {
		lincons = ap_lincons1_array_get(&lincons_array,i);
		constraints.push_back(lincons1ToSmt(b,lincons,skip));
	}
	ap_lincons1_array_clear(&lincons_array);
	if (constraints.size() == 0)
		return man->SMT_mk_true();
	else
		return man->SMT_mk_and(constraints);
}

const std::string SMTpass::getDisjunctiveIndexName(AbstractDisj * A, int index) {
	std::ostringstream name;
	name << "d_" << A << "_" << index;
	return name.str();
}

const std::string SMTpass::getUndeterministicChoiceName(Value * v) {
	std::ostringstream name;
	name << "c_" << v;
	return name.str();
}

const std::string SMTpass::getNodeSubName(BasicBlock * b) {
	if (NodeNames.count(b) == 0) {
		NodeNames[b] = CurrentNodeName;
		NodeAddress[CurrentNodeName] = b;
		CurrentNodeName++;
	}
	std::ostringstream oss;
	oss << NodeNames[b];
	return oss.str();
}

BasicBlock * SMTpass::getNodeBasicBlock(std::string name) {
	return NodeAddress[boost::lexical_cast<int>(name)];
}

const std::string SMTpass::getNodeName(BasicBlock* b, bool src) {
	std::string name;
	Pr * FPr = Pr::getInstance(b->getParent());
	if (FPr->inPr(b)) {
		if (src)
			name = "bs_";
		else
			name = "bd_";
	} else {
		name = "b_";
	}

	name += getNodeSubName(b);
	return name;
}

const std::string SMTpass::getEdgeName(BasicBlock* b1, BasicBlock* b2) {
	std::string name;
	name =  "t_" 
		+ getNodeSubName(b1)
		+ "_" 
		+ getNodeSubName(b2);
	return name;
}

const std::string SMTpass::getValueName(Value * v, bool primed) {
	std::string name;
	std::string var = getVarName(v);
	if (primed)
		name = "x_prime_" + var + "_";
	else
		name = "x_" + var + "_";
	return name;
}


std::map<Value*,std::string> SMTpass::VarNames;
int VarNames_unnamed = 0;

const std::string SMTpass::getVarName(Value * v) {
	if (VarNames.count(v) && !isa<UndefValue>(v)) {
		return VarNames[v];
	}

	std::string s_string;
	raw_string_ostream * s = new raw_string_ostream(s_string);

	if (v->hasName()) {
		*s << v->getName();
	} else {
		VarNames_unnamed++;
		//*s << *v;
		*s << "unnamed_" << VarNames_unnamed;
	}

	std::string & name = s->str();
	//*Dbg << name << "\n";
	size_t found;
	found=name.find_first_of("%");
	if (found!=std::string::npos) {
		name = name.substr(found);
	}
	found=name.find_first_of(" ");
	if (found!=std::string::npos) {
		name.resize(found);
	}
	delete s;
	if (!isa<UndefValue>(v))
		VarNames[v] = name;
	return name;
}

SMT_type SMTpass::getValueType(Value * v) {
	switch (v->getType()->getTypeID()) {
		case Type::PointerTyID:
			if (!pointer_arithmetic()) {assert(false && "this code should be unreachable");};
			// no break is done on purpose, since pointer == integer in this case
		case Type::IntegerTyID:
			if (v->getType()->getPrimitiveSizeInBits() == 1) {
				return man->bool_type;
			} else {
				return man->int_type;
			}
		default:
			return man->float_type;
	}
}

SMT_var SMTpass::getVar(Value * v, bool primed) {
	std::string name = getValueName(v,primed);
	return man->SMT_mk_var(name,getValueType(v));
}

SMT_var SMTpass::getBoolVar(Value * v, bool primed) {
	std::string name = getValueName(v,primed);
	return man->SMT_mk_bool_var(name);
}

SMT_expr SMTpass::getValueExpr(Value * v, bool primed) {
	SMT_var var;
	SMT_expr NULL_res;

	ap_texpr_rtype_t ap_type;
	switch (Expr::get_ap_type(v, ap_type)) {
		case 0: // this is an integer or float
			if (isa<ConstantInt>(v)) { 
				ConstantInt * Int = dyn_cast<ConstantInt>(v);
				int n = Int->getSExtValue();
				return man->SMT_mk_num((int)n);
			} else if (isa<ConstantPointerNull>(v)) {
				return man->SMT_mk_num((int)0);
			} else if (isa<ConstantFP>(v)) {
				ConstantFP * FP = dyn_cast<ConstantFP>(v);
				APFloat APF = FP->getValueAPF();
				double x;
				if (FP->getType()->isFloatTy()) {
					x = (double)APF.convertToFloat();
				} else if (FP->getType()->isDoubleTy()) {
					x = APF.convertToDouble();
				}
				if (std::isnan(x) || std::isinf(x)) {
					// x is not a number or is infinity...
					std::ostringstream name;
					name << getValueName(v,false);
					nundef++;
					return man->SMT_mk_expr_from_var(man->SMT_mk_var(name.str(),getValueType(v)));
				}
				return man->SMT_mk_real(x);
			} else if (isa<UndefValue>(v)) {
				std::ostringstream name;
				//name << getValueName(v,false) << "_" << nundef;
				name << getValueName(v,false);
				nundef++;
				return man->SMT_mk_expr_from_var(man->SMT_mk_var(name.str(),getValueType(v)));
			} else if (isa<Instruction>(v) || isa<Argument>(v)) {
				var = getVar(v,primed);
				return man->SMT_mk_expr_from_var(var);
			} else {
				var = getVar(v,primed);
				return man->SMT_mk_expr_from_var(var);
			}
		case 2: // this is a Boolean
			{
				SMT_expr cond;
				if (isa<Instruction>(v) || isa<Argument>(v)) {
					var = getBoolVar(v,primed);
					return man->SMT_mk_expr_from_bool_var(var);
				} else if (ConstantInt * vint = dyn_cast<ConstantInt>(v)) {
					// the constant is either true or false
					if (vint->isZero()) {
						cond = man->SMT_mk_false();
					} else if (vint->isOne()) {
						cond = man->SMT_mk_true();
					}
				} 
				if (cond.is_empty()) {
					SMT_var cvar = man->SMT_mk_bool_var(getUndeterministicChoiceName(v));
					cond = man->SMT_mk_expr_from_bool_var(cvar);
				}
				return cond;
			}
		case 1: // this value is not of interesting type
			*Dbg << "ERROR: getValueExpr returns NULL for the following value:\n" << *v << "\n";
			assert(false && "ERROR: getValueExpr returns NULL");
	}
	return NULL_res;
}

void SMTpass::getElementFromString(
		std::string name,
		bool &isEdge,
		bool &isIndex,
		bool &start,
		BasicBlock * &src,
		BasicBlock * &dest,
		int &index) {

	std::string edge ("t_");
	std::string simple_node ("b_");
	std::string source_node ("bs_");
	std::string dest_node ("bd_");
	std::string disjunctive_index ("d_");
	size_t found;
	void* address;
	src = NULL;
	dest = NULL;
	start = false;

	vector<std::string> fields;
	split( fields, name, is_any_of( "_" ) );

	// case 1 : this is an edge
	found = name.substr(0,edge.size()).find(edge);
	if (found!=std::string::npos) {
		isEdge = true;
		src = getNodeBasicBlock(fields[1]);
		dest = getNodeBasicBlock(fields[2]);
		return;
	}
	isEdge = false;

	// case 2 : this is a disjunctive index
	found = name.substr(0,disjunctive_index.size()).find(disjunctive_index);
	if (found!=std::string::npos) {
		isIndex = true;
		std::string source = name.substr (2,9);
		std::string stringindex(name);
		stringindex.erase(0,12);	
		std::istringstream indexStream(stringindex);
		indexStream >> index;
		return;
	}
	isIndex = false;

	std::string nodename;
	// case 3 : this is a node
	found = name.substr(0,simple_node.size()).find(simple_node);
	if (found!=std::string::npos) {
		// this is a node of the form b_*
		src = getNodeBasicBlock(fields[1]);
	} else {
		found = name.substr(0,source_node.size()).find(source_node);
		if (found==std::string::npos) found = name.find(dest_node);
		else start = true;
		if (found!=std::string::npos) {
			src = getNodeBasicBlock(fields[1]);
		}
	}
}

void SMTpass::computePrSuccAndPred(
		Function &F
		) {
	Pr * FPr = Pr::getInstance(&F);
	std::set<BasicBlock*>::iterator i = FPr->getPr()->begin(), e = FPr->getPr()->end();
	for (;i!= e; ++i) {
		int counter = 0;
		BasicBlock * dest = *i;
		std::set<BasicBlock*> visited;
		std::queue<BasicBlock*> ToBeComputed;
		ToBeComputed.push(dest);
		while (!ToBeComputed.empty()) {
			counter++;
			BasicBlock * b = ToBeComputed.front();
			ToBeComputed.pop();
			if (visited.count(b)) continue;
			visited.insert(b);
			if (!FPr->inPr(b) || b == dest) {
				BasicBlock * pred;
				for (pred_iterator p = pred_begin(b), E = pred_end(b); p != E; ++p) {
					pred = *p;
					if (!FPr->inPr(pred)) {
						if (!visited.count(pred)) {
							ToBeComputed.push(pred);
						}
					} else {
						FPr->Pr_pred[dest].insert(pred);
						FPr->Pr_succ[pred].insert(dest);
					}
				}
			}
		}
	}
}

void SMTpass::computeRhoRec(Function &F, 
		std::set<BasicBlock*> * visited,
		BasicBlock * dest) {
	Pr * FPr = Pr::getInstance(&F);

	std::queue<BasicBlock*> ToBeComputed;
	ToBeComputed.push(dest);

	while (!ToBeComputed.empty()) {
		BasicBlock * b = ToBeComputed.front();
		ToBeComputed.pop();
		bool first = (visited->count(b) > 0);
		visited->insert(b);
		if (first) continue;

		if (!FPr->inPr(b) || b == dest) {
			// we recursively construct Rho, starting from the predecessors
			BasicBlock * pred;
			for (pred_iterator p = pred_begin(b), E = pred_end(b); p != E; ++p) {
				pred = *p;
				if (!FPr->inPr(pred)) {
					if (!visited->count(pred)) {
						ToBeComputed.push(pred);
					}
				}
			}
		}

		if (first) continue;

		// we create a boolean reachability predicate for the basicblock
		SMT_var bvar = man->SMT_mk_bool_var(getNodeName(b,false));
		// we associate it the right predicate depending on its incoming edges
		std::vector<SMT_expr> predicate;
		for (pred_iterator p = pred_begin(b), e = pred_end(b); p != e; ++p) {
			SMT_var evar = man->SMT_mk_bool_var(getEdgeName(*p,b));
			predicate.push_back(man->SMT_mk_expr_from_bool_var(evar));
		}
		SMT_expr bpredicate;

		if (predicate.size() != 0)
			bpredicate = man->SMT_mk_or(predicate);
		else
			bpredicate = man->SMT_mk_false();

		SMT_expr bvar_exp = man->SMT_mk_expr_from_bool_var(bvar);
		bvar_exp = man->SMT_mk_eq(bvar_exp,bpredicate);
		rho_components.push_back(bvar_exp);
		// we compute the transformation due to the basicblock's
		// instructions
		bvar = man->SMT_mk_bool_var(getNodeName(b,false));
		instructions.clear();

		for (BasicBlock::iterator i = b->begin(), e = b->end();
				i != e; ++i) {
			visit(*i);
		}

		if (instructions.size() > 0) {
			bpredicate = man->SMT_mk_and(instructions);
			instructions.clear();
			SMT_expr bvar_exp = man->SMT_mk_expr_from_bool_var(bvar);
			bvar_exp = man->SMT_mk_not(bvar_exp);
			std::vector<SMT_expr> implies;
			implies.push_back(bvar_exp);
			implies.push_back(bpredicate);
			bvar_exp = man->SMT_mk_or(implies);
			rho_components.push_back(bvar_exp);
		}
	}
}

void SMTpass::computeRho(Function &F) {
	BasicBlock * b;
	Pr * FPr = Pr::getInstance(&F);

	rho_components.clear();
	std::set<BasicBlock*>::iterator i = FPr->getPr()->begin(), e = FPr->getPr()->end();
	std::set<BasicBlock*> visited;
	for (;i!= e; ++i) {
		b = *i;
		computeRhoRec(F,&visited,b);
	}
	rho[&F] = man->SMT_mk_and(rho_components); 
	rho_components.clear();
	computePrSuccAndPred(F);
}


void SMTpass::push_context() {
	stack_level++;
	man->push_context();
}

void SMTpass::pop_context() {
	stack_level--;
	man->pop_context();
}

void SMTpass::SMT_assert(SMT_expr expr) {
	man->SMT_assert(expr);
}

SMT_expr SMTpass::createSMTformula(
		BasicBlock * source, 
		bool use_X_d, 
		params t,
		SMT_expr constraint) {
	Function &F = *source->getParent();
	Pr * FPr = Pr::getInstance(&F);
	std::vector<SMT_expr> formula;
	//formula.push_back(getRho(F));

	SMT_var bvar = man->SMT_mk_bool_var(getNodeName(source,true));
	formula.push_back(man->SMT_mk_expr_from_bool_var(bvar));

	std::set<BasicBlock*>::iterator i = FPr->getPr()->begin(), e = FPr->getPr()->end();
	for (; i!=e; ++i) {
		if (*i != source) {
			bvar = man->SMT_mk_bool_var(getNodeName(*i,true));
			formula.push_back(man->SMT_mk_not(man->SMT_mk_expr_from_bool_var(bvar)));
		}
	}

	Abstract * A = Nodes[source]->X_s[t];
	if (AbstractDisj * Adis = dynamic_cast<AbstractDisj*>(A))
		formula.push_back(AbstractDisjToSmt(NULL,Adis,true));
	else
		formula.push_back(AbstractToSmt(NULL,A));

	std::vector<SMT_expr> Or;
	std::set<BasicBlock*> Successors = FPr->getPrSuccessors(source);
	i = Successors.begin(), e = Successors.end();
	for (; i!=e; ++i) {
		std::vector<SMT_expr> SuccExp;
		SMT_var succvar = man->SMT_mk_bool_var(getNodeName(*i,false));
		SuccExp.push_back(man->SMT_mk_expr_from_bool_var(succvar));

		if (use_X_d)
			SuccExp.push_back(man->SMT_mk_not(AbstractToSmt(*i,Nodes[*i]->X_d[t])));
		else {
			SuccExp.push_back(man->SMT_mk_not(AbstractToSmt(*i,Nodes[*i]->X_s[t])));
		}

		Or.push_back(man->SMT_mk_and(SuccExp));
	}
	// if Or is empty, that means the block has no successor => formula has to
	// be false
	if (Or.size() > 0)
		formula.push_back(man->SMT_mk_or(Or));
	else
		formula.push_back(man->SMT_mk_false());

	// if constraint argument is specified, we insert it into our formula
	if (!constraint.is_empty())
		formula.push_back(constraint);

	return man->SMT_mk_and(formula);
}

int SMTpass::SMTsolve(
		SMT_expr expr, 
		std::list<BasicBlock*> * path, 
		Function * F, 
		params passID) {
	int index;
	return SMTsolve(expr,path,index,F,passID);
}

int SMTpass::SMTsolve(
		SMT_expr expr, 
		std::list<BasicBlock*> * path, 
		int &index,
		Function * F, 
		params passID) {
	std::set<std::string> true_booleans;
	std::map<BasicBlock*, BasicBlock*> succ;
	int res;

	sys::TimeValue time(sys::TimeValue::now());

	res = man->SMT_check(expr,&true_booleans);

	if (Total_time_SMT[passID].count(F) == 0) {
		sys::TimeValue * time_SMT = new sys::TimeValue(0,0);
		Total_time_SMT[passID][F] = time_SMT;
	}

	*Total_time_SMT[passID][F] += sys::TimeValue::now()-time;

	if (res != 1) return res;
	bool isEdge, isIndex, start;
	BasicBlock * src;
	BasicBlock * dest;
	int disj;

	std::set<std::string>::iterator i = true_booleans.begin(), e = true_booleans.end();
	for (; i != e; ++i) {
		std::string name = *i;
		getElementFromString(name,isEdge,isIndex,start,src,dest,disj);
		if (isEdge) {
			succ[src] = dest;
		} else if (isIndex) {
			index = disj;
		} else {
			if (start)
				path->push_back(src);
		}
		Out->flush();
	}

	while (succ.count(path->back())) {
		path->push_back(succ[path->back()]);
		if (path->back() == path->front()) break;
	}
	return res;	
}

int SMTpass::SMTsolve_simple(SMT_expr expr) {
	std::set<std::string> true_booleans;
	return man->SMT_check(expr,&true_booleans);
}

void SMTpass::visitReturnInst (ReturnInst &I) {
}

void SMTpass::visitBranchInst (BranchInst &I) {
	BasicBlock * b = I.getParent();

	SMT_var bvar = man->SMT_mk_bool_var(getNodeName(b,true));
	SMT_expr bexpr = man->SMT_mk_expr_from_bool_var(bvar);
	BasicBlock * s = I.getSuccessor(0);
	SMT_var evar = man->SMT_mk_bool_var(getEdgeName(b,s));
	SMT_expr eexpr = man->SMT_mk_expr_from_bool_var(evar);

	if (I.isUnconditional() || s == I.getSuccessor(1)) {
		rho_components.push_back(man->SMT_mk_eq(eexpr,bexpr));
	} else {
		SMT_expr components_and;
		std::vector<SMT_expr> components;

		SMT_expr cond = getValueExpr(I.getOperand(0),false);

		components.push_back(bexpr);
		if (!cond.is_empty())
			components.push_back(cond);
		components_and = man->SMT_mk_and(components);
		rho_components.push_back(man->SMT_mk_eq(eexpr,components_and));

		components.clear();
		cond = man->SMT_mk_not(cond);
		s = I.getSuccessor(1);
		evar = man->SMT_mk_bool_var(getEdgeName(b,s));
		eexpr = man->SMT_mk_expr_from_bool_var(evar);
		components.push_back(bexpr);
		if (!cond.is_empty())
			components.push_back(cond);
		components_and = man->SMT_mk_and(components);
		components.clear();
		rho_components.push_back(man->SMT_mk_eq(eexpr,components_and));
	}
}

void SMTpass::visitSwitchInst (SwitchInst &I) {
}

void SMTpass::visitIndirectBrInst (IndirectBrInst &I) {
}

void SMTpass::visitInvokeInst (InvokeInst &I) {
	BasicBlock * b = I.getParent();

	SMT_var cvar = man->SMT_mk_bool_var(getUndeterministicChoiceName(&I));
	SMT_expr cexpr = man->SMT_mk_expr_from_bool_var(cvar);

	SMT_var bvar = man->SMT_mk_bool_var(getNodeName(b,true));
	SMT_expr bexpr = man->SMT_mk_expr_from_bool_var(bvar);

	// normal destination
	BasicBlock * s = I.getNormalDest();
	SMT_var evar = man->SMT_mk_bool_var(getEdgeName(b,s));
	SMT_expr eexpr = man->SMT_mk_expr_from_bool_var(evar);

	std::vector<SMT_expr> components;
	components.push_back(bexpr);
	components.push_back(cexpr);
	rho_components.push_back(man->SMT_mk_eq(eexpr,man->SMT_mk_and(components)));

	// Unwind destination
	s = I.getUnwindDest();
	evar = man->SMT_mk_bool_var(getEdgeName(b,s));
	eexpr = man->SMT_mk_expr_from_bool_var(evar);
	components.pop_back();
	components.push_back(man->SMT_mk_not(cexpr));
	rho_components.push_back(man->SMT_mk_eq(eexpr,man->SMT_mk_and(components)));
}

void SMTpass::visitUnreachableInst (UnreachableInst &I) {
}

void SMTpass::visitAllocaInst (AllocaInst &I) {
}

void SMTpass::visitLoadInst (LoadInst &I) {
}

void SMTpass::visitStoreInst (StoreInst &I) {
}

void SMTpass::visitGetElementPtrInst (GetElementPtrInst &I) {
	if (pointer_arithmetic()) { 
		const SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));
		const SMT_expr op0 = getValueExpr(I.getOperand(0), false);
		const SMT_expr op1 = getValueExpr(I.getOperand(1), false);
		const PointerType* ptrType = static_cast<PointerType*>(I.getPointerOperandType());
		Type* type = ptrType -> getElementType();

		const BasicBlock *bb = I.getParent();
		const Function *fn = bb->getParent();
		const Module *mod = fn->getParent();
		const DataLayout layout(mod);
		const ::uint64_t size=layout.getTypeAllocSize(type);

		// TODO check bit width
		SMT_expr prod = man->SMT_mk_mul(man->SMT_mk_num(size), op1);
		SMT_expr assign = man->SMT_mk_sum(op0, prod);

		rho_components.push_back(man->SMT_mk_eq(expr,assign));
	}
}

SMT_expr SMTpass::construct_phi_ite(PHINode &I, unsigned i, unsigned n) {
	if (i == n-1) {
		// this is the last possible value of the PHI-variable
		return getValueExpr(I.getIncomingValue(i), false);
	}
	SMT_expr incomingVal =	getValueExpr(I.getIncomingValue(i), false);

	// incomingVal is empty if the i-th argument in the PHI is not a number
	// (nan)
	if (incomingVal.is_empty()) return incomingVal;

	SMT_var evar = man->SMT_mk_bool_var(getEdgeName(I.getIncomingBlock(i),I.getParent()));
	SMT_expr incomingBlock = man->SMT_mk_expr_from_bool_var(evar);

	SMT_expr tail = construct_phi_ite(I,i+1,n);

	// tail may also be empty if one operand of the phi is nan
	if (tail.is_empty()) return tail;

	return man->SMT_mk_ite(incomingBlock,incomingVal,tail);
}

bool SMTpass::is_primed(BasicBlock * b, Instruction &I) {
	if (b == NULL) return false;
	Function * F = b->getParent();
	Pr * FPr = Pr::getInstance(F);
	return (b != NULL 
			&& (I.getParent() == b && FPr->inPr(b))
			&& isa<PHINode>(I)
	       );
}

void SMTpass::visitPHINode (PHINode &I) {
	ap_texpr_rtype_t ap_type;
	if (Expr::get_ap_type((Value*)&I, ap_type) == 1) return;

	SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
	SMT_expr assign = construct_phi_ite(I,0,I.getNumIncomingValues());

	if (assign.is_empty()) return;

	//if (is_primed(I.getParent(),I) && I.getNumIncomingValues() != 1) {
	if (I.getNumIncomingValues() != 1) {
		SMT_var bvar = man->SMT_mk_bool_var(getNodeName(I.getParent(),false));
		SMT_expr bexpr = man->SMT_mk_not(man->SMT_mk_expr_from_bool_var(bvar));
		std::vector<SMT_expr> disj;
		disj.push_back(bexpr);
		disj.push_back(man->SMT_mk_eq(expr,assign));
		expr = man->SMT_mk_or(disj);	
	} else {
		expr = man->SMT_mk_eq(expr,assign);
	}
	rho_components.push_back(expr);
}

void SMTpass::visitTruncInst (TruncInst &I) {
	if(I.getSrcTy()->isIntegerTy() && I.getDestTy()->isIntegerTy(1)) {
		// we cast an integer to boolean
		SMT_expr operand0 = getValueExpr(I.getOperand(0), false);
		SMT_expr zero = man->SMT_mk_num(0);
		SMT_expr cmp = man->SMT_mk_eq(operand0,zero);

		SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
		rho_components.push_back(man->SMT_mk_eq(expr,cmp));
	}
#ifdef NAIVE_TRUNC
	else {
		const SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));
		const SMT_expr assign = getValueExpr(I.getOperand(0), false);
		rho_components.push_back(man->SMT_mk_eq(expr,assign));
	}
#endif
}

void SMTpass::visitZExtInst (ZExtInst &I) {
	SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
	SMT_expr operand0 = getValueExpr(I.getOperand(0), false);
	if(I.getSrcTy()->isIntegerTy(1) && I.getDestTy()->isIntegerTy()) {
		// we cast a boolean to an integer
		SMT_expr zero = man->SMT_mk_num(0);
		SMT_expr one = man->SMT_mk_num(1);
		SMT_expr ite = man->SMT_mk_ite(operand0,one,zero);
		rho_components.push_back(man->SMT_mk_eq(expr,ite));
	} else if(I.getSrcTy()->isIntegerTy() && I.getDestTy()->isIntegerTy()) {
		// we cast an integer to an integer of different size
		// we assume the two integers are equal
		// TODO: not sound
		rho_components.push_back(man->SMT_mk_eq(operand0,expr));
	}
}

void SMTpass::visitSExtInst (SExtInst &I) {
#ifdef NAIVE_TRUNC
	const SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));
	const SMT_expr assign = getValueExpr(I.getOperand(0), false);
	rho_components.push_back(man->SMT_mk_eq(expr,assign));
#endif
}

void SMTpass::visitFPTruncInst (FPTruncInst &I) {
#ifdef NAIVE_TRUNC
	const SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));
	const SMT_expr assign = getValueExpr(I.getOperand(0), false);
	rho_components.push_back(man->SMT_mk_eq(expr,assign));
#endif
}

void SMTpass::visitFPExtInst (FPExtInst &I) {
#ifdef NAIVE_TRUNC
	const SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));
	const SMT_expr assign = getValueExpr(I.getOperand(0), false);
	rho_components.push_back(man->SMT_mk_eq(expr,assign));
#endif
}

void SMTpass::visitFPToUIInst (FPToUIInst &I) {
	SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
	SMT_expr operand0 = getValueExpr(I.getOperand(0), false);
	SMT_expr assign = man->SMT_mk_real2int(operand0);
	rho_components.push_back(man->SMT_mk_eq(expr,assign));
}

void SMTpass::visitFPToSIInst (FPToSIInst &I) {
	SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
	SMT_expr operand0 = getValueExpr(I.getOperand(0), false);
	SMT_expr assign = man->SMT_mk_real2int(operand0);
	rho_components.push_back(man->SMT_mk_eq(expr,assign));
}

void SMTpass::visitUIToFPInst (UIToFPInst &I) {
	SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
	SMT_expr operand0 = getValueExpr(I.getOperand(0), false);
	SMT_expr assign = man->SMT_mk_int2real(operand0);
	rho_components.push_back(man->SMT_mk_eq(expr,assign));
}

void SMTpass::visitSIToFPInst (SIToFPInst &I) {
	SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
	SMT_expr operand0 = getValueExpr(I.getOperand(0), false);
	SMT_expr assign = man->SMT_mk_int2real(operand0);
	rho_components.push_back(man->SMT_mk_eq(expr,assign));
}

void SMTpass::visitPtrToIntInst (PtrToIntInst &I) {
	if (pointer_arithmetic()) {
		const SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));
		const SMT_expr assign = getValueExpr(I.getOperand(0), false);
		rho_components.push_back(man->SMT_mk_eq(expr,assign));
	}
}

void SMTpass::visitIntToPtrInst (IntToPtrInst &I) {
	if (pointer_arithmetic()) {
		const SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));
		const SMT_expr assign = getValueExpr(I.getOperand(0), false);
		rho_components.push_back(man->SMT_mk_eq(expr,assign));
	}
}

void SMTpass::visitBitCastInst (BitCastInst &I) {
}

void SMTpass::visitSelectInst (SelectInst &I) {
	// if not interesting type, skip
	ap_texpr_rtype_t ap_type;
	if (Expr::get_ap_type((Value*)&I, ap_type) == 1) return;

	SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
	SMT_expr TrueValue = getValueExpr(I.getTrueValue(), false);
	SMT_expr FalseValue = getValueExpr(I.getFalseValue(), false);
	SMT_expr cond = getValueExpr(I.getCondition(), false);
	rho_components.push_back(man->SMT_mk_eq(
				expr,
				man->SMT_mk_ite(cond,TrueValue,FalseValue)
				));
}

void SMTpass::visitCallInst(CallInst &I) {
}

void SMTpass::visitVAArgInst (VAArgInst &I) {
}

void SMTpass::visitExtractElementInst (ExtractElementInst &I) {
}

void SMTpass::visitInsertElementInst (InsertElementInst &I) {
}

void SMTpass::visitShuffleVectorInst (ShuffleVectorInst &I) {
}

void SMTpass::visitExtractValueInst (ExtractValueInst &I) {
}

void SMTpass::visitInsertValueInst (InsertValueInst &I) {
}

void SMTpass::visitTerminatorInst (TerminatorInst &I) {
}

void SMTpass::visitBinaryOperator (BinaryOperator &I) {
	bool skip = false;
	ap_texpr_rtype_t ap_type;
	int t = Expr::get_ap_type((Value*)&I, ap_type);

	//primed[I.getParent()].insert(&I);
	//exist_prime.insert(&I);
	SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
	SMT_expr assign;	
	SMT_expr operand0 = getValueExpr(I.getOperand(0), false);
	SMT_expr operand1 = getValueExpr(I.getOperand(1), false);

	// if an operand is nan, return...
	if (operand0.is_empty()) return;
	if (operand1.is_empty()) return;

	switch(I.getOpcode()) {
		// Standard binary operators...
		case Instruction::Add : 
		case Instruction::FAdd: 
			assign = man->SMT_mk_sum(operand0,operand1);
			break;
		case Instruction::Sub : 
		case Instruction::FSub: 
			assign = man->SMT_mk_sub(operand0,operand1);
			break;
		case Instruction::Mul : 
		case Instruction::FMul: 
			if (!skipNonLinear())
				assign = man->SMT_mk_mul(operand0,operand1);
			break;
		case Instruction::And :
			//if (!t) return;
			if (t != 2) return;
			{
				std::vector<SMT_expr> operands;
				operands.push_back(operand0);
				operands.push_back(operand1);
				assign = man->SMT_mk_and(operands);
			}
			break;
		case Instruction::Or  :
			//if (!t) return;
			if (t != 2) return;
			{
				std::vector<SMT_expr> operands;
				operands.push_back(operand0);
				operands.push_back(operand1);
				assign = man->SMT_mk_or(operands);
			}
			break;
		case Instruction::Xor :
			//if (!t) return;
			if (t != 2) return;
			assign = man->SMT_mk_xor(operand0,operand1);
			break;
		case Instruction::UDiv: 
		case Instruction::SDiv: 
			if (!skipNonLinear())
				assign = man->SMT_mk_div(operand0,operand1);
			break;
		case Instruction::FDiv: 
			if (!skipNonLinear())
				assign = man->SMT_mk_div(operand0,operand1,false);
			break;
		case Instruction::URem: 
		case Instruction::SRem: 
		case Instruction::FRem: 
			//skip = true;
			if (!skipNonLinear())
				assign = man->SMT_mk_rem(operand0,operand1);
			break;
			// the others are not implemented
		case Instruction::Shl : // Shift left  (logical)
		case Instruction::LShr: // Shift right (logical)
		case Instruction::AShr: // Shift right (arithmetic)
		case Instruction::BinaryOpsEnd:
			// NOT IMPLEMENTED
			return;
	}
	if (!assign.is_empty())
		rho_components.push_back(man->SMT_mk_eq(expr,assign));
}

void SMTpass::visitCmpInst (CmpInst &I) {
	SMT_expr expr = getValueExpr(&I, is_primed(I.getParent(),I));	
	SMT_expr assign;	

	ap_texpr_rtype_t ap_type;
	if (Expr::get_ap_type((Value*)I.getOperand(0), ap_type)) {
		// the comparison is not between integers or reals
		// we create an undeterministic choice variable
		SMT_var cvar = man->SMT_mk_bool_var(getUndeterministicChoiceName(&I));
		assign = man->SMT_mk_expr_from_bool_var(cvar);
		rho_components.push_back(man->SMT_mk_eq(expr,assign));
		return;
	}

	SMT_expr op1, op2;

	op1 = getValueExpr(I.getOperand(0), false);
	op2 = getValueExpr(I.getOperand(1), false);

	if (op1.is_empty() || op2.is_empty()) {
		// the comparison involve at least an operand that is nan
		SMT_var cvar = man->SMT_mk_bool_var(getUndeterministicChoiceName(&I));
		assign = man->SMT_mk_expr_from_bool_var(cvar);
		rho_components.push_back(man->SMT_mk_eq(expr,assign));
		return;
	}

	switch (I.getPredicate()) {
		case CmpInst::FCMP_FALSE:
			assign = man->SMT_mk_false();
			break;
		case CmpInst::FCMP_TRUE: 
			assign = man->SMT_mk_true();
			break;
		case CmpInst::FCMP_OEQ: 
		case CmpInst::FCMP_UEQ: 
		case CmpInst::ICMP_EQ:
			assign = man->SMT_mk_eq(op1,op2);
			break;
		case CmpInst::FCMP_OGT:
		case CmpInst::FCMP_UGT:
		case CmpInst::ICMP_UGT: 
		case CmpInst::ICMP_SGT: 
			assign = man->SMT_mk_gt(op1,op2);
			break;
		case CmpInst::FCMP_OLT: 
		case CmpInst::FCMP_ULT: 
		case CmpInst::ICMP_ULT: 
		case CmpInst::ICMP_SLT: 
			assign = man->SMT_mk_lt(op1,op2);
			break;
		case CmpInst::FCMP_OGE:
		case CmpInst::FCMP_UGE:
		case CmpInst::ICMP_UGE:
		case CmpInst::ICMP_SGE:
			assign = man->SMT_mk_ge(op1,op2);
			break;
		case CmpInst::FCMP_OLE:
		case CmpInst::FCMP_ULE:
		case CmpInst::ICMP_ULE:
		case CmpInst::ICMP_SLE:
			assign = man->SMT_mk_le(op1,op2);
			break;
		case CmpInst::FCMP_ONE:
		case CmpInst::FCMP_UNE: 
		case CmpInst::ICMP_NE: 
			assign = man->SMT_mk_diseq(op1,op2);
			break;
		case CmpInst::FCMP_ORD: 
		case CmpInst::FCMP_UNO:
		case CmpInst::BAD_ICMP_PREDICATE:
		case CmpInst::BAD_FCMP_PREDICATE:
			{
				SMT_var cvar = man->SMT_mk_bool_var(getUndeterministicChoiceName(&I));
				assign = man->SMT_mk_expr_from_bool_var(cvar);
			}
			break;
	}
	rho_components.push_back(man->SMT_mk_eq(expr,assign));
}

void SMTpass::visitCastInst (CastInst &I) {
}
