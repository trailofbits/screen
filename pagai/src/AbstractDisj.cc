/**
 * \file AbstractDisj.cc
 * \brief Implementation of the AbstractDisj class
 * \author Julien Henry
 */
#include "stdio.h"

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "AbstractDisj.h"
#include "Node.h"
#include "Expr.h"
#include "Analyzer.h"

AbstractDisj::AbstractDisj(ap_manager_t* _man, Environment * env) {
	man_disj = new AbstractManClassic();
	disj.push_back(man_disj->NewAbstract(_man,env));
	main = disj[0]->main;
	pilot = NULL;
	man = _man;
}

AbstractDisj::AbstractDisj(ap_manager_t* _man, Environment * env, int max_index) {
	man_disj = new AbstractManClassic();
	for (int i = 0; i <= max_index; i++)
		disj.push_back(man_disj->NewAbstract(_man,env));
	main = disj[0]->main;
	pilot = NULL;
	man = _man;
}

int AbstractDisj::AddDisjunct(Environment * env) {
	disj.push_back(man_disj->NewAbstract(man,env));
	return disj.size()-1;
}

Abstract * AbstractDisj::getDisjunct(int index) {
	SetNDisjunct(index);
	return disj[index];
}

void AbstractDisj::setDisjunct(int index, Abstract * A) {
	SetNDisjunct(index);
	disj[index] = A;
	main = disj[0]->main;
}

int AbstractDisj::getMaxIndex() {
	return disj.size()-1;
}

void AbstractDisj::SetNDisjunct(int N) {
	if (N <= disj.size()-1) return;
	Environment env;
	while (N > disj.size()-1) {
		disj.push_back(man_disj->NewAbstract(man,&env));
	}
	main = disj[0]->main;
}

AbstractDisj::AbstractDisj(Abstract* A) {
	man_disj = new AbstractManClassic();
	man = A->man;
	disj.clear();
	if (AbstractDisj * A_dis = dynamic_cast<AbstractDisj*>(A)) {
		std::vector<Abstract*>::iterator it = A_dis->disj.begin(), et = A_dis->disj.end();
		for (; it != et; it++) {
			disj.push_back(man_disj->NewAbstract(*it));
		}
		main = disj[0]->main;
	} else {
		*Out << "ERROR when trying to create a disjunctive invariant\n";
		// ERROR
		main = NULL;
	}
	pilot = NULL;
}

void AbstractDisj::clear_all() {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		delete *it;
	}
	disj.clear();
	main = NULL;
}

AbstractDisj::~AbstractDisj() {
	delete man_disj;
	clear_all();
}

void AbstractDisj::set_top(Environment * env) {
	set_top(env,0);
}

void AbstractDisj::set_top(Environment * env, int index) {
	SetNDisjunct(index);
	int i = 0;
	// every disjunct is at bottom except the one of index 'index'
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++, i++) {
		if (i == index)
			(*it)->set_top(env);
		else
			(*it)->set_bottom(env);
	}
	main = disj[0]->main;
}

void AbstractDisj::set_bottom(Environment * env) {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->set_bottom(env);
	}
	main = disj[0]->main;
}

void AbstractDisj::set_bottom(Environment * env, int index) {
	SetNDisjunct(index);
	disj[index]->set_bottom(env);
	main = disj[0]->main;
}


void AbstractDisj::change_environment(Environment * env) {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->change_environment(env);
	}
	main = disj[0]->main;
}

void AbstractDisj::change_environment(Environment * env, int index) {
	SetNDisjunct(index);
	disj[index]->change_environment(env);
	if (index == 0)
		main = disj[0]->main;
}

bool AbstractDisj::is_leq_index (Abstract *d, int index) {
	SetNDisjunct(index);
	return disj[index]->is_leq(d);
}

bool AbstractDisj::is_eq_index (Abstract *d, int index) {
	SetNDisjunct(index);
	return disj[index]->is_eq(d);
}

