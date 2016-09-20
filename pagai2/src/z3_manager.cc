/**
 * \file z3_manager.cc
 * \brief Implementation of the z3_manager class
 * \author Julien Henry
 */
#if HAS_Z3
#include <algorithm>
#include <cstddef>
#include <string.h>
#include <sstream>
#include <iostream>

#include "llvm/Support/FormattedStream.h"

#include <gmp.h>
#include "z3_manager.h"
#include "Analyzer.h"
#include "Debug.h"

using namespace z3;

z3_manager::z3_manager() {
	ctx.set("MODEL", "true");
	if (getTimeout() != 0) {
		std::ostringstream timeout;
		timeout << getTimeout()*1000;
		ctx.set("SOFT_TIMEOUT", timeout.str().c_str());
	}

	int_type.z3.reset(new z3::sort(ctx.int_sort()));
	float_type.z3.reset(new z3::sort(ctx.real_sort()));
	bool_type.z3.reset(new z3::sort(ctx.bool_sort()));

	s = new solver(ctx);
}

z3_manager::~z3_manager() {
	// we have to clear the smartpointers from SMT_type before everything is
	// freed by the delete s
	int_type.z3_clear();
	float_type.z3_clear();
	bool_type.z3_clear();
	delete s;
}

SMT_expr z3_manager::SMT_mk_true(){
	return SMT_expr(expr(ctx.bool_val(true)));
}

SMT_expr z3_manager::SMT_mk_false(){
	return SMT_expr(expr(ctx.bool_val(false)));
}

SMT_var z3_manager::SMT_mk_bool_var(std::string name){
	if (!vars.count(name)) {
		char * cstr = new char [name.size()+1];
		strcpy (cstr, name.c_str());
		vars.insert(std::pair<std::string,SMT_var>(name,SMT_var(symbol(ctx,Z3_mk_string_symbol(ctx,cstr)))));
		delete [] cstr;
		types[vars[name]] = bool_type;
	}
	return vars[name];
}

SMT_var z3_manager::SMT_mk_var(std::string name, SMT_type type){
	if (!vars.count(name)) {
		char * cstr = new char [name.size()+1];
		strcpy (cstr, name.c_str());
		vars.insert(std::pair<std::string,SMT_var>(name,SMT_var(symbol(ctx,Z3_mk_string_symbol(ctx,cstr)))));
		delete [] cstr;
		types[vars[name]] = type;
	} 
	return vars[name];
}

SMT_expr z3_manager::SMT_mk_expr_from_bool_var(SMT_var var){
	return SMT_expr(expr(ctx.constant(*var.symb(),*bool_type.sort())));
}

SMT_expr z3_manager::SMT_mk_expr_from_var(SMT_var var){
	return SMT_expr(expr(ctx.constant(*var.symb(),*types[var].sort())));
}

SMT_expr z3_manager::SMT_mk_or (std::vector<SMT_expr> args){
	if (args.size() == 0) {
		return SMT_mk_true();
	}
	expr e(ctx);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	e = *(*B).expr();
	B++;
	for (; B != E; ++B) {
		e = e || *(*B).expr();
	}
	return SMT_expr(e);
}

SMT_expr z3_manager::SMT_mk_and (std::vector<SMT_expr> args){
	if (args.size() == 0) {
		return SMT_mk_true();
	}
	expr e(ctx);
	e = ctx.bool_val(true);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	e = *(*B).expr();
	B++;
	for (; B != E; ++B) {
		e = e && *(*B).expr();
	}
	return SMT_expr(e);
}

