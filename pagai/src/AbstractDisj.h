/**
 * \file AbstractDisj.h
 * \brief Declaration of the AbstractDisj class
 * \author Julien Henry
 */
#ifndef _ABSTRACTDISJ_H
#define _ABSTRACTDISJ_H

#include <vector>

#include "ap_global1.h"
#include "Abstract.h"
#include "AbstractMan.h"

class Node;
class Sigma;

/** 
 * \class AbstractDisj
 * \brief Abstract Domain used for computing disjunctive invariants
 */
class AbstractDisj: public Abstract {
	friend class Sigma;

	public:
		/**
		 * \brief manager for creating / deleting disjuncts
		 */
		AbstractMan * man_disj;

	protected:
		/**
		 * \brief set of disjuncts
		 */
		std::vector<Abstract*> disj;
		
		/**
		 * \brief clears the abstract value
		 */
		void clear_all();

	public:
		/**
		 * create a disjunctive invariant with one sigle disjunct
		 */
		AbstractDisj(ap_manager_t* _man, Environment * env);

		/**
		 * \brief create a disjunctive invariant with max_index+1 disjunct
		 */
		AbstractDisj(ap_manager_t* _man, Environment * env, int max_index);

		/**
		 * \brief add a new disjunct in the abstract
		 * \return the index of the new disjunct
		 */
		int AddDisjunct(Environment * env);
		
		/**
		 * \brief get the disjunct of index 'index'
		 * \return the corresponding disjunct
		 */
		Abstract * getDisjunct(int index);

		/**
		 * \brief set the disjunct 'index' to the abstract A
		 * IMPORTANT: A should not be duplicated, and should not be deleted
		 */
		void setDisjunct(int index, Abstract * A);

		/** 
		 * \brief verifies that the AbstractDisj has at least N disjunct
		 *
		 * if not, it creates disjuncts so that the AbstractDisj has N
		 * disjuncts
		 */
		void SetNDisjunct(int N);
		
		/**
		 * \brief get the index of the last disjunct
		 *
		 * WARNING : the first index is 0. If the result of this method is N,
		 * it means the invariant has N+1 disjuncts, indexed from 0 to N
		 */
		int getMaxIndex();

		/**
		 * copy constructor : duplicates the abstract value
		 */
		AbstractDisj(Abstract* A);

		~AbstractDisj();

		/**
		 * \brief abstract value is set to top
		 * \param env environment of the value
		 */
		void set_top(Environment * env);

		/**
		 * \brief set a disjunct to top
		 * \param env environment of the value
		 * \param index  index of the disjunct
		 */
		void set_top(Environment * env, int index);

		/**
		 * \brief abstract value is set to bottom
		 * \param env environment of the value
		 */
		void set_bottom(Environment * env);

		/**
		 * \brief set a disjunct to bottom
		 * \param env environment of the value
		 * \param index  index of the disjunct
		 */
		void set_bottom(Environment * env, int index);

		/**
		 * \brief change the environment of the abstract value
		 * \param env the new environment
		 */
		void change_environment(Environment * env);
		/**
		 * \brief change the environment of a disjunct
		 * \param env the new environment
		 * \param index  index of the disjunct
		 */
		void change_environment(Environment * env, int index);

		/**
		 * \brief compare a disjunct with an abstract value
		 * \param d the abstract value
		 * \param index  index of the disjunct
		 * \return true iff disjunct(index) <= d
		 */
		bool is_leq_index(Abstract * d, int index);

		/**
		 * \brief compare a disjunct with an abstract value
		 * \param d the abstract value
		 * \param index  index of the disjunct
		 * \return true iff disjunct(index) = d
		 */
		bool is_eq_index(Abstract * d, int index);

		/**
		 * \brief check if the value is bottom
		 * \return true if the value is bottom, else return false
		 */
		bool is_bottom();

		/**
		 * \brief check if the disjunct is bottom
		 * \param index  index of the disjunct
		 * \return true if the disjunct is bottom, else return false
		 */
		bool is_bottom(int index);

		/**
		 * \brief check if the value is top
		 * \return true if the value is top, else return false
		 */
		bool is_top();
		

		/**
		 * \brief apply the widening operator, according to its
		 * definition in the domain.
		 * \param X the second argument of the widening operator
		 */
		void widening(Abstract * X);

		/**
		 * \brief apply the widening operator, according to its
		 * definition in the domain.
		 * \param X the second argument of the widening operator
		 * \param index  index of the disjunct
		 */
		void widening(Abstract * X, int index);
		
