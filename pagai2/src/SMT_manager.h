/**
 * \file SMT_manager.h
 * \brief Declaration of the SMT_manager class
 * \author Julien Henry
 */
#ifndef SMT_MANAGER_H
#define SMT_MANAGER_H

#include <string>
#include <vector>
#include <set>

#include <boost/shared_ptr.hpp>

#include "gmp.h"
#include "mpfr.h"
#include "Debug.h"
#ifdef HAS_Z3
#include "z3++.h"
#endif

/**
 * \class SMT_expr
 * \brief class of SMT expressions
 */
class SMT_expr {
	private:
		std::string s;
		Z3(boost::shared_ptr<z3::expr> z3;)

	public:
		void * i;

		SMT_expr () {
			s = std::string("");
			Z3(z3.reset();)
			i = NULL;
		}

		SMT_expr (const SMT_expr& e): s(e.s), i(e.i) {
			Z3(z3 = e.z3;)
		}

		Z3(SMT_expr(z3::expr e) {
			s = std::string("");
			z3.reset(new z3::expr(e));
			i = NULL;
		})

		SMT_expr(std::string _s) : s(_s) {
			Z3(z3.reset();)
			i = NULL;
		}

		Z3(z3::expr * expr() {
			return z3.get();
		})

		std::string SMTlib() {
			return s;
		}

		~SMT_expr(){
			Z3(z3.reset();)
			s.clear();
		}

		bool is_empty() {
			return i == NULL Z3(&& z3.get() == NULL) && s == "";
		}

		SMT_expr& operator=(const SMT_expr &e) {
			s.clear();
			s = e.s;
			Z3(z3 = e.z3;)
			i = e.i;
			return *this;
		}
};

/**
 * \class SMT_type
 * \brief class of SMT types
 */
class SMT_type {
	public:
		std::string s;
		void* i;
		Z3(boost::shared_ptr<z3::sort> z3;)

		SMT_type () {
			s = std::string("");
			i = NULL;
			Z3(z3.reset();)
		}

		SMT_type (const SMT_type& t): s(t.s), i(t.i) {
			Z3(z3 = t.z3;)
		}

		~SMT_type(){
			Z3(z3.reset();)
			s.clear();
		}

		Z3(SMT_type(z3::sort t) {
			s = std::string("");
			z3.reset(new z3::sort(t));
			i = NULL;
		})

		Z3(z3::sort * sort() {
			return z3.get();
		})

		Z3(void z3_clear() {
			z3.reset();
		})

		bool is_empty() {
			return i == NULL Z3(&& z3.get() == NULL) && s == "";
		}

		SMT_type& operator=(const SMT_type &t) {
			s.clear();
			s = t.s;
			Z3(z3 = t.z3;)
			i = t.i;
			return *this;
		}
};

/**
 * \class SMT_var
 * \brief class of SMT variables
 */
class SMT_var {
	public:
		std::string s;
		Z3(boost::shared_ptr<z3::symbol> z3;)
		void* i;

		SMT_var () {
			s = std::string("");
			Z3(z3.reset();)
			i = NULL;
		}

		Z3(SMT_var(z3::symbol n) {
			s = n.str();
			z3.reset(new z3::symbol(n));
			i = NULL;
		};)

		~SMT_var() { }

		SMT_var& operator=(const SMT_var &v) {
			s.clear();
			s = v.s;
			Z3(z3 = v.z3;)
			i = v.i;
			return *this;
		}

		Z3(z3::symbol * symb() {
			return z3.get();
		})

		int Compare (const SMT_var& v) const {
			if (i < v.i)
				return -1;
			else if (i > v.i)
				return 1;
			else {
				if (s < v.s)
					return -1;
				else if (s > v.s)
					return 1;
				else return 0;
			}
		}

		bool operator == (const SMT_var& v) const {
			return !Compare(v);
		}

		bool operator < (const SMT_var& v) const {
			return Compare(v)<0;   
		}
}; 

/**
 * \class SMT_manager
 * \brief interface of an SMT manager
 */
class SMT_manager {
	public:
		SMT_type int_type;
		SMT_type float_type;
		SMT_type bool_type;
	public:
		virtual ~SMT_manager() {}

		virtual SMT_expr SMT_mk_true() = 0;
		virtual SMT_expr SMT_mk_false() = 0;

		virtual SMT_var SMT_mk_bool_var(std::string name) = 0;
		virtual SMT_var SMT_mk_var(std::string name,SMT_type type) = 0;
		virtual SMT_expr SMT_mk_expr_from_bool_var(SMT_var var) = 0;
		virtual SMT_expr SMT_mk_expr_from_var(SMT_var var) = 0;

		virtual SMT_expr SMT_mk_or (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_and (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_xor (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e) = 0;
		virtual SMT_expr SMT_mk_not (SMT_expr a) = 0;

		virtual SMT_expr SMT_mk_num (int n) = 0;
		virtual SMT_expr SMT_mk_num_mpq (mpq_t mpq) = 0;
		virtual SMT_expr SMT_mk_real (double x) = 0;

		virtual SMT_expr SMT_mk_sum (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_sub (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_mul (std::vector<SMT_expr> args) = 0;

		virtual SMT_expr SMT_mk_sum (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_sub (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_mul (SMT_expr a1, SMT_expr a2) = 0;

		virtual SMT_expr SMT_mk_eq (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_diseq (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_lt (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_le (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_gt (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_ge (SMT_expr a1, SMT_expr a2) = 0;

		virtual SMT_expr SMT_mk_divides (SMT_expr a1, SMT_expr a2);

		virtual SMT_expr SMT_mk_div (SMT_expr a1, SMT_expr a2, bool integer = true) = 0;
		virtual SMT_expr SMT_mk_rem (SMT_expr a1, SMT_expr a2) = 0;

		virtual SMT_expr SMT_mk_int2real(SMT_expr a) = 0;
		virtual SMT_expr SMT_mk_real2int(SMT_expr a) = 0;
		virtual SMT_expr SMT_mk_is_int(SMT_expr a) = 0;

		virtual SMT_expr SMT_mk_int0() = 0;
		virtual SMT_expr SMT_mk_real0() = 0;

		virtual void push_context() = 0;
		virtual void pop_context() = 0;

		virtual void SMT_print(SMT_expr a) = 0;
		virtual void SMT_assert(SMT_expr a) = 0;
		virtual int SMT_check(SMT_expr a, std::set<std::string> * true_booleans) = 0;

		virtual bool interrupt();
};

#endif
