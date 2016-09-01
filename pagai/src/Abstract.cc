/**
 * \file Abstract.cc
 * \brief Implementation of the Abstract class
 * \author Julien Henry
 */
#include "SMTpass.h"
#include "Abstract.h"
#include "AbstractGopan.h"
#include "AbstractClassic.h"
#include "Expr.h"

int Abstract::compare(Abstract * d) {
	bool f = false;
	bool g = false;

//if enabled, we may have errors because the two abstracts may not have the same
//environment
#if 1
	if (dynamic_cast<AbstractClassic*>(d) 
		&& dynamic_cast<AbstractClassic*>(this)) {
		if (ap_abstract1_is_eq(man,main,d->main))
			return 0;
		if (ap_abstract1_is_leq(man,main,d->main))
			return 1;
		if (ap_abstract1_is_leq(man,d->main,main))
			return -1;
		return -2;
	}
#endif

	SMTpass * LSMT = SMTpass::getInstanceForAbstract();

	LSMT->push_context();
	SMT_expr A_smt = LSMT->AbstractToSmt(NULL,this);
	SMT_expr B_smt = LSMT->AbstractToSmt(NULL,d);

	LSMT->push_context();
	// f = A and not B
	std::vector<SMT_expr> cunj;
	cunj.push_back(A_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(B_smt));
	SMT_expr test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		f = true;
	}
	LSMT->pop_context();

	// g = B and not A
	cunj.clear();
	cunj.push_back(B_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(A_smt));
	test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		g = true;
	}

	LSMT->pop_context();
	
	if (!f && !g) {
		// eq
		return 0;
	} else if (!f && g) {
		// lt
		return 1;
	} else if (f && !g) {
		// gt
		return -1;
	} else {
		// not comparable
		return -2;
	}
}

bool Abstract::has_same_environment(Abstract * A) {
	if (ap_environment_is_eq(main->env,A->main->env))
		return true;
	else
		return false;
}

bool Abstract::CanJoinPrecisely(AbstractMan * aman, Abstract * A) {
	//return false;
	bool res = true;
	SMTpass * LSMT = SMTpass::getInstance();
	std::vector<Abstract*> Join;
	Environment env_this(this);
	Environment env_A(A);
	Environment cenv(Environment::common_environment(&env_this,&env_A));
	Join.push_back(aman->NewAbstract(this));
	Join.push_back(aman->NewAbstract(A));
	Abstract * J = aman->NewAbstract(this);
	J->join_array(&cenv,Join);

	LSMT->push_context();
	SMT_expr A_smt = LSMT->AbstractToSmt(NULL,this);
	SMT_expr B_smt = LSMT->AbstractToSmt(NULL,A);
	SMT_expr J_smt = LSMT->AbstractToSmt(NULL,J);
	
	std::vector<SMT_expr> cunj;
	cunj.push_back(J_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(A_smt));
	cunj.push_back(LSMT->man->SMT_mk_not(B_smt));
	SMT_expr test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		res = false;
	}
	LSMT->pop_context();
	delete J;
	return res;
}

bool Abstract::is_leq(Abstract * d) {
	// in the case we compare two AbstractGopan, we have to slightly change the
	// comparison
	if (dynamic_cast<AbstractGopan*>(d) 
		&& dynamic_cast<AbstractGopan*>(this)) {
		if (ap_abstract1_is_eq(man,main,d->main)) {
			if (ap_abstract1_is_leq(man,pilot,d->pilot) || d->pilot == NULL) 
				return true; 
			else 
				return false;
		}
		return ap_abstract1_is_leq(man,main,d->main);
	}
	return (compare(d) >= 0);
}

bool Abstract::is_eq(Abstract * d) {
	return (compare(d) == 0);
}

void Abstract::assign_texpr_array(
		std::vector<ap_var_t> * name,
		std::vector<Expr*> * expr,
		ap_abstract1_t* dest) {
		
	std::vector<ap_texpr1_t> texpr;
	std::vector<Expr*>::iterator it = expr->begin(), et = expr->end();
	for (; it != et; it++) {
		//ap_texpr1_t * exp = ap_texpr1_copy((*it).getExpr());
		ap_texpr1_t * exp = (*it)->getExpr();
		texpr.push_back(*exp);
	}
	assign_texpr_array(&(*name)[0],&texpr[0],name->size(),dest);

	// FREE expr
	for (it = expr->begin(), et = expr->end(); it != et; it++) {
		delete *it;
	}
	name->clear();
	expr->clear();
}


llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, Abstract const& A) {
	A.display(stream);
    return stream;
}
