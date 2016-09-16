/**
 * \file SMTlib.cc
 * \brief Implementation of the SMTlib class
 * \author Julien Henry
 */
#include <algorithm>
#include <cstddef>
#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>

#include "llvm/Support/FormattedStream.h"

#include <gmp.h>
#include "SMTlib.h"
#include "Analyzer.h"
#include "Debug.h"
#include "SMTlib2driver.h"


SMTlib::SMTlib() {
	stack_level = 0;
	SMTlib_init();
}

static int logfile_counter = 0;

void SMTlib::SMTlib_init() {
	if (log_smt_into_file()) {
		char filename[sizeof("logfile-000.smt2")];
		sprintf(filename, "logfile-%03d.smt2", logfile_counter++);
		log_file = fopen(filename, "w");
	} else {
		log_file = NULL;
	}


	int_type.s = "Int";
	float_type.s = "Real";
	bool_type.s = "Bool";
	char buf;
	solver_pid = 0;

	if (pipe(rpipefd) == -1) {
		exit(EXIT_FAILURE);
	}
	if (pipe(wpipefd) == -1) {
		exit(EXIT_FAILURE);
	}

	pid_t cpid = fork();
	if (cpid == -1) {
		exit(EXIT_FAILURE);
	}
	if (cpid == 0) {
		/* Child : SMT solver */
		close(wpipefd[1]);
		close(rpipefd[0]);
		dup2(wpipefd[0], STDIN_FILENO);
		dup2(rpipefd[1], STDOUT_FILENO);
		close(wpipefd[0]);
		close(rpipefd[1]);
		switch (getSMTSolver()) {
			case MATHSAT:
				char * mathsat_argv[2];
				mathsat_argv[0] = const_cast<char*>("mathsat");
				mathsat_argv[1] = NULL;
				if (execvp("mathsat",mathsat_argv)) {
					perror("exec mathsat");
					exit(1);
				}
				break;
			case SMTINTERPOL:
				char * smtinterpol_argv[2];
				smtinterpol_argv[0] = const_cast<char*>("smtinterpol");
				smtinterpol_argv[1] = NULL;
				if (execvp("smtinterpol",smtinterpol_argv)) {
					perror("exec smtinterpol");
					exit(1);
				}		
				break;
			case Z3:
			case Z3_QFNRA:
				char * z3_argv[4];
				z3_argv[0] = const_cast<char*>("z3");
				z3_argv[1] = const_cast<char*>("-smt2");
				z3_argv[2] = const_cast<char*>("-in");
				z3_argv[3] = NULL;
				if (execvp("z3",z3_argv)) {
					perror("exec z3");
					exit(1);
				}
				break;
			case CVC3:
				char * cvc3_argv[4];
				cvc3_argv[0] = const_cast<char*>("cvc3");
				cvc3_argv[1] = const_cast<char*>("-lang");
				cvc3_argv[2] = const_cast<char*>("smt2");
				cvc3_argv[3] = NULL;
				if (execvp("cvc3",cvc3_argv)) {
					perror("exec cvc3");
					exit(1);
				}
				break;
			case CVC4:
				char * cvc4_argv[9];
				cvc4_argv[0] = const_cast<char*>("cvc4");
				cvc4_argv[1] = const_cast<char*>("--lang");
				cvc4_argv[2] = const_cast<char*>("smt2");
				cvc4_argv[3] = const_cast<char*>("--output-lang");
				cvc4_argv[4] = const_cast<char*>("smt2");
				cvc4_argv[5] = const_cast<char*>("--quiet");
				cvc4_argv[6] = const_cast<char*>("--produce-models");
				cvc4_argv[7] = const_cast<char*>("--incremental");
				cvc4_argv[8] = NULL;
				if (execvp("cvc4",cvc4_argv)) {
					perror("exec cvc4");
					exit(1);
				}
				break;
			default:
				exit(1);
		}
	}

	/* Parent : PAGAI */
	solver_pid = cpid;
	close(wpipefd[0]);
	close(rpipefd[1]);
	input = fdopen(rpipefd[0],"r");
	//setbuf(input, NULL);
	if (input == NULL) {
		perror("fdopen");
		exit(1);
	}

	//Enable model construction
	if (getSMTSolver() == CVC3 || getSMTSolver() == CVC4) {
		pwrite("(set-logic AUFLIRA)\n");
	} else {
		pwrite("(set-option :produce-models true)\n");
		pwrite("(set-option :produce-unsat-cores true)\n");
		if (getSMTSolver() == Z3 || getSMTSolver() == Z3_QFNRA) {
			pwrite("(set-option :interactive-mode true)\n");
			pwrite("(set-option :global-decls false)\n");
			if (getTimeout() != 0) {
				std::ostringstream timeout;
				timeout << getTimeout()*1000;
				pwrite("(set-option :soft-timeout "+timeout.str()+")\n");
			}
		}
		if (getSMTSolver() == SMTINTERPOL) {
			pwrite("(set-logic QF_UFLIRA)\n");
		}
		pwrite("(set-option :print-success false)\n");
	}
	//pwrite("(set-logic QF_LRA)\n");
}

