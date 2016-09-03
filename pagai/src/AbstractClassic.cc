/**
 * \file AbstractClassic.cc
 * \brief Implementation of the AbstractClassic class
 * \author Julien Henry
 */
#include "stdio.h"
#include <iostream>
#include <sstream>

#include "llvm/Support/FormattedStream.h"
#include "llvm/IR/IRBuilder.h"

#include "ap_global1.h"

#include "Abstract.h"
#include "AbstractClassic.h"
#include "Node.h"
#include "Expr.h"
#include "apron.h"
#include "apron_MD.h"
#include "Analyzer.h"
#include "Debug.h"

AbstractClassic::AbstractClassic(ap_manager_t* _man, Environment * env) {
	main = new ap_abstract1_t(ap_abstract1_bottom(_man,env->getEnv()));
	pilot = NULL;
	man = _man;
}


AbstractClassic::AbstractClassic(Abstract* A) {
	man = A->man;
	main = new ap_abstract1_t(ap_abstract1_copy(man,A->main));
	pilot = NULL;
}

void AbstractClassic::clear_all() {
	ap_abstract1_clear(man,main);
	delete main;
}

AbstractClassic::~AbstractClassic() {
	clear_all();
}

void AbstractClassic::set_top(Environment * env) {
	ap_abstract1_clear(man,main);
	*main = ap_abstract1_top(man,env->getEnv());
}

void AbstractClassic::set_bottom(Environment * env) {
	ap_abstract1_clear(man,main);
	*main = ap_abstract1_bottom(man,env->getEnv());
}

void AbstractClassic::change_environment(Environment * env) {
	if (!ap_environment_is_eq(env->getEnv(),main->env))
		*main = ap_abstract1_change_environment(man,true,main,env->getEnv(),false);
}

bool AbstractClassic::is_bottom() {
	return ap_abstract1_is_bottom(man,main);
}

bool AbstractClassic::is_top() {
	return ap_abstract1_is_top(man,main);
}

void AbstractClassic::widening(Abstract * X) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,X->main,main);
	Xmain_widening = ap_abstract1_widening(man,X->main,&Xmain);
	ap_abstract1_clear(man,&Xmain);

	ap_abstract1_clear(man,main);
	*main = Xmain_widening;
	canonicalize();
}

void AbstractClassic::widening_threshold(Abstract * X, Constraint_array* cons) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,X->main,main);
	Xmain_widening = ap_abstract1_widening_threshold(man,X->main,&Xmain, cons->to_lincons1_array());
	ap_abstract1_clear(man,&Xmain);

	ap_abstract1_clear(man,main);
	*main = Xmain_widening;
	canonicalize();
}

void AbstractClassic::meet_tcons_array(Constraint_array* tcons) {
	Environment main_env(this);
	Environment cons_env(tcons);

	if (!(cons_env <= main_env)) {
		// environment of the constraint is not included in main_env
		// we have to update the environment of the abstract value
		Environment lcenv(Environment::common_environment(&main_env,&cons_env));
		// the following commented code should be equivalent to the next three
		// lines, but there is a memory leak when using it
		//*main = ap_abstract1_change_environment(man,true,main,lcenv.getEnv(),false);
		ap_abstract1_t new_main = ap_abstract1_change_environment(man,false,main,lcenv.getEnv(),false);
	    ap_abstract1_clear(man,main);
	    *main = new_main;
	}
	*main = ap_abstract1_meet_tcons_array(man,true,main,tcons->to_tcons1_array());
	canonicalize();
}

void AbstractClassic::canonicalize() {
  /*DM: this code, from JH, was supposed to
    "delete from the abstract values the constraints with very large coefficients, to prevent some very high analysis time".
    Unfortunately it tends to create UNREACHABLE values where there should not be.
    Temporarily disabled until we find the reason.
  */

#ifdef SIMPLIFY_CONSTRAINTS
	// 
	//  iterate over the constraints forming the abstract value; 
	//  if a constraint is "too complicated", delete it
	//  
	//  "too complicated" == the constraints coefficients are huge
	//
	ap_tcons1_array_t tcons_array = ap_abstract1_to_tcons_array(man,main);
	size_t size = ap_tcons1_array_size (&tcons_array);
	for (int i = 0; i < size; i++) {
		ap_tcons1_t cons = ap_tcons1_array_get(&tcons_array,i);
		ap_texpr1_t expr = ap_tcons1_texpr1ref(&cons);
		if (has_huge_coeffs(expr.texpr0)) {
			PDEBUG(*Out << "Deleting a constraint with huge coefficients\n";);
			ap_scalar_t * one = ap_scalar_alloc();
			ap_scalar_set_int(one,1);
			ap_tcons1_t true_constraint = ap_tcons1_make(
					AP_CONS_EQ,
					ap_texpr1_cst_scalar(main->env,one),
					one
					);
			ap_tcons1_array_set(&tcons_array,i,&true_constraint);
		}
	}
	// TODO: memory leak ?
	*main = ap_abstract1_of_tcons_array(man,main->env,&tcons_array);
	ap_tcons1_array_clear(&tcons_array);
	
	// APRON canonicalize
#endif
	ap_abstract1_canonicalize(man,main);
}

