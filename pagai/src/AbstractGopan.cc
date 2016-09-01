/**
 * \file AbstractGopan.cc
 * \brief Implementation of the AbstractGopan class
 * \author Julien Henry
 */
#include "stdio.h"

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "AbstractGopan.h"
#include "Node.h"
#include "Expr.h"
#include "Analyzer.h"

AbstractGopan::AbstractGopan(ap_manager_t* _man, Environment * env) {
	main = new ap_abstract1_t(ap_abstract1_bottom(_man,env->getEnv()));
	pilot = main;
	man = _man;
}


AbstractGopan::AbstractGopan(Abstract* A) {
	man = A->man;
	main = new ap_abstract1_t(ap_abstract1_copy(man,A->main));
	if (A->pilot == NULL || A->pilot == A->main)
		pilot = main;
	else
		pilot = new ap_abstract1_t(ap_abstract1_copy(man,A->pilot));
}

void AbstractGopan::clear_all() {
	if (pilot != main) {
		ap_abstract1_clear(man,pilot);
		delete pilot;
	}
	ap_abstract1_clear(man,main);
	delete main;
}

AbstractGopan::~AbstractGopan() {
	clear_all();
}

void AbstractGopan::set_top(Environment * env) {
	if (pilot != main) {
		ap_abstract1_clear(man,pilot);
		delete pilot;
	}
	ap_abstract1_clear(man,main);
	*main = ap_abstract1_top(man,env->getEnv());
	pilot = main;
}

void AbstractGopan::set_bottom(Environment * env) {
	if (pilot != main) {
		ap_abstract1_clear(man,pilot);
		delete pilot;
	}
	ap_abstract1_clear(man,main);
	*main = ap_abstract1_bottom(man,env->getEnv());
	pilot = main;
}

void AbstractGopan::change_environment(Environment * env) {

	if (!ap_environment_is_eq(env->getEnv(),main->env))
		*main = ap_abstract1_change_environment(man,true,main,env->getEnv(),false);
	if (pilot != main && !ap_environment_is_eq(env->getEnv(),pilot->env))
		*pilot = ap_abstract1_change_environment(man,true,pilot,env->getEnv(),false);
}

bool AbstractGopan::is_bottom() {
	return ap_abstract1_is_bottom(man,main);
}

bool AbstractGopan::is_top() {
	return ap_abstract1_is_top(man,main);
}

void AbstractGopan::widening(Abstract * X) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xpilot_widening;

	if (is_leq(X)) {
		Xmain_widening = ap_abstract1_copy(man,X->main);
		Xpilot_widening = ap_abstract1_copy(man,X->pilot);
	} else {
		ap_abstract1_t dpUcm = ap_abstract1_join(man,false,pilot,X->main);
		if (ap_abstract1_is_leq(man,&dpUcm,X->pilot)) {
			Xmain_widening = ap_abstract1_copy(man,&dpUcm);
			Xpilot_widening = ap_abstract1_copy(man,&dpUcm);
		} else {
			Xmain_widening = ap_abstract1_join(man,false,main,X->main);
			Xpilot_widening = ap_abstract1_widening(man,X->pilot,&dpUcm);
		}
		ap_abstract1_clear(man,&dpUcm);
	}	
	
	if (pilot != main) {
		ap_abstract1_clear(man,pilot);
		delete pilot;
	}
	ap_abstract1_clear(man,main);
	
	*main = Xmain_widening;
	if (ap_abstract1_is_eq(man,&Xmain_widening,&Xpilot_widening)) {
		pilot = main;
		ap_abstract1_clear(man,&Xpilot_widening);
	} else {
		pilot = new ap_abstract1_t(Xpilot_widening);
	}
}

/*
 * widening with threshold is not implemented. We do a classical widening
 * instead
 */
void AbstractGopan::widening_threshold(Abstract * X, Constraint_array* cons) {
	widening(X);
}

ap_tcons1_array_t AbstractGopan::to_tcons_array() {
	return ap_abstract1_to_tcons_array(man,main);
}