void SMTlib::SMTlib_close() {
	pwrite("(exit)\n");
	close(wpipefd[1]); /* Reader will see EOF */
	close(rpipefd[0]);
	if (log_file) fclose(log_file);
	wait(NULL);
}

SMTlib::~SMTlib() {
	SMTlib_close();
}

void SMTlib::pwrite(std::string s) {
	PDEBUG(*Out << "WRITING : " << s  << "\n";);
	// DM pquoi pas s.length() ?
	if (!write(wpipefd[1], s.c_str(), strlen(s.c_str()))) {
		*Out << "ERROR WHEN TRYING TO WRITE IN THE SMT-LIB PIPE\n";
	}
	if (log_file) {
		fputs(s.c_str(), log_file);
		fflush(log_file);
	}
}

int SMTlib::pread() {
	int ret;

	SMTlib2driver driver;
	driver.parse(input);

	switch (driver.ans) {
		case SAT:
			ret = 1;
			break;
		case UNSAT:
			ret = 0;
			break;
		case UNKNOWN:
			*Out << "UNKNOWN\n";
			ret = -1;
			break;
		case ERROR:
			*Out << "SMT-SOLVER INTERNAL ERROR\n";
			SMTlib_close();
			SMTlib_init();
			for (int i = 0; i < stack_level; i++) {
				pwrite("(push 1)\n");
			}
			ret = -1;
			break;
		default:
			ret = -1;
	}

	model.clear();
	model.insert(driver.model.begin(),driver.model.end());
	return ret;
}

SMT_expr SMTlib::SMT_mk_true(){
	return SMT_expr("true");
}

SMT_expr SMTlib::SMT_mk_false(){
	return SMT_expr("false");
}

SMT_var SMTlib::SMT_mk_bool_var(std::string name){
	if (!vars.count(name)) {
		SMT_var res;
		res.s = name;
		vars[name].var = res;
		vars[name].stack_level = stack_level;
		vars[name].declaration = "(declare-fun " + name + " () Bool)\n";
		pwrite(vars[name].declaration);
	}
	return vars[name].var;
}

SMT_var SMTlib::SMT_mk_var(std::string name, SMT_type type){
	if (!vars.count(name)) {
		SMT_var res;
		res.s = name;
		vars[name].var = res;
		vars[name].stack_level = stack_level;
		vars[name].declaration = "(declare-fun " + name + " () " + type.s + ")\n";
		pwrite(vars[name].declaration);
	}
	return vars[name].var;
}

SMT_expr SMTlib::SMT_mk_expr_from_bool_var(SMT_var var){
	return SMT_expr(var.s);
}

SMT_expr SMTlib::SMT_mk_expr_from_var(SMT_var var){
	return SMT_expr(var.s);
}

SMT_expr SMTlib::SMT_mk_or (std::vector<SMT_expr> args){
	switch (args.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			return SMT_expr(args[0].SMTlib());
			break;
		default:
			std::string or_smt;
			or_smt = "(or "; 

			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			for (; b != e; ++b) {
				or_smt += (*b).SMTlib() + " ";
			}
			or_smt += ")";
			return SMT_expr(or_smt);
	}
}

SMT_expr SMTlib::SMT_mk_and (std::vector<SMT_expr> args){
	switch (args.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			return SMT_expr(args[0].SMTlib());
			break;
		default:
			std::string or_smt;
			or_smt = "(and \n"; 

			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			for (; b != e; ++b) {
				or_smt += " " + (*b).SMTlib() + "\n";
			}
			or_smt += ")";
			return SMT_expr(or_smt);
	}
}