void AbstractClassic::assign_texpr_array(
		ap_var_t* tvar, 
		ap_texpr1_t* texpr, 
		size_t size, 
		ap_abstract1_t* dest
		) {
	*main = ap_abstract1_assign_texpr_array(man,true,main,
			tvar,
			texpr,
			size,
			dest);
	canonicalize();
}

void AbstractClassic::join_array(Environment * env, std::vector<Abstract*> X_pred) {
	size_t size = X_pred.size();
	ap_abstract1_clear(man,main);

	ap_abstract1_t  Xmain[size];

	for (unsigned i=0; i < size; i++) {
		Xmain[i] = ap_abstract1_change_environment(man,false,X_pred[i]->main,env->getEnv(),false);
		delete X_pred[i];
	}

	if (size > 1) {
		*main = ap_abstract1_join_array(man,Xmain,size);	
		for (unsigned i=0; i < size; i++) {
			ap_abstract1_clear(man,&Xmain[i]);
		}
	} else {
		*main = Xmain[0];
	}
	canonicalize();
}

void AbstractClassic::join_array_dpUcm(Environment *env, Abstract* n) {
	std::vector<Abstract*> v;
	v.push_back(n);
	v.push_back(new AbstractClassic(this));
	join_array(env,v);
}

void AbstractClassic::meet(Abstract* A) {
	ap_abstract1_t tmp = *main;
	*main = ap_abstract1_meet(man,false,main,A->main);
	ap_abstract1_clear(man,&tmp);
}

ap_tcons1_array_t AbstractClassic::to_tcons_array() {
	return ap_abstract1_to_tcons_array(man,main);
}

ap_lincons1_array_t AbstractClassic::to_lincons_array() {
	return ap_abstract1_to_lincons_array(man,main);
}

void AbstractClassic::print(std::string &outString, bool only_main) {
	std::string s;
	llvm::raw_string_ostream buffer(s);
	// buffer << *this;
	outString = buffer.str();
	*Out << *this;
}

void AbstractClassic::display(llvm::raw_ostream &stream, std::string &ret_invariant, std::string * left) const {

#if 0
	std::string vertices = "[";
	std::string rays = "[";
	std::string lines = "[";
	std::string legend;
	bool first_vertice = true;
	bool first_ray = true;
	bool first_line = true;
	ap_generator1_array_t gen_array = ap_abstract1_to_generator_array(man,main);
	size_t size = ap_generator1_array_size(&gen_array);
	for (size_t k = 0; k < size; k++) {
		 ap_generator1_t gen = ap_generator1_array_get (&gen_array,k);
		 ap_gentyp_t* gentyp = ap_generator1_gentypref(&gen);
		 std::string * out;
		 if (*gentyp == AP_GEN_RAY) {
			 out = &rays; 
			 if (!first_ray) *out += ",";
			 first_ray = false;
		 } else if (*gentyp == AP_GEN_VERTEX) {
			 out = &vertices;
			 if (!first_vertice) *out += ",";
			 first_vertice = false;
		 } else if (*gentyp == AP_GEN_LINE) {
			 out = &lines;
			 if (!first_line) *out += ",";
			 first_line = false;
			 continue;
		 } else *Out << "ERROR : gentyp is unknown\n";
		 ap_environment_t* env = ap_generator1_envref (&gen);
		 size_t env_size = env->realdim + env->intdim;
		 *out += "[";
		 ap_coeff_t * coeff = ap_coeff_alloc(AP_COEFF_SCALAR);
		 legend = "[";
		 for (size_t j = 0; j < env_size; j++) {
			 if (j > 0) *out += ",";
		     ap_var_t var = ap_environment_var_of_dim(env,j);
			 legend += ((Value*)var)->getName();
			 legend += " ";
		     bool zero = ap_generator1_get_coeff(coeff,&gen,var);
			 if (!zero) {
				// we suppose here that the coeff is of type scalar 
		     	ap_scalar_t * scalar = coeff->val.scalar;
		     	switch(scalar->discr) {
		     	   case AP_SCALAR_DOUBLE:
		     	   	{
		     	   	double dbl = scalar->val.dbl;
					std::ostringstream oss;
					oss << dbl;
					*out += oss.str();
		     	   	}
		     	   	break;
		     	   case AP_SCALAR_MPQ:
		     	   	{
		     	   	mpq_ptr mpq = scalar->val.mpq;
					std::ostringstream oss;
					oss << mpq;
					*out += oss.str();
		     	   	}
		     	   	break;
		     	   case AP_SCALAR_MPFR:
		     	   	{
		     	   	mpfr_ptr mpfr = scalar->val.mpfr;
					std::ostringstream oss;
					oss << mpfr;
					*out += oss.str();
		     	   	}
		     	   	break;
		     	}
			 }
		 }
		 legend += "]";
		 *out += "]";
	}
	vertices += "]";
	rays += "]";
	lines += "]";
	*Out << "sage: Polyhedron(vertices=" + vertices + ", rays=" + rays + ", lines=" + lines + ")\n";
	*Out << "sage: legend=" + legend + "\n";
#endif
	std::string return_invariant = "";	
	std::string s;
	llvm::raw_string_ostream buffer(s);

#if 1
	PDEBUG(
	// first, print the environment
	Environment env(main->env);
	stream << "Abstract value environment:\n" << env << "\n";
	);

	ap_tcons1_array_t tcons_array = ap_abstract1_to_tcons_array(man,main);
	size_t size = ap_tcons1_array_size(&tcons_array);
	if (ap_abstract1_is_bottom(man,main)) {
		if (left != NULL) stream << *left;
		stream << "UNREACHABLE\n";
	} else if (size == 0) {
		if (left != NULL) stream << *left;
		stream << "TOP\n";
	} else {
		for (size_t k = 0; k < size; k++) {
			ap_tcons1_t cons = ap_tcons1_array_get(&tcons_array,k);
			if (left != NULL)
			{	
				stream << *left;
				buffer << *left;
			}
			stream << cons << "\n";
			buffer << cons << "\n";
		}
		return_invariant = buffer.str();
	}
	ap_tcons1_array_clear(&tcons_array);

#else
	FILE* tmp = tmpfile();
	if (tmp == NULL) {
		*Out << "ERROR WHEN PRINTING ABSTRACT VALUE\n";
		return;
	}

	ap_environment_fdump(tmp,main->env);
	ap_abstract1_fprint(tmp,man,main);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		stream << c;
	fclose(tmp);
#endif
	ret_invariant = return_invariant;
}
		