bool AbstractDisj::is_bottom() {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		if (!(*it)->is_bottom()) return false;
	}
	return true;
}

bool AbstractDisj::is_top() {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		if ((*it)->is_top()) return true;
	}
	return false;
}

bool AbstractDisj::is_bottom(int index) {
	SetNDisjunct(index);
	return disj[index]->is_bottom();
}

//NOT IMPLEMENTED
void AbstractDisj::widening(Abstract * X) {
}

void AbstractDisj::widening(Abstract * X, int index) {
	SetNDisjunct(index);
	disj[index]->widening(X);
	main = disj[0]->main;
}

//NOT IMPLEMENTED
void AbstractDisj::widening_threshold(Abstract * X, Constraint_array* cons) {
}

void AbstractDisj::widening_threshold(Abstract * X, Constraint_array* cons, int index) {
	SetNDisjunct(index);
	disj[index]->widening_threshold(X,cons);
	main = disj[0]->main;
}

void AbstractDisj::meet_tcons_array(Constraint_array* tcons) {

	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->meet_tcons_array(tcons);
	}
	main = disj[0]->main;
}

void AbstractDisj::meet_tcons_array(Constraint_array* tcons, int index) {
	SetNDisjunct(index);
	disj[index]->meet_tcons_array(tcons);
	main = disj[0]->main;
}

void AbstractDisj::canonicalize() {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->canonicalize();
	}
}

void AbstractDisj::assign_texpr_array(
		ap_var_t* tvar, 
		ap_texpr1_t* texpr, 
		size_t size, 
		ap_abstract1_t* dest
		) {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->assign_texpr_array(tvar,texpr,size,dest);
	}
	main = disj[0]->main;
}

void AbstractDisj::assign_texpr_array(
		ap_var_t* tvar, 
		ap_texpr1_t* texpr, 
		size_t size, 
		ap_abstract1_t* dest,
		int index
		) {
	SetNDisjunct(index);
	disj[index]->assign_texpr_array(tvar,texpr,size,dest);
	main = disj[0]->main;
}

//NOT IMPLEMENTED
void AbstractDisj::join_array(Environment * env, std::vector<Abstract*> X_pred) {
}

void AbstractDisj::join_array(Environment * env, std::vector<Abstract*> X_pred, int index) {
	SetNDisjunct(index);
	disj[index]->join_array(env,X_pred);
	main = disj[0]->main;
}

//NOT IMPLEMENTED
void AbstractDisj::join_array_dpUcm(Environment *env, Abstract* n) {
}

void AbstractDisj::join_array_dpUcm(Environment *env, Abstract* n, int index) {
	SetNDisjunct(index);
	disj[index]->join_array_dpUcm(env,n);
	main = disj[0]->main;
}

void AbstractDisj::meet(Abstract* A) {
	//TODO
}

//NOT CORRECT
ap_tcons1_array_t AbstractDisj::to_tcons_array() {
	return ap_abstract1_to_tcons_array(man,main);
}

ap_tcons1_array_t AbstractDisj::to_tcons_array(int index) {
	SetNDisjunct(index);
	return disj[index]->to_tcons_array();
}

//NOT CORRECT
ap_lincons1_array_t AbstractDisj::to_lincons_array() {
	return ap_abstract1_to_lincons_array(man,main);
}

ap_lincons1_array_t AbstractDisj::to_lincons_array(int index) {
	SetNDisjunct(index);
	return disj[index]->to_lincons_array();
}

void AbstractDisj::print(bool only_main) {
	*Out << *this;
}

void AbstractDisj::display(llvm::raw_ostream &stream, std::string * left) const {
	int k = 0;
	std::vector<Abstract*>::const_iterator it = disj.begin(), et = disj.end();
	if (disj.size() == 1)
			disj[0]->display(stream,left);
	else {
		for (; it != et; it++, k++) {
			if (left != NULL) stream << *left;
			stream << "Disjunct " << k << "\n";
			(*it)->display(stream,left);
		}
	}
}
