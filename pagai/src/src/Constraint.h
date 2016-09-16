/**
 * \file Constraint.h
 * \brief Declaration of the Constraint and Constraint_array classes
 * \author Julien Henry
 */
#ifndef _CONSTRAINT_H
#define _CONSTRAINT_H

#include <vector>

#include "ap_global1.h"

class Expr;

/**
 * \class Constraint
 * \brief wrapper around ap_tcons1_t apron type
 */
class Constraint {

	private:
		ap_tcons1_t ap_cons;

	public:
		Constraint(ap_constyp_t constyp, Expr * expr, ap_scalar_t* scalar);

		~Constraint();

		ap_tcons1_t * get_ap_tcons1();
		void print();
};

/**
 * \class Constraint_array
 * \brief wrapper around ap_tcons1_array_t apron type
 */
class Constraint_array {

	private:

		std::vector<Constraint*> constraints;

		ap_tcons1_array_t ap_array;
		bool ap_array_ready;

	public:
		Constraint_array();
		Constraint_array(Constraint * c);

		~Constraint_array();

		void add_constraint(Constraint * cons);

		ap_tcons1_array_t * to_tcons1_array();
		ap_lincons1_array_t * to_lincons1_array();

		ap_environment_t * getEnv();

		size_t size();

		void print();
};
#endif
