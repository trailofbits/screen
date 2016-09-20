/**
 * \file Environment.cc
 * \brief Implementation of the Environment class
 * \author Julien Henry
 */

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"
#include "apron.h"
#include "Environment.h"
#include "Debug.h"

using namespace llvm;

Environment::Environment() {
	env = ap_environment_alloc_empty();
}
 
Environment::Environment(const Environment &e) {
	env = ap_environment_copy(e.env);
}

void Environment::init(std::set<ap_var_t> * intvars, std::set<ap_var_t> * realvars) {
	ap_var_t * _intvars = (ap_var_t*)malloc(intvars->size()*sizeof(ap_var_t));
	ap_var_t * _realvars = (ap_var_t*)malloc(realvars->size()*sizeof(ap_var_t));

	int j = 0;
	for (std::set<ap_var_t>::iterator i = intvars->begin(),
			e = intvars->end(); i != e; ++i) {
		_intvars[j] = *i;
		j++;
	}

	j = 0;
	for (std::set<ap_var_t>::iterator i = realvars->begin(),
			e = realvars->end(); i != e; ++i) {
		_realvars[j] = *i;
		j++;
	}

	env = ap_environment_alloc(_intvars,intvars->size(),_realvars,realvars->size());
	free(_intvars);
	free(_realvars);
}

Environment::Environment(std::set<ap_var_t> * intvars, std::set<ap_var_t> * realvars) {
	init(intvars,realvars);
}

Environment::Environment(Abstract * A) {
	env = ap_environment_copy(A->main->env);
}

Environment::Environment(ap_tcons1_array_t * cons) {
	env = ap_environment_copy(cons->env);
}

Environment::Environment(Constraint * cons) {
	env = ap_environment_copy(cons->get_ap_tcons1()->env);
}

Environment::Environment(Constraint_array * cons) {
	env = ap_environment_copy(cons->getEnv());
}

Environment::Environment(ap_environment_t * e) {
	env = ap_environment_copy(e);
}
		
Environment::Environment(Node * n, Live * LV) {
	std::set<ap_var_t> Sintvars;
	std::set<ap_var_t> Srealvars;

	for (std::map<Value*,std::set<ap_var_t> >::iterator i = n->intVar.begin(),
			e = n->intVar.end(); i != e; ++i) {
		if (LV->isLiveByLinearityInBlock((*i).first,n->bb,true)
			|| isa<UndefValue>((*i).first)) {
			Sintvars.insert((*i).second.begin(), (*i).second.end());
		}
	}
	for (std::map<Value*,std::set<ap_var_t> >::iterator i = n->realVar.begin(),
			e = n->realVar.end(); i != e; ++i) {
		if (LV->isLiveByLinearityInBlock((*i).first,n->bb,true)
			|| isa<UndefValue>((*i).first))
			Srealvars.insert((*i).second.begin(), (*i).second.end());
	}
	init(&Sintvars,&Srealvars);
}

Environment::~Environment() {
	ap_environment_free(env);
}

Environment & Environment::operator= (const Environment &e) {
	ap_environment_t * ee = ap_environment_copy(e.env);
	ap_environment_free(env);
	env = ee;
	return *this;
}
		
bool Environment::operator == (const Environment &e) {
	return ap_environment_is_eq(env, e.env);
}

bool Environment::operator != (const Environment &e) {
	return !ap_environment_is_eq(env, e.env);
}

bool Environment::operator <= (const Environment &e) {
	// ap_environment_is_leq is buggy when comparing 2 uncomparable environments
	// APRON has been patched so that ap_environment_is_leq behaves as expected
	return ap_environment_is_leq(env, e.env);

	// unreachable
	int n = ap_environment_compare(env,e.env);
	return (n == -1 || n == 0);
}

ap_environment_t * Environment::getEnv() {
	return env;
}

void Environment::get_vars(std::set<ap_var_t> * intdims, std::set<ap_var_t> * realdims) {
	ap_var_t var;
	Value* val;
	for (size_t i = 0; i < env->intdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		intdims->insert(var);
	}
	for (size_t i = env->intdim; i < env->intdim + env->realdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		realdims->insert(var);
	}
}

