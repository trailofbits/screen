/**
 * \file Abstract.h
 * \brief Declaration of the Abstract class
 * \author Julien Henry
 */
#ifndef _ABSTRACT_H
#define _ABSTRACT_H

#include <vector>

#include "ap_global1.h"

#if LLVM_VERSION_MAJOR>3 || LLVM_VERSION_MAJOR==3 && LLVM_VERSION_MINOR>=6
#include "llvm/Analysis/CFG.h"
#else
#include "llvm/Analysis/CFG.h"
#endif

class Node;
class SMTpass;
class AbstractMan;
class Expr;
class Environment;
class Constraint_array;

/**
 * \class Abstract
 * \brief Base class of abstract domains
 */
class Abstract {
	public:

		/**
		 * \brief apron manager
		 */
		ap_manager_t * man;

		/**
		 * \brief abstract value
		 */
		ap_abstract1_t * main;

		/**
		 * \brief second abstract value, != NULL only for AbstractGopan objects
		 */
		ap_abstract1_t * pilot;

	protected:
		/**
		 * \brief clears the abstract value
		 */
		virtual void clear_all() = 0;

	public:

		virtual ~Abstract() {};

		/**
		 * \brief abstract value is set to top
		 * \param env environment of the value
		 */
		virtual void set_top(Environment * env) = 0;

		/**
		 * \brief abstract value is set to bottom
		 * \param env environment of the value
		 */
		virtual void set_bottom(Environment * env) = 0;

		/**
		 * \brief change the environment of the abstract value
		 * \param env the new environment
		 */
		virtual void change_environment(Environment * env) = 0;

		/**
		 * \brief compare the abstract value with another one
		 * \return 0 in equal,
		 *   1 in case of this < d,
		 *  -1 in case of d < this,
		 *  -2 if not comparable
		 */
		int compare(Abstract * d);

		/**
		 * \brief check if the abstract value can be joined with another one
		 * without loss of precision
		 * \param aman apron manager
		 * \param A the abstract value to join with
		 * \return true iff the join of this and A is exactly the union
		 */
		bool CanJoinPrecisely(AbstractMan * aman, Abstract * A);

		/**
		 * \brief check if two abstract values have the same environment
		 * \param A the second abstract value
		 * \return true if environment(this) = environment(A)
		 */
		bool has_same_environment(Abstract * A);

		/**
		 * \brief compare two abstract values
		 * \param A the second abstract value
		 * \return true iff this <= d
		 */
		virtual bool is_leq(Abstract * d);
		
		/**
		 * \brief compare two abstract values
		 * \param A the second abstract value
		 * \return true iff this = d
		 */
		virtual bool is_eq(Abstract * d);

		/**
		 * \brief check if the value is bottom
		 * \return true if the value is bottom, else return false
		 */
		virtual bool is_bottom() = 0;

		/**
		 * \brief check if the value is top
		 * \return true if the value is top, else return false
		 */
		virtual bool is_top() = 0;

		/**
		 * \brief apply the widening operator, according to its
		 * definition in the domain.
		 * \param X the second argument of the widening operator
		 */
		virtual void widening(Abstract * X) = 0;

		/**
		 * \brief apply the widening operator with threshold, according to its
		 * definition in the domain.
		 * \param X the second argument of the widening operator
		 * \param cons the set of constraints to use as thresholds
		 */
		virtual void widening_threshold(Abstract * X, Constraint_array* cons) = 0;

		/**
		 * \brief intersect the abstract value with an array of
		 * constraints
		 * \param tcons the array of constraints to meet with
		 */
		virtual void meet_tcons_array(Constraint_array* tcons) = 0;

		/**
		 * \brief canonicalize the apron representation of the abstract 
		 * value
		 */
		virtual void canonicalize() = 0;


		/**
		 * \brief assign an expression to a set of variables
		 * \param tvar array of the variables to assign
		 * \param texpr  array of corresponding expressions 
		 * \param size size of the array
		 * \param dest see apron doc
		 */
		virtual void assign_texpr_array(
				ap_var_t* tvar, 
				ap_texpr1_t* texpr, 
				size_t size, 
				ap_abstract1_t* dest) = 0;

		/**
		 * \brief assign an expression to a set of variables
		 * \param name vector of the variables to assign
		 * \param expr vector of corresponding expressions 
		 * \param dest see apron doc
		 */
		void assign_texpr_array(
			std::vector<ap_var_t> * name,
			std::vector<Expr*> * expr,
			ap_abstract1_t* dest);
		
		/**
		 * \brief the abstract value becomes the join of a set of
		 * abstract values
		 * \param env the environment of the vector X_pred
		 * \param X_pred the set of abstract values to join
		 */
		virtual void join_array(Environment * env, std::vector<Abstract*> X_pred) = 0;

		/**
		 * \brief the abstract value becomes the dpUcm of a set of
		 * abstract values
		 * \param env the environment
		 * \param n the abstract value to join with
		 *
		 * See Lookahead Widening - Gopan and Reps - SAS 06 to understand the
		 * operation dpUcm
		 */
		virtual void join_array_dpUcm(Environment *env, Abstract* n) = 0;

		/**
		 * \brief meet the current abstract value with another one
		 * \param A the abstract value to meet with
		 */
		virtual void meet(Abstract* A) = 0;
		
		/**
		 * \brief convert the abstract value to a conjunction of
		 * tree constraints
		 */
		virtual ap_tcons1_array_t to_tcons_array() = 0;

		/**
		 * \brief convert the abstract value to a conjunction of
		 * linear constraints
		 */
		virtual ap_lincons1_array_t to_lincons_array() = 0;

		/**
		 * \brief print the abstract domain on standard output
		 */
		virtual void print(bool only_main = false) = 0;

		/**
		 * \brief print the abstract domain in the stream
		 * \param left print the string given as argument at the beginning of
		 * each new line
		 */
		virtual void display(llvm::raw_ostream &stream, std::string * left = NULL) const = 0;


		virtual void to_MDNode(llvm::Instruction * Inst, std::vector<llvm::Metadata*> * met) {}

		virtual void insert_as_LLVM_invariant(llvm::Instruction * Inst) {}
};

llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, Abstract const& A);

#endif
