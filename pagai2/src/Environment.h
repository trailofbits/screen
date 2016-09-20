/**
 * \file Environment.h
 * \brief Declaration of the Environment class
 * \author Julien Henry
 */
#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include <set>

#include "ap_global1.h"
#include "Abstract.h"
#include "Live.h"
#include "Constraint.h"
#include "Node.h"

using namespace llvm;

/**
 * \class Environment
 * \brief wrapper around ap_environment_t apron type
 */
class Environment {

	private:
		ap_environment_t * env;

		void init(std::set<ap_var_t> * intvars, std::set<ap_var_t> * realvars);

	public:
		/**
		 * \{
		 * \name Constructors
		 */
		Environment();
		Environment(const Environment &e);
		Environment(std::set<ap_var_t> * intvars, std::set<ap_var_t> * realvars);
		Environment(Abstract * A);
		Environment(ap_tcons1_array_t * cons);
		Environment(Constraint * cons);
		Environment(Constraint_array * cons);
		Environment(ap_environment_t * e);
		Environment(Node * n, Live * LV);
		/**
		 * \}
		 */

		~Environment();

		/**
		 * \brief Overloaded copy assignment operator
		 */
		Environment & operator= (const Environment &e);
	
		/**
		 * \brief Overloaded equality test
		 */
		bool operator == (const Environment &e);

		/**
		 * \brief Overloaded disequality test
		 */
		bool operator != (const Environment &e);

		bool operator <= (const Environment &e);

		ap_environment_t * getEnv();

		/**
		 * insert into intdims the int dimensions of the environment
		 * insert into realdims the real dimensions of the environment
		 */
		void get_vars(std::set<ap_var_t> * intdims, std::set<ap_var_t> * realdims);

		/**
		 * \brief same as get_vars, but gets only variables that are live in b
		 */
		void get_vars_live_in(
				BasicBlock * b, Live * LV,
				std::set<ap_var_t> * intdims, 
				std::set<ap_var_t> * realdims);

		/**
		 * \brief modifies exp1 and exp2, so that they have the same env
		 */
		static void common_environment(ap_texpr1_t * exp1, ap_texpr1_t * exp2);

		/**
		 * \brief compute the least common environment of two expressions
		 */
		static Environment common_environment(Expr* exp1, Expr* exp2);

		/**
		 * \brief compute the least common environment of two environments
		 */
		static Environment common_environment(Environment* env1, Environment* env2);

		/**
		 * \brief compute the intersection of two environments
		 */
		static Environment intersection(Environment * env1, Environment * env2);

		/**
		 * \brief print the environment
		 */
		void display(llvm::raw_ostream &stream) const;

		
		void to_MDNode(LLVMContext * C, std::vector<llvm::Metadata*> * met);

	private:

		/**
		 * \brief compute the intersection of two apron environments
		 */
		static ap_environment_t * common_environment(ap_environment_t * env1, ap_environment_t * env2);
};

llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, const Environment& env);
#endif