ap_lincons1_array_t AbstractGopan::to_lincons_array() {
	return ap_abstract1_to_lincons_array(man,main);
}

void AbstractGopan::meet_tcons_array(Constraint_array* tcons) {

	Environment main_env(this);
	Environment cons_env(tcons);
	Environment lcenv = Environment::common_environment(&main_env,&cons_env);

	if (pilot != main) {
		*pilot = ap_abstract1_change_environment(man,true,pilot,lcenv.getEnv(),false);
		*pilot = ap_abstract1_meet_tcons_array(man,true,pilot,tcons->to_tcons1_array());
	}

	*main = ap_abstract1_change_environment(man,true,main,lcenv.getEnv(),false);
	*main = ap_abstract1_meet_tcons_array(man,true,main,tcons->to_tcons1_array());

}

void AbstractGopan::canonicalize() {
	ap_abstract1_canonicalize(man,main);
	if (pilot != main)
		ap_abstract1_canonicalize(man,pilot);
}

void AbstractGopan::assign_texpr_array(
		ap_var_t* tvar, 
		ap_texpr1_t* texpr, 
		size_t size, 
		ap_abstract1_t* dest
		) {
	if (pilot != main)
		*pilot = ap_abstract1_assign_texpr_array(man,true,pilot,
				tvar,
				texpr,
				size,
				dest);

	*main = ap_abstract1_assign_texpr_array(man,true,main,
			tvar,
			texpr,
			size,
			dest);
}

void AbstractGopan::join_array(Environment * env, std::vector<Abstract*> X_pred) {
	size_t size = X_pred.size();

	ap_abstract1_t  Xmain[size];
	ap_abstract1_t  Xpilot[size];
	
	for (unsigned i=0; i < size; i++) {
		Xmain[i] = ap_abstract1_change_environment(man,false,X_pred[i]->main,env->getEnv(),false);
		Xpilot[i] = ap_abstract1_change_environment(man,false,X_pred[i]->pilot,env->getEnv(),false);
		delete X_pred[i];
	}
	
	ap_abstract1_clear(man,main);
	if (pilot != main) {
		ap_abstract1_clear(man,pilot);
		delete pilot;
	}
	if (size > 1) {
		*main = ap_abstract1_join_array(man,Xmain,size);	
		pilot = new ap_abstract1_t(ap_abstract1_join_array(man,Xpilot,size));	
		for (unsigned i=0; i < size; i++) {
			ap_abstract1_clear(man,&Xmain[i]);
			ap_abstract1_clear(man,&Xpilot[i]);
		}
	} else {
		*main = Xmain[0];
		pilot = new ap_abstract1_t(Xpilot[0]);
		ap_abstract1_clear(man,&Xpilot[0]);
	}
	
	if (ap_abstract1_is_eq(man,main,pilot)) {
		ap_abstract1_clear(man,pilot);
		delete pilot;
		pilot = main;
	}
}

void AbstractGopan::join_array_dpUcm(Environment *env, Abstract* n) {
	ap_abstract1_t Xmain;
	ap_abstract1_t Xpilot;
	Xmain = ap_abstract1_join(man,false,main,n->main);
	Xpilot = ap_abstract1_join(man,false,pilot,n->main);
	delete n;
	
	if (pilot == main) {
		pilot = new ap_abstract1_t(Xpilot);
	} else {
		ap_abstract1_clear(man,pilot);
		*pilot = Xpilot;
	}
	ap_abstract1_clear(man,main);
	*main = Xmain;
}

void AbstractGopan::meet(Abstract* A) {
	//TODO
}

void AbstractGopan::print(bool only_main) {
	*Out << *this;
}

void AbstractGopan::display(llvm::raw_ostream &stream,  std::string * left) const {
	FILE* tmp = tmpfile();
	if (tmp == NULL) return;

	ap_environment_fdump(tmp,main->env);
	ap_abstract1_fprint(tmp,man,main);

	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		stream << c;
	fclose(tmp);
}