SMT_expr SMTlib::SMT_mk_eq (SMT_expr a1, SMT_expr a2){
	return SMT_expr("(= " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_diseq (SMT_expr a1, SMT_expr a2){
	return SMT_expr("(not (= " + a1.SMTlib() + " " + a2.SMTlib() + "))");
}

SMT_expr SMTlib::SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e){
	return SMT_expr("(ite "
		+ c.SMTlib() + " "
		+ t.SMTlib() + " "
		+ e.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_not (SMT_expr a){
	return SMT_expr("(not " + a.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_num (int n){
	std::ostringstream oss;
        //std::cerr << "const " << n << std::endl;
        if (n == INT_MIN)
                oss << "(- 2147483648)";
	else if (n < 0)
		oss << "(- " << -n << ")";
	else
		oss << n;
	return SMT_expr(oss.str());
}

SMT_expr SMTlib::SMT_mk_num_mpq (mpq_t mpq) {
	SMT_expr res;
	char * charmpq;
	char * cnum;
	char * cden;
	if (mpq_sgn(mpq) < 0) {
		mpq_t nmpq;
		mpq_init(nmpq);
		mpq_neg(nmpq,mpq);
		cnum = mpz_get_str(NULL,10,mpq_numref(nmpq));
		cden = mpz_get_str(NULL,10,mpq_denref(nmpq));
		std::string snum(cnum);
		std::string sden(cden);
		if (sden.compare("1")) {
			res = SMT_expr("(- (/ " + snum + " " + sden + "))");
		} else {
			res = SMT_expr("(- " + snum + ")");
		}
		mpq_clear(nmpq);
	} else {
		cnum = mpz_get_str(NULL,10,mpq_numref(mpq));
		cden = mpz_get_str(NULL,10,mpq_denref(mpq));
		std::string snum(cnum);
		std::string sden(cden);
		if (sden.compare("1")) {
			res = SMT_expr("(/ " + snum + " " + sden + ")");
		} else {
			res = SMT_expr(snum);
		}
	}
	free(cnum);
	free(cden);
	return res;
}

std::string num_to_string(std::string num,int exponent) {
	std::ostringstream oss;
	switch (exponent) {
		case 0:
			oss << "0." << num;
			break;
		default:
			std::string r;
			if (exponent > 0) {
				int k;
				for (k = 0; k < exponent; k++) {
					if (k >= num.size())
						oss << "0";
					else
						oss << num[k];
				}
				oss << ".";
				if (k >= num.size())
					oss << "0";
				else
					for (; k < num.size(); k++) {
						oss << num[k];
					}
			} else {
				r = "0.";
				for (int i = -exponent; i > 1; i--) {
					r.append("0");
				}
				r.append("1");
				oss << "(* 0." << num << " " << r << ")";
			}
	}
	return oss.str();
}

SMT_expr SMTlib::SMT_mk_real (double x) {
	std::ostringstream oss;
	double intpart;
	bool is_neg = false;
	bool is_zero = false;

	mpf_t f;
	mpf_init(f);
	mpf_set_d(f,x);
	switch (mpf_sgn(f)) {
		case 0:
			is_zero = true;
			break;
		case -1:
			is_neg = true;
			mpf_t fneg;
			mpf_init(fneg);
			mpf_abs(fneg,f);
			mpf_set(f,fneg);
			mpf_clear(fneg);
			break;
		default:
			break;
	}
	mp_exp_t expptr;
	size_t n_digits = 20;
	char * r = mpf_get_str(NULL,&expptr,10,n_digits,f);
	mpf_clear(f);
	std::string num(r);
	
	if (is_zero)
		oss << "0.0";
	else {
		if (is_neg) oss << "(- "; 
		oss << num_to_string(num,expptr);
		if (is_neg) oss << ")"; 
		
	}
	return SMT_expr(oss.str());
}

SMT_expr SMTlib::SMT_mk_sum (std::vector<SMT_expr> args){
	SMT_expr res;
	switch (args.size()) {
		case 0:
			return SMT_expr(" ");
		case 1:
			return SMT_expr(args[0].SMTlib());
		default:
			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			std::string r;
			r = "(+ "; 
			for (; b != e; ++b) {
				r += (*b).SMTlib() + " ";
			}
			r += ")";
			return SMT_expr(r);
	}
	return SMT_expr();
}

SMT_expr SMTlib::SMT_mk_sub (std::vector<SMT_expr> args){
	switch (args.size()) {
		case 0:
			return SMT_expr(" ");
		case 1:
			return SMT_expr(args[0].SMTlib());
		default:
			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			std::string r;
			r = "(- "; 
			for (; b != e; ++b) {
				r += (*b).SMTlib() + " ";
			}
			r += ")";
			return SMT_expr(r);
	}
	return SMT_expr();
}

SMT_expr SMTlib::SMT_mk_mul (std::vector<SMT_expr> args){
	switch (args.size()) {
		case 0:
			return SMT_expr(" ");
		case 1:
			return SMT_expr(args[0].SMTlib());
		default:
			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			std::string r;
			r = "(* "; 
			for (; b != e; ++b) {
				r += (*b).SMTlib() + " ";
			}
			r += ")";
			return SMT_expr(r);
	}
	return SMT_expr();
}

SMT_expr SMTlib::SMT_mk_sum (SMT_expr a1, SMT_expr a2) {
	return SMT_expr("(+ " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_sub (SMT_expr a1, SMT_expr a2) {
	return SMT_expr("(- " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_mul (SMT_expr a1, SMT_expr a2) {
	return SMT_expr("(* " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_div (SMT_expr a1, SMT_expr a2, bool integer) {
	std::string s;
	// the syntax in SMTlib 2 differs between integer and real division
	if (integer)
		s = "(div ";
	else
		s = "(/ ";
	s += a1.SMTlib() + " " + a2.SMTlib() + ")";
	return SMT_expr(s);
}

SMT_expr SMTlib::SMT_mk_rem (SMT_expr a1, SMT_expr a2) {
	return SMT_expr("(mod " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_xor (SMT_expr a1, SMT_expr a2) {
	return SMT_expr("(xor " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_lt (SMT_expr a1, SMT_expr a2){
	return SMT_expr("(< " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_le (SMT_expr a1, SMT_expr a2){
	return SMT_expr("(<= " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_gt (SMT_expr a1, SMT_expr a2){
	return SMT_expr("(> " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_ge (SMT_expr a1, SMT_expr a2){
	return SMT_expr("(>= " + a1.SMTlib() + " " + a2.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_int2real(SMT_expr a) {
	return SMT_expr("(to_real " + a.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_real2int(SMT_expr a) {
	return SMT_expr("(to_int " + a.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_is_int(SMT_expr a) {
	return SMT_expr("(is_int " + a.SMTlib() + ")");
}

SMT_expr SMTlib::SMT_mk_int0() {
	return SMT_expr("0");
}

SMT_expr SMTlib::SMT_mk_real0() {
	return SMT_expr("0.0");
}

#if SMT_SUPPORTS_DIVIDES
// WORKS ONLY FOR CONSTANT a2
SMT_expr SMTlib::SMT_mk_divides (SMT_expr a1, SMT_expr a2) {
	return SMT_expr("((_ divisible " +  a1.SMTlib() + ") " +  a2.SMTlib() + ")");
}
#endif

void SMTlib::SMT_print(SMT_expr a){
	std::map<std::string,struct definedvars>::iterator it = vars.begin(), et = vars.end();
	for (; it != et; it++) {
		if ((*it).second.stack_level <= stack_level) {
			*Out << (*it).second.declaration;
		}
	}
	*Out << "(assert\n";
	*Out << a.SMTlib() << "\n";
	*Out << "); end of rho formula\n";
}

void SMTlib::SMT_assert(SMT_expr a){
	std::string assert_stmt;
	assert_stmt = "(assert " + a.SMTlib() + ")\n";
	PDEBUG(
			*Out << "\n\n" << assert_stmt << "\n\n";
		 );
	pwrite(assert_stmt);
}

int SMTlib::SMT_check(SMT_expr a, std::set<std::string> * true_booleans){
	int ret;
	std::string check_stmt;
	check_stmt = "(assert " + a.SMTlib() + ")\n";
	if (getSMTSolver() == Z3_QFNRA) {
		check_stmt += "(check-sat-using qfnra)\n";
	} else {
		check_stmt += "(check-sat)\n";
	}
	PDEBUG(
			*Out << "\n\n" << check_stmt << "\n\n";
		 );
	pwrite(check_stmt);

	ret = pread();
	if (ret == 1) {
		// SAT
		pwrite("(get-model)\n");
		pread();
		true_booleans->clear();
		true_booleans->insert(model.begin(),model.end());
	}
	if (ret == 0) {
		// UNSAT
		//pwrite("(get-unsat-core)\n");
		//pread();
	}
	return ret;
}

void SMTlib::push_context() {
	pwrite("(push 1)\n");
	stack_level++;
}

void SMTlib::pop_context() {
	pwrite("(pop 1)\n");
	stack_level--;

	std::map<std::string,struct definedvars> tmpvars;
	std::map<std::string,struct definedvars>::iterator it = vars.begin(), et = vars.end();
	for (; it != et; it++) {
		if ((*it).second.stack_level <= stack_level) {
			tmpvars.insert(*it);
		}
	}
	vars.clear();
	vars.insert(tmpvars.begin(), tmpvars.end());
}

bool SMTlib::interrupt() {
  kill(solver_pid, SIGINT);
  return true;
}
