/**
 * \file yices.h
 * \brief Declaration of the yices class
 * \author Julien Henry
 */
#ifndef YICES_H
#define YICES_H

#if HAS_YICES
#include <map>

#include "yices_c.h"

#include "SMT_manager.h"

/**
 * \class yices 
 * \brief Interface with the yices API
 */
class yices: public SMT_manager {
	private:
		yices_context ctx;
		std::map<std::string,SMT_var> vars;
		SMT_expr int0, real0;

	public:
		
		yices();
		~yices();

		SMT_expr SMT_mk_true();
		SMT_expr SMT_mk_false();

		SMT_var SMT_mk_bool_var(std::string name);
		SMT_var SMT_mk_var(std::string name,SMT_type type);
		SMT_expr SMT_mk_expr_from_bool_var(SMT_var var);
		SMT_expr SMT_mk_expr_from_var(SMT_var var);
		SMT_expr SMT_mk_or (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_and (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_xor (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e);
		SMT_expr SMT_mk_not (SMT_expr a);

		SMT_expr SMT_mk_num (int n);
		SMT_expr SMT_mk_num_mpq (mpq_t mpq);
		SMT_expr SMT_mk_real (double x);

		SMT_expr SMT_mk_sum (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_sub (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_mul (std::vector<SMT_expr> args);

		SMT_expr SMT_mk_sum (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_sub (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_mul (SMT_expr a1, SMT_expr a2);

		SMT_expr SMT_mk_eq (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_diseq (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_lt (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_le (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_gt (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_ge (SMT_expr a1, SMT_expr a2);

		SMT_expr SMT_mk_div (SMT_expr a1, SMT_expr a2, bool integer = true);
		SMT_expr SMT_mk_rem (SMT_expr a1, SMT_expr a2);
 
		SMT_expr SMT_mk_int2real(SMT_expr a);
		SMT_expr SMT_mk_real2int(SMT_expr a);
		SMT_expr SMT_mk_is_int(SMT_expr a);

		SMT_expr SMT_mk_int0();
		SMT_expr SMT_mk_real0();

		void push_context();
		void pop_context();

		void SMT_print(SMT_expr a);
		void SMT_assert(SMT_expr a);
		int SMT_check(SMT_expr a, std::set<std::string> * true_booleans);
};
#endif
#endif