SMT_expr z3_manager::SMT_mk_eq (SMT_expr a1, SMT_expr a2){
	return SMT_expr(expr(*a1.expr() == *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_diseq (SMT_expr a1, SMT_expr a2){
	return SMT_expr(expr(*a1.expr() != *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e){
	expr ite  = to_expr(ctx, Z3_mk_ite(ctx, *c.expr(), *t.expr(), *e.expr()));
	return SMT_expr(expr(ite));
}

SMT_expr z3_manager::SMT_mk_not (SMT_expr a){
	return SMT_expr(expr( ! *a.expr()));
}

SMT_expr z3_manager::SMT_mk_num (int n){
	return SMT_expr(expr(ctx.int_val(n)));
}

SMT_expr z3_manager::SMT_mk_num_mpq (mpq_t mpq) {
	//char * x = mpq_get_str (NULL,10,mpq);
	//SMT_expr res(expr(ctx.int_val(x)));
	//free(x);

	SMT_expr res;
	char * cnum = mpz_get_str(NULL,10,mpq_numref(mpq));
	char * cden = mpz_get_str(NULL,10,mpq_denref(mpq));
	std::string snum(cnum);
	std::string sden(cden);
	if (sden.compare("1")) {
		SMT_expr numerator(expr(ctx.int_val(cnum)));
		SMT_expr denominator(expr(ctx.int_val(cden)));
		res = SMT_mk_div(numerator,denominator);
	} else {
		res = expr(ctx.int_val(cnum));
	}
	free(cnum);
	free(cden);
	return res;
}

SMT_expr z3_manager::SMT_mk_real (double x) {
	mpq_t val;
	mpq_init(val);
	mpq_set_d(val,x);

	double den = mpz_get_d(mpq_denref(val));
	char * cnum = mpz_get_str(NULL,10,mpq_numref(val));
	char * cden = mpz_get_str(NULL,10,mpq_denref(val));

	std::ostringstream oss;
	if (den == 1)
		oss << cnum;
	else
		oss << cnum << "/" << cden;
	std::string r = oss.str();
	free(cnum);
	free(cden);
	return SMT_expr(expr(ctx.real_val(r.c_str())));
}

SMT_expr z3_manager::SMT_mk_sum (std::vector<SMT_expr> args){
	expr e(ctx);
	e = ctx.int_val(0);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	for (; B != E; ++B) {
		e = e + *(*B).expr();
	}
	return SMT_expr(e);
}

SMT_expr z3_manager::SMT_mk_sub (std::vector<SMT_expr> args){
	expr e(ctx);
	e = ctx.int_val(0);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	for (; B != E; ++B) {
		e = e - *(*B).expr();
	}
	return SMT_expr(e);
}

SMT_expr z3_manager::SMT_mk_mul (std::vector<SMT_expr> args){
	expr e(ctx);
	e = ctx.int_val(1);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	for (; B != E; ++B) {
		e = e * *(*B).expr();
	}
	return SMT_expr(e);
}

SMT_expr z3_manager::SMT_mk_sum (SMT_expr a1, SMT_expr a2) {
	return SMT_expr(expr(*a1.expr() + *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_sub (SMT_expr a1, SMT_expr a2) {
	return SMT_expr(expr(*a1.expr() - *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_mul (SMT_expr a1, SMT_expr a2) {
	return SMT_expr(expr(*a1.expr() * *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_div (SMT_expr a1, SMT_expr a2, bool integer) {
	return SMT_expr(expr(*a1.expr() / *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_rem (SMT_expr a1, SMT_expr a2) {
	expr r  = to_expr(ctx, Z3_mk_rem(ctx, *a1.expr(), *a2.expr()));
	return SMT_expr(r);
}

SMT_expr z3_manager::SMT_mk_xor (SMT_expr a1, SMT_expr a2) {
	expr r = to_expr(ctx, Z3_mk_xor(ctx, *a1.expr(), *a2.expr()));
	return SMT_expr(r);
	// next line is bit-wise xor
	// return SMT_expr(expr(*a1.expr() ^ *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_lt (SMT_expr a1, SMT_expr a2){
	return SMT_expr(expr(*a1.expr() < *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_le (SMT_expr a1, SMT_expr a2){
	return SMT_expr(expr(*a1.expr() <= *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_gt (SMT_expr a1, SMT_expr a2){
	return SMT_expr(expr(*a1.expr() > *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_ge (SMT_expr a1, SMT_expr a2){
	return SMT_expr(expr(*a1.expr() >= *a2.expr()));
}

SMT_expr z3_manager::SMT_mk_int2real(SMT_expr a) {
	expr r  = to_real(*a.expr());
	return SMT_expr(expr(r));
}

SMT_expr z3_manager::SMT_mk_real2int(SMT_expr a) {
	expr r  = to_expr(ctx, Z3_mk_real2int(ctx, *a.expr()));
	return SMT_expr(expr(r));
}

SMT_expr z3_manager::SMT_mk_is_int(SMT_expr a) {
	assert(Z3_get_sort_kind(ctx, Z3_get_sort(ctx, *a.expr())) == Z3_REAL_SORT);
	expr r  = to_expr(ctx, Z3_mk_is_int(ctx, *a.expr()));
	return SMT_expr(expr(r));
}

SMT_expr z3_manager::SMT_mk_int0() {
	return SMT_mk_num(0);
}

SMT_expr z3_manager::SMT_mk_real0() {
	return SMT_mk_real(0.0);
}

void z3_manager::SMT_print(SMT_expr a){

	if (onlyOutputsRho()) {
		// we need this for Diego's WCET scripts
		//Z3_set_ast_print_mode(ctx,Z3_PRINT_SMTLIB_FULL);
		Z3_set_ast_print_mode(ctx,Z3_PRINT_SMTLIB2_COMPLIANT);
	}

	std::ostringstream oss;
	oss << "\"" << getFilename() << "\"";

	*Out << Z3_benchmark_to_smtlib_string(ctx,
			oss.str().c_str(),
			"unknown",
			"unknown",
			"",
			0,
			NULL,
			*a.expr());
}

void z3_manager::SMT_assert(SMT_expr a){
	s->add(*a.expr());
}

int z3_manager::SMT_check(SMT_expr a, std::set<std::string> * true_booleans){
	int ret = 0;
	SMT_assert(a);
	//check_result result = s->check(1,a.expr());
	check_result result;
	try {
		result = s->check();
	} catch (z3::exception e) {
		result = unknown;
	}
	switch (result) {
		case unsat:
			PDEBUG(
					*Out << "unsat\n";
			     );
			ret = 0;
			break;
		case unknown:
			PDEBUG(
					*Out << "unknown\n";
			     );
			*Out << "UNKNOWN\n";
			ret = -1;
			break;
		case sat:
			PDEBUG(
					*Out << "sat\n";
			     );
			ret = 1;
			model m = s->get_model();
			//DEBUG_SMT(
			//*Out << Z3_model_to_string(ctx,m);
			//);
			unsigned n = m.num_consts();
			std::ostringstream oss_true;
			std::ostringstream oss_false;
			std::ostringstream oss;
			for (unsigned i = 0; i < n; i++) {
				func_decl decl = m.get_const_decl(i);
				expr v = m.get_const_interp(decl);
				std::string name = decl.name().str();


				if (v.is_bool()) {
					switch (Z3_get_bool_value(ctx,v)) {
						case Z3_L_FALSE:
							DEBUG_SMT(
									oss_false << name << " := false\n";
								 );
							break;
						case Z3_L_UNDEF:
							DEBUG_SMT(
									oss_true << name << " := undef\n";
								 );
							break;
						case Z3_L_TRUE:
							DEBUG_SMT(
									oss_true << name << " := true\n";
								 );
							true_booleans->insert(name);
							break;
					}

				} else {
					DEBUG_SMT(
							oss << name << " := " << Z3_ast_to_string(ctx,v) << "\n";
						 );
				}
			}
			*Out << oss_true.str();
			*Out << oss.str();
			break;
	}
	return ret;
}

void z3_manager::push_context() {
	s->push();
}

void z3_manager::pop_context() {
	s->pop();
}

bool z3_manager::interrupt() {
	Z3_interrupt(ctx);
	return true;
}
#endif