		/**
		 * \brief apply the widening operator with threshold, according to its
		 * definition in the domain.
		 * \param X the second argument of the widening operator
		 * \param cons the set of constraints to use as thresholds
		 */
		void widening_threshold(Abstract * X, Constraint_array* cons);

		/**
		 * \brief apply the widening operator with threshold, according to its
		 * definition in the domain.
		 * \param X the second argument of the widening operator
		 * \param cons the set of constraints to use as thresholds
		 * \param index  index of the disjunct
		 */
		void widening_threshold(Abstract * X, Constraint_array* cons, int index);

		/**
		 * \brief intersect the abstract value with an array of
		 * constraints
		 * \param tcons the array of constraints to meet with
		 */
		void meet_tcons_array(Constraint_array* tcons);

		/**
		 * \brief intersect the disjunct with an array of
		 * constraints
		 * \param tcons the array of constraints to meet with
		 * \param index  index of the disjunct
		 */
		void meet_tcons_array(Constraint_array* tcons, int index);

		/**
		 * \brief canonicalize the apron representation of the abstract 
		 * value
		 */
		void canonicalize();

		/**
		 * \brief assign an expression to a set of variables
		 * \param tvar array of the variables to assign
		 * \param texpr  array of corresponding expressions 
		 * \param size size of the array
		 * \param dest see apron doc
		 */
		void assign_texpr_array(
				ap_var_t* tvar, 
				ap_texpr1_t* texpr, 
				size_t size, 
				ap_abstract1_t* dest);

		/**
		 * \brief assign an expression to a set of variables
		 * \param tvar array of the variables to assign
		 * \param texpr  array of corresponding expressions 
		 * \param size size of the array
		 * \param dest see apron doc
		 * \param index  index of the disjunct
		 */
		void assign_texpr_array(
				ap_var_t* tvar, 
				ap_texpr1_t* texpr, 
				size_t size, 
				ap_abstract1_t* dest,
				int index);
		
		/**
		 * \brief the abstract value becomes the join of a set of
		 * abstract values
		 * \param env the environment of the vector X_pred
		 * \param X_pred the set of abstract values to join
		 */
		void join_array(Environment * env, std::vector<Abstract*> X_pred);

		/**
		 * \brief the disjunct becomes the join of a set of
		 * abstract values
		 * \param env the environment of the vector X_pred
		 * \param X_pred the set of abstract values to join
		 * \param index  index of the disjunct
		 */
		void join_array(Environment * env, std::vector<Abstract*> X_pred, int index);

		/**
		 * \brief the abstract value becomes the dpUcm of a set of
		 * abstract values
		 * \param env the environment
		 * \param n the abstract value to join with
		 *
		 * See Lookahead Widening - Gopan and Reps - SAS 06 to understand the
		 * operation dpUcm
		 */
		void join_array_dpUcm(Environment *env, Abstract* n);

		/**
		 * \brief the disjunct becomes the dpUcm of a set of
		 * abstract values
		 * \param env the environment
		 * \param n the abstract value to join with
		 * \param index  index of the disjunct
		 *
		 * See Lookahead Widening - Gopan and Reps - SAS 06 to understand the
		 * operation dpUcm
		 */
		void join_array_dpUcm(Environment *env, Abstract* n, int index);
		
		/**
		 * \brief meet the current abstract value with another one
		 * \param A the abstract value to meet with
		 */
		void meet(Abstract* A);

		/**
		 * \brief convert the abstract value to a conjunction of
		 * tree constraints
		 */
		ap_tcons1_array_t to_tcons_array();

		/**
		 * \brief convert the disjunct to a conjunction of
		 * tree constraints
		 * \param index  index of the disjunct
		 */
		ap_tcons1_array_t to_tcons_array(int index);

		/**
		 * \brief convert the abstract value to a conjunction of
		 * linear constraints
		 */
		ap_lincons1_array_t to_lincons_array();

		/**
		 * \brief convert the disjunct to a conjunction of
		 * linear constraints
		 * \param index  index of the disjunct
		 */
		ap_lincons1_array_t to_lincons_array(int index);

		/**
		 * \brief print the abstract domain on standard output
		 */
		void print(bool only_main = false);

		/**
		 * \brief print the abstract domain in the stream
		 * \param left print the string given as argument at the beginning of
		 * each new line
		 */
		void display(llvm::raw_ostream &stream, std::string * left = NULL) const;
};
#endif
