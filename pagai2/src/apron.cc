/**
 * \file apron.cc
 * \brief Implementation of Apron interface
 * \author Julien Henry
 */
#include <stdio.h>
#include <string>

#include "llvm/Analysis/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"
#include "box.h"
#include "oct.h"
#include "pk.h"
#include "pkeq.h"
#ifdef OPT_OCT_ENABLED
#include "opt_oct.h"
#endif
#ifdef PPL_ENABLED
#include "ap_ppl.h"
#include "ap_pkgrid.h"
#endif

#include "apron.h"
#include "Expr.h"
#include "SMTpass.h"
#include "Analyzer.h"
#include "Info.h"
#include "recoverName.h"
using namespace llvm;

ap_var_operations_t var_op_manager;

/*
 * new to_string function for the var_op_manager
 */
char* ap_var_to_string(ap_var_t var) {
	std::string name;
	if (Expr::is_undef_ap_var(var)) {
		name = "undef";
	} else {
		Value * val = dyn_cast<Value>((Value*)var);
	
		if (useSourceName()) {
			const Value * val1=val;	
			Info IN = recoverName::getMDInfos(val1);	
			if(!IN.empty()) {
				name=IN.getName();
			} else {
				PDEBUG(
					*Out << "IN is empty\n";
				);
				name = SMTpass::getVarName(val);
			}
		} else {
			name = SMTpass::getVarName(val);
		}
	}
	char * cname = (char*)malloc((name.size()+1)*sizeof(char));
	strcpy(cname,name.c_str());
	return cname;
}

/*
 * new compare function, working with Value * type
 */
int ap_var_compare(ap_var_t v1, ap_var_t v2) {
	if (v1 == v2) return 0;
	if (v1 > v2) return 1;
	return -1;
}

/*
 * hash function for ap_var_t
 */
int ap_var_hash(ap_var_t v) {
	return 0;
}

// no copy, no free ! 
ap_var_t ap_var_copy(ap_var_t var) {return var;}
void ap_var_free(ap_var_t var) {}

/*
 * This function aims to change the functions for the apron var manager,
 * since var type is not char* but Value*.
 */
void init_apron() {
	var_op_manager.compare = &ap_var_compare;
	var_op_manager.hash = &ap_var_hash;
	var_op_manager.copy = &ap_var_copy;
	var_op_manager.free = &ap_var_free;
	var_op_manager.to_string = &ap_var_to_string;

	ap_var_operations = &var_op_manager;
}

ap_manager_t * create_manager(Apron_Manager_Type man) {
	ap_manager_t * ap_man;
	switch (man) {
		case BOX:
			return box_manager_alloc(); // Apron boxes
		case OCT:
			return oct_manager_alloc(); // Octagons
#ifdef OPT_OCT_ENABLED
	        case OPT_OCT:
		        return opt_oct_manager_alloc(); // ETHZ optimized octagons
#endif

		case PK: 
			return pk_manager_alloc(true); // NewPolka strict polyhedra
		case PKEQ: 
			return pkeq_manager_alloc(); // NewPolka linear equalities
#ifdef PPL_ENABLED
		case PPL_POLY: 
			return ap_ppl_poly_manager_alloc(true); // PPL strict polyhedra
		case PPL_POLY_BAGNARA: 
			ap_man = ap_ppl_poly_manager_alloc(true); // PPL strict polyhedra
			ap_funopt_t funopt;
			ap_funopt_init(&funopt);
			funopt.algorithm = 1;
			ap_manager_set_funopt(ap_man,AP_FUNID_WIDENING,&funopt);
			return ap_man;
		case PPL_GRID: 
			return ap_ppl_grid_manager_alloc(); // PPL grids
		case PKGRID: 
			// Polka strict polyhedra + PPL grids
			return ap_pkgrid_manager_alloc(	pk_manager_alloc(true),
					ap_ppl_grid_manager_alloc()); 
#endif
	}
}


llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, ap_tcons1_t & cons) {

	ap_constyp_t* constyp = ap_tcons1_constypref(&cons);
	ap_texpr1_t texpr = ap_tcons1_texpr1ref(&cons);
	ap_scalar_t* scalar;

	stream << texpr;
	switch (*constyp) {
		case AP_CONS_EQ:
			stream << " = 0";
			break;
		case AP_CONS_SUPEQ:
			stream << " >= 0";
			break;
		case AP_CONS_SUP:
			stream << " > 0";
			break;
		case AP_CONS_EQMOD:
			scalar = ap_tcons1_scalarref(&cons);
			stream << " = 0 MOD " << *scalar;
			break;
		case AP_CONS_DISEQ:
			stream << " != 0";
			break;
	}
	return stream;
}



void interval_print(llvm::raw_ostream *stream, ap_interval_t * a) {
	*stream << "interval";
}

void coeff_print(llvm::raw_ostream *stream, ap_coeff_t * a) {
	switch(a->discr){
		case AP_COEFF_SCALAR:
			*stream << *(a->val.scalar);
			break;
		case AP_COEFF_INTERVAL:
			interval_print(stream,a->val.interval);
			break;
	}
}

simpl check_scalar(ap_scalar_t * a) {
	if (ap_scalar_equal_int(a,0))
		return ZERO;
	else if (ap_scalar_equal_int(a,1))
		return ONE;
	else if (ap_scalar_equal_int(a,-1))
		return MINUSONE;
	else if (ap_scalar_sgn(a) == -1)
		return NEGATIVE;
	else
		return POSITIVE;
}

simpl check_coeff(ap_coeff_t * a) {
    simpl res = DEFAULT;
	if (a->discr == AP_COEFF_SCALAR) {
		res = check_scalar(a->val.scalar);
	}
	assert(res != DEFAULT);
	return res;
}

simpl check_texpr0_node(ap_texpr0_node_t * a) {
	simpl A = check_texpr0(a->exprA);
	if (a->exprB) {
		simpl B = check_texpr0(a->exprB);
		if ((a->op == 0 || a->op == 1)) {
			// this is a + or a -
			if (A == ZERO) {
				return B;
			}
		}

		if (a->op == 2) {
			// this is a *
			if (A == ZERO) {
				return ZERO;
			}
			if (B == ZERO) {
				return ZERO;
			}
			if (A == ONE) {
				return B;
			}
			if (B == ONE) {
				return A;
			}
			if (A == MINUSONE) {
				return minus(B);
			}
			if (B == MINUSONE) {
				return minus(A);
			}
		}
	} else {
		return A;
	}
	return A;
}

simpl check_texpr0(ap_texpr0_t * a) {
	if ( a->discr == AP_TEXPR_CST) {
		return check_coeff(&a->val.cst);
	} else if (a->discr == AP_TEXPR_NODE) {
		return check_texpr0_node(a->val.node);	
	}
	return POSITIVE;
}

simpl minus(simpl s) {
	switch (s) {
		case ZERO:
			return ZERO;
		case ONE:
			return MINUSONE;
		case MINUSONE:
			return ONE;
		case POSITIVE:
			return NEGATIVE;
		case NEGATIVE:
			return POSITIVE;
		case DEFAULT:
			return DEFAULT;
	}
}

