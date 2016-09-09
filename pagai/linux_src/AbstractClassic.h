/**
 * \file AbstractClassic.h
 * \brief Declaration of the AbstractClassic class
 * \author Julien Henry
 */
#ifndef _ABSTRACTCLASSIC_H
#define _ABSTRACTCLASSIC_H

#include <vector>

#include "ap_global1.h"
#include "Abstract.h"

class Node;
class AbstractGopan;

/**
 * \class AbstractClassic
 * \brief abstract domain used by every AI pass but AIGopan
 */
class AbstractClassic: public Abstract {

	protected:
		/**
		 * \brief clears the abstract value
		 */
		void clear_all();

	public:

		/**
		 * \brief creates a TOP abstract value in the environment env
		 * \param _man apron manager 
		 * \param env the environment of the abstract value
		 */
		AbstractClassic(ap_manager_t* _man, Environment * env);

		/**
		 * \brief copy constructor : duplicates the abstract value
		 * \param A the abstract value to copy
		 */
		AbstractClassic(Abstract* A);

		~AbstractClassic();

		/**
		 * \brief abstract value is set to top
		 * \param env environment of the value
		 */
		void set_top(Environment * env);

		/**
		 * \brief abstract value is set to bottom
		 * \param env environment of the value
		 */
		void set_bottom(Environment * env);

		/**
		 * \brief change the environment of the abstract value
		 * \param env the new environment
		 */
		void change_environment(Environment * env);

		/**
		 * \brief check if the value is bottom
		 * \return true if the value is bottom, else return false
		 */
		bool is_bottom();

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
		 * \brief apply the widening operator with threshold, according to its
		 * definition in the domain.
		 * \param X the second argument of the widening operator
		 * \param cons the set of constraints to use as thresholds
		 */
		void widening_threshold(Abstract * X, Constraint_array* cons);

		/**
		 * \brief intersect the abstract value with an array of
		 * constraints
		 * \param tcons the array of constraints to meet with
		 */
		void meet_tcons_array(Constraint_array* tcons);

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
		 * \brief the abstract value becomes the join of a set of
		 * abstract values
		 * \param env the environment of the vector X_pred
		 * \param X_pred the set of abstract values to join
		 */
		void join_array(Environment * env, std::vector<Abstract*> X_pred);

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
		 * \brief convert the abstract value to a conjunction of
		 * linear constraints
		 */
		ap_lincons1_array_t to_lincons_array();

		/**
		 * \brief print the abstract domain on standard output
		 */
		void print(std::string &outString, bool only_main = false);

		/**
		 * \brief print the abstract domain in the stream
		 * \param left print the string given as argument at the beginning of
		 * each new line
		 */
		void display(llvm::raw_ostream &stream, std::string &ret_invariant, std::string * left = NULL) const;

		
		void to_MDNode(llvm::Instruction * Inst, std::vector<llvm::Metadata*> * met);

		void insert_as_LLVM_invariant(llvm::Instruction * Inst);
};
#endif