void Environment::get_vars_live_in(BasicBlock * b, Live * LV, std::set<ap_var_t> * intdims, std::set<ap_var_t> * realdims) {
	ap_var_t var;
	Value* val;
	for (size_t i = 0; i < env->intdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		// we consider undef values as never live
		if (Expr::is_undef_ap_var(var)) continue;
		val = (Value*)var;
		if (LV->isLiveByLinearityInBlock(val,b,true)) {
			intdims->insert(var);
		}
	}
	for (size_t i = env->intdim; i < env->intdim + env->realdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		if (Expr::is_undef_ap_var(var)) continue;
		val = (Value*)var;
		if (LV->isLiveByLinearityInBlock(val,b,true)) {
			realdims->insert(var);
		}
	}
}

ap_environment_t * Environment::common_environment(
		ap_environment_t * env1, 
		ap_environment_t * env2) {

	ap_dimchange_t * dimchange1 = NULL;
	ap_dimchange_t * dimchange2 = NULL;
	ap_environment_t * lcenv = ap_environment_lce(
			env1,
			env2,
			&dimchange1,
			&dimchange2);

	if (dimchange1 != NULL)
		ap_dimchange_free(dimchange1);
	if (dimchange2 != NULL)
		ap_dimchange_free(dimchange2);

	return lcenv;
}

void Environment::common_environment(ap_texpr1_t * exp1, ap_texpr1_t * exp2) {
	ap_environment_t * env1 = exp1->env;
	ap_environment_t * env2 = exp2->env;
	ap_environment_t * common = common_environment(env1,env2);
	ap_texpr1_extend_environment_with(exp1,common);
	ap_texpr1_extend_environment_with(exp2,common);
	ap_environment_free(common);
}

Environment Environment::common_environment(Expr* exp1, Expr* exp2) {
	ap_environment_t * env1 = exp1->getExpr()->env;
	ap_environment_t * env2 = exp2->getExpr()->env;
	if (ap_environment_is_eq(env1,env2))
			return Environment(env1);
	{
		ap_environment_t * common = common_environment(env1,env2);
		Environment res(common);
		ap_environment_free(common);
		return res;
	}
}

Environment Environment::common_environment(Environment* env1, Environment* env2) {
	if (ap_environment_is_eq(env1->getEnv(),env2->env))
		return Environment(env1->getEnv());
	ap_environment_t * common = common_environment(env1->env,env2->env);
	Environment res(common);
	ap_environment_free(common);
	return res;
}

Environment Environment::intersection(Environment * env1, Environment * env2) {
	ap_environment_t * lcenv = common_environment(env1->env,env2->env);
	ap_environment_t * intersect = ap_environment_copy(lcenv);	
	ap_environment_t * tmp = NULL;	

	for (size_t i = 0; i < lcenv->intdim + lcenv->realdim; i++) {
		ap_var_t var = ap_environment_var_of_dim(lcenv,(ap_dim_t)i);
		if (!ap_environment_mem_var(env1->env,var) || !ap_environment_mem_var(env2->env,var)) {
			//size_t size = intersect->intdim + intersect->realdim;
			tmp = ap_environment_remove(intersect,&var,1);
			ap_environment_free(intersect);
			intersect = tmp;
		}	
	}	
	Environment res(intersect);
	ap_environment_free(intersect);
	ap_environment_free(lcenv);
	return res;
}

void Environment::display(llvm::raw_ostream &stream) const {
	stream << "Environment : count=" << (unsigned long)env->count << "\n";
	for (size_t i=0; i<env->intdim+env->realdim; i++) {
		char* c = ap_var_operations->to_string(env->var_of_dim[i]);
		stream << i << ": " << c << (i<env->intdim ? " (int)" : " (real)") << "\n";
		free(c);
	}
}
		
void Environment::to_MDNode(LLVMContext * C, std::vector<llvm::Metadata*> * met) {
	for (size_t i=0; i<env->intdim+env->realdim; i++) {
		ap_var_t var = env->var_of_dim[i];
		Value * val = (Value*) var;
		MDString * dim = MDString::get(*C, ap_var_to_string(var));
		MDNode* N = MDNode::get(*C,dim);
		met->push_back(N);
	}
}

llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, const Environment& env) {
	env.display(stream);
	return stream;
}