void texpr0_node_print(llvm::raw_ostream *stream, ap_texpr0_node_t * a, char ** name_of_dim) {
	int prec = ap_texpr_op_precedence[a->op];

	if (a->exprB) {
		int A = check_texpr0(a->exprA);
		int B = check_texpr0(a->exprB);

		if ((a->op == 0 || a->op == 1)) {
			// this is a + or a -
			if (A == ZERO) {
				texpr0_display(stream,a->exprB,name_of_dim);
				return;
			}
			if (B == ZERO) {
				texpr0_display(stream,a->exprA,name_of_dim);
				return;
			}
		}

		if (a->op == 2) {
			// this is a *
			if (A == ZERO) {
				return;
			}
			if (B == ZERO) {
				return;
			}
			if (A == ONE) {
				texpr0_display(stream,a->exprB,name_of_dim);
				return;
			}
			if (B == ONE) {
				texpr0_display(stream,a->exprA,name_of_dim);
				return;
			}
			if (A == MINUSONE) {
				*stream << "-";
				texpr0_display(stream,a->exprB,name_of_dim);
				return;
			}
			if (B == MINUSONE) {
				*stream << "-";
				texpr0_display(stream,a->exprA,name_of_dim);
				return;
			}
		}

		/* left argument (if binary) */
		int prec2 = ap_texpr0_precedence(a->exprA);
		if (prec2<prec) *stream << "(";
		texpr0_display(stream, a->exprA, name_of_dim);
		if (prec2<prec) *stream << ")";
		*stream << "";
	}

	/* operator & rounding mode */
	if (a->exprB 
		&& check_texpr0(a->exprB) != NEGATIVE
		&& check_texpr0(a->exprB) != MINUSONE
	   ){
	*stream << ap_texpr_op_name[a->op];
	if (!ap_texpr0_node_exact(a))
		*stream 
			<< "_" 
			<< ap_texpr_rtype_name[a->type] 
			<< ","
			<<  ap_texpr_rdir_name[a->dir];
	}

	/* right argument */
	{
		ap_texpr0_t* arg = a->exprB ? a->exprB : a->exprA;
		int prec2 = ap_texpr0_precedence(arg);
		if (a->exprB) *stream << "";
		if (prec2<=prec) *stream << "(";
		texpr0_display(stream,arg,name_of_dim);
		if (prec2<=prec) *stream << ")";
	}
}

void texpr0_display(llvm::raw_ostream  * stream, ap_texpr0_t* a, char ** name_of_dim) {
	if (!a) return;
	switch (a->discr) {
		case AP_TEXPR_CST:
			coeff_print(stream, &a->val.cst);
			break;
		case AP_TEXPR_DIM:
			if (name_of_dim) *stream << name_of_dim[a->val.dim];
			else             *stream << (unsigned long)a->val.dim;
			break;
		case AP_TEXPR_NODE:
			texpr0_node_print(stream, a->val.node, name_of_dim);
			break;
		default:
			assert(false && "error in texpr0_display");
	}
}


llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, ap_texpr1_t & cons) {

	ap_environment_name_of_dim_t* name_of_dim;

	name_of_dim = ap_environment_name_of_dim_alloc(cons.env);
	texpr0_display(&stream, cons.texpr0, name_of_dim->p);
	ap_environment_name_of_dim_free(name_of_dim);

	return stream;
}

llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, ap_scalar_t & cons) {
	int flag;

	flag = ap_scalar_infty(&cons);
	if (flag){
		if (flag > 0)
			stream << "+infty";
		else
			stream << "-infty";
	} else {
		char * num = NULL;
		switch(cons.discr){
			case AP_SCALAR_DOUBLE:
				stream << cons.val.dbl + 0.0;
				break;
			case AP_SCALAR_MPQ:
				num = mpq_get_str(num,10,cons.val.mpq);
				stream << num;
				free(num);
				break;
			case AP_SCALAR_MPFR:
				{
					double d = mpfr_get_d(cons.val.mpfr,GMP_RNDU);
					if (mpfr_cmp_d(cons.val.mpfr,d)) 
						//mpfr_out_str(stream,10,ap_scalar_print_prec,a->val.mpfr,GMP_RNDU);
						stream << "mpfr";
					else 
						//fprintf(stream,"%.*g",ap_scalar_print_prec,d + 0.0);
						stream << d + 0.0;
				}
				break;
			default: 
				break;
		}
	} 
	return stream;
}