void AbstractClassic::to_MDNode(llvm::Instruction * Inst, std::vector<llvm::Metadata*> * met) {

	LLVMContext& C = Inst->getContext();
	ap_tcons1_array_t tcons_array = ap_abstract1_to_tcons_array(man,main);
	size_t size = ap_tcons1_array_size(&tcons_array);
	//met->push_back(Inst->getParent());
	if (ap_abstract1_is_bottom(man,main)) {
		met->push_back(MDString::get(C, "false"));
	} else if (size == 0) {
		met->push_back(MDString::get(C, "true"));
	} else {
#if 1
		// alternative invariant representation
		Environment env(this);
		std::vector<Metadata*> MDenv;
		env.to_MDNode(&C,&MDenv);
		met->push_back(MDNode::get(C,MDenv));
		std::vector<Metadata*> MDconstraints;
		for (size_t k = 0; k < size; k++) {
			ap_tcons1_t cons = ap_tcons1_array_get(&tcons_array,k);

			std::vector<llvm::Metadata*> c;
			ap_tcons1_t_to_MDNode(cons,Inst,&c);
			MDconstraints.push_back(MDNode::get(C,c));
		}
		met->push_back(MDNode::get(C,MDconstraints));
#else
		for (size_t k = 0; k < size; k++) {
			ap_tcons1_t cons = ap_tcons1_array_get(&tcons_array,k);
			//std::string s_string;
			//raw_string_ostream * s = new raw_string_ostream(s_string);
			//*s << cons;
			//std::string & cstr = s->str();
			//met->push_back(MDString::get(C, cstr));

			std::vector<llvm::Metadata*> c;
			ap_tcons1_t_to_MDNode(cons,Inst,&c);
			met->push_back(MDNode::get(C,c));

			FunctionType * ftype = FunctionType::get(Type::getVoidTy(Inst->getContext()),true);
			Module * M = Inst->getParent()->getParent()->getParent();
			//Constant * pagai_inv = M->getOrInsertFunction("__pagai_invariant",ftype);
			//CallInst * call = CallInst::Create(pagai_inv,MDNode::get(C,c),"",Inst);
		}
#endif
	}
	ap_tcons1_array_clear(&tcons_array);
}

void AbstractClassic::insert_as_LLVM_invariant(llvm::Instruction * Inst) {
	ap_tcons1_array_t tcons_array = ap_abstract1_to_tcons_array(man,main);
	size_t size = ap_tcons1_array_size(&tcons_array);
    LLVMContext &Context = Inst->getContext();
	
	IRBuilder<> Builder(Context);
	Builder.SetInsertPoint(Inst);
	Value * invariant = invariant = ConstantInt::getTrue(Context);
	if (ap_abstract1_is_bottom(man,main)) {
		invariant = ConstantInt::getFalse(Context);
	} else if (size == 0) {
		invariant = ConstantInt::getTrue(Context);
	} else {
		for (size_t k = 0; k < size; k++) {
			ap_tcons1_t cons = ap_tcons1_array_get(&tcons_array,k);
			invariant = Builder.CreateAnd(invariant,ap_tcons1_to_LLVM(cons,&Builder));
		}
	}

	Constant * invFn = Inst->getParent()->getParent()->getParent()->getOrInsertFunction(
			"pagai.invariant", 
			Type::getVoidTy(Context), 
			Type::getInt1Ty(Context),
			NULL);
	Builder.CreateCall(invFn,invariant);	
}
