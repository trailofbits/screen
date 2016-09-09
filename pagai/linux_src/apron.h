/**
 * \file apron.h
 * \brief Declares functions related to the Apron interface
 * \author Julien Henry
 */
#ifndef _APRON_H 
#define _APRON_H 

//#include "llvm/Analysis/CFG.h"

#include "ap_global1.h"

#include "Analyzer.h"
#include "Node.h"

using namespace llvm;

/**
 * \brief initialize the apron library. 
 *
 * This function has to be called at the very beginning of the pass
 */
void init_apron();

/**
 * \brief creates an apron manager
 */
ap_manager_t * create_manager(Apron_Manager_Type man);

char* ap_var_to_string(ap_var_t var);


llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, ap_tcons1_t & cons);

llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, ap_texpr1_t & cons);

llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, ap_scalar_t & cons);

void texpr0_display(llvm::raw_ostream * stream, ap_texpr0_t* a, char ** name_of_dim);

typedef enum _simpl {ZERO,ONE,MINUSONE,NEGATIVE,POSITIVE,DEFAULT} simpl; 
simpl minus(simpl s);
simpl check_scalar(ap_scalar_t * a);
simpl check_coeff(ap_coeff_t * a);
simpl check_texpr0_node(ap_texpr0_node_t * a);
simpl check_texpr0(ap_texpr0_t * a);


static const int ap_texpr_op_precedence[] =
{ 1, 1, 2, 2, 2,  /* binary */
	3, 4, 4         /* unary */
};

static const char* ap_texpr_op_name[] =
{ "+", "-", "*", "/", "%", /* binary */
	"-", "cast", "sqrt",     /* unary */
};

static const char* ap_texpr_rtype_name[] =
{ "", "i", "f", "d", "l", "q", };

static const char* ap_texpr_rdir_name[] =
{ "n", "0", "+oo", "-oo", "?", "", };

/* node induces some rounding (to float or integer) */
static inline bool ap_texpr0_node_exact(ap_texpr0_node_t* a)
{
	if (a->op==AP_TEXPR_NEG || a->op==AP_TEXPR_MOD ||
			a->type==AP_RTYPE_REAL) return true;
	return false;
}

static inline int ap_texpr0_precedence(ap_texpr0_t* a)
{
	if (!a || a->discr!=AP_TEXPR_NODE) return ap_texpr_op_precedence[AP_TEXPR_NEG];
	return ap_texpr_op_precedence[a->val.node->op];
}

#endif
