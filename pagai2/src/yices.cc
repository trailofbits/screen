/**
 * \file yices.cc
 * \brief Implementation of the yices class
 * \author Julien Henry
 */

#if HAS_YICES
#include <cstddef>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <unistd.h>

#include <gmp.h>
#include "yices_c.h"

#include "yices.h"
#include "Analyzer.h"
#include "Debug.h"

using namespace llvm;

yices::yices() {
	const char* intname =	"int";
	const char* floatname ="float";
	ctx = yices_mk_context();
	int_type.i = yices_mk_type(ctx,const_cast<char*>(intname));
	float_type.i = yices_mk_type(ctx,const_cast<char*>(floatname));
	int0 = SMT_mk_num(0);
	real0 = SMT_mk_real(0.0);
}

yices::~yices() {
	yices_del_context (ctx);
}

SMT_expr yices::SMT_mk_true() {
	SMT_expr res;
	res.i = yices_mk_true (ctx);
	return res;
}

SMT_expr yices::SMT_mk_false() {
	SMT_expr res;
	res.i = yices_mk_false (ctx);
	return res;
}

SMT_var yices::SMT_mk_bool_var(std::string val) {
	char * cstr = new char [val.size()+1];
	strcpy (cstr, val.c_str());
	SMT_var res;
	res.i = yices_get_var_decl_from_name(ctx,cstr);
	if (res.i == NULL) {
		res.i = yices_mk_bool_var_decl(ctx,cstr);
	} 
	delete [] cstr;
	return res;
}

SMT_var yices::SMT_mk_var(std::string name,SMT_type type) {
	char * cstr = new char [name.size()+1];
	strcpy (cstr, name.c_str());
	SMT_var res;
	res.i = yices_get_var_decl_from_name(ctx,cstr);
	if (res.i == NULL) {
		res.i = yices_mk_var_decl(ctx,cstr,type.i);
	} 
	delete [] cstr;
	return res;
}

SMT_expr yices::SMT_mk_expr_from_bool_var(SMT_var var) {
	SMT_expr res;
	res.i = yices_mk_bool_var_from_decl (ctx,(yices_var_decl)var.i);
	return res;
}

SMT_expr yices::SMT_mk_expr_from_var(SMT_var var) {
	SMT_expr res;
	res.i = yices_mk_var_from_decl (ctx,(yices_var_decl)var.i);
	return res;
}

SMT_expr yices::SMT_mk_or (std::vector<SMT_expr> args) {
	std::vector<yices_expr> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		assert((*b).i != 0);
		arguments.push_back((yices_expr)(*b).i);
	}
	switch (arguments.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			return args[0];
			break;
		default:
			SMT_expr res;
			res.i = yices_mk_or(ctx,&arguments[0],arguments.size());
			return res;
	}
}

SMT_expr yices::SMT_mk_and (std::vector<SMT_expr> args) {
	std::vector<yices_expr> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		assert((*b).i != 0);
		arguments.push_back((yices_expr)(*b).i);
	}
	switch (arguments.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			return args[0];
			break;
		default:
			SMT_expr res;
			res.i = yices_mk_and(ctx,&arguments[0],arguments.size());
			return res;
	}
}

SMT_expr yices::SMT_mk_eq (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = yices_mk_eq(ctx,(yices_expr)a1.i,(yices_expr)a2.i);
	return res;
}

SMT_expr yices::SMT_mk_diseq (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = yices_mk_diseq(ctx,(yices_expr)a1.i,(yices_expr)a2.i);
	return res;
}

SMT_expr yices::SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e) {
	SMT_expr res;
	res.i = yices_mk_ite(ctx,(yices_expr)c.i,(yices_expr)t.i,(yices_expr)e.i);
	return res;
}

SMT_expr yices::SMT_mk_not (SMT_expr a) {
	SMT_expr res;
	res.i = yices_mk_not(ctx,(yices_expr)a.i);
	return res;
}

SMT_expr yices::SMT_mk_num (int n) {
	SMT_expr res;
	res.i = yices_mk_num(ctx,n);
	return res;
}
		
SMT_expr yices::SMT_mk_num_mpq (mpq_t mpq) {
	SMT_expr res;
	res.i = yices_mk_num_from_mpq(ctx,mpq);
	return res;
}

SMT_expr yices::SMT_mk_real (double x) {
	SMT_expr res;
	mpq_t val;
	mpq_init(val);
	mpq_set_d(val,x);
	res.i = yices_mk_num_from_mpq(ctx,val);
	return res;
	//std::ostringstream oss;
	//oss << x ;
	//char * c = const_cast<char*>(oss.str().c_str());
	//return yices_mk_num_from_string(ctx,c);
}

SMT_expr yices::SMT_mk_sum (std::vector<SMT_expr> args) {
	std::vector<yices_expr> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		assert((*b).i != 0);
		arguments.push_back((yices_expr)(*b).i);
	}
	switch (arguments.size()) {
		case 0:
			return int0;
			break;
		case 1:
			return args[0];
			break;
		default:
			SMT_expr res;
			res.i = yices_mk_sum(ctx,&arguments[0],arguments.size());
			return res;
	}
}

SMT_expr yices::SMT_mk_sub (std::vector<SMT_expr> args) {
	std::vector<yices_expr> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		assert((*b).i != 0);
		arguments.push_back((yices_expr)(*b).i);
	}
	switch (arguments.size()) {
		case 0:
			return int0;
			break;
		case 1:
			return args[0];
			break;
		default:
			SMT_expr res;
			res.i = yices_mk_sub(ctx,&arguments[0],arguments.size());
			return res;
	}
}

SMT_expr yices::SMT_mk_mul (std::vector<SMT_expr> args) {
	std::vector<yices_expr> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		assert((*b).i != 0);
		arguments.push_back((yices_expr)(*b).i);
	}
	switch (arguments.size()) {
		case 0:
			return int0;
			break;
		case 1:
			return args[0];
			break;
		default:
			SMT_expr res;
			res.i = yices_mk_mul(ctx,&arguments[0],arguments.size());
			return res;
	}
}

SMT_expr yices::SMT_mk_sum (SMT_expr a1, SMT_expr a2) {
	std::vector<yices_expr> arguments;
	arguments.push_back((yices_expr)a1.i);
	arguments.push_back((yices_expr)a2.i);
	SMT_expr res;
	res.i = yices_mk_sum(ctx,&arguments[0],2);
	return res;
}

SMT_expr yices::SMT_mk_sub (SMT_expr a1, SMT_expr a2) {
	std::vector<yices_expr> arguments;
	arguments.push_back((yices_expr)a1.i);
	arguments.push_back((yices_expr)a2.i);
	SMT_expr res;
	res.i = yices_mk_sub(ctx,&arguments[0],2);
	return res;
}

SMT_expr yices::SMT_mk_mul (SMT_expr a1, SMT_expr a2) {
	std::vector<yices_expr> arguments;
	arguments.push_back((yices_expr)a1.i);
	arguments.push_back((yices_expr)a2.i);
	SMT_expr res;
	res.i = yices_mk_mul(ctx,&arguments[0],2);
	return res;
}


SMT_expr yices::SMT_mk_div (SMT_expr a1, SMT_expr a2, bool integer) {
	return int0;
}

SMT_expr yices::SMT_mk_rem (SMT_expr a1, SMT_expr a2) {
	return int0;
}

SMT_expr yices::SMT_mk_xor (SMT_expr a1, SMT_expr a2) {
	return SMT_mk_true();
}

SMT_expr yices::SMT_mk_lt (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = yices_mk_lt(ctx,(yices_expr)a1.i,(yices_expr)a2.i);
	return res;
} 

SMT_expr yices::SMT_mk_le (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = yices_mk_le(ctx,(yices_expr)a1.i,(yices_expr)a2.i);
	return res;
}

SMT_expr yices::SMT_mk_gt (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = yices_mk_gt(ctx,(yices_expr)a1.i,(yices_expr)a2.i);
	return res;
}

SMT_expr yices::SMT_mk_ge (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = yices_mk_ge(ctx,(yices_expr)a1.i,(yices_expr)a2.i);
	return res;
}

SMT_expr yices::SMT_mk_int2real(SMT_expr a) {
  return SMT_mk_real0();
}

SMT_expr yices::SMT_mk_real2int(SMT_expr a) {
  return SMT_mk_int0();
}

SMT_expr yices::SMT_mk_is_int(SMT_expr a) {
  return SMT_mk_true();
}

SMT_expr yices::SMT_mk_int0() {
  return int0;
}

SMT_expr yices::SMT_mk_real0() {
  return real0;
}

void yices::SMT_print(SMT_expr a) {

	//Save position of current standard output
	fpos_t pos;
	fgetpos(stdout, &pos);
	int fd = dup(fileno(stdout));
	if (freopen("/tmp/yices_output.txt", "w", stdout) == NULL) return;
	yices_pp_expr ((yices_expr)a.i);
	//Flush stdout so any buffered messages are delivered
	fflush(stdout);
	//Close file and restore standard output to stdout - which should be the terminal
	dup2(fd, fileno(stdout));
	close(fd);
	clearerr(stdout);
	fsetpos(stdout, &pos);

	FILE * tmp = fopen ("/tmp/yices_output.txt" , "r");
	if (tmp == NULL) return;
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
	fclose(tmp);

	*Out << "\n";
}

void yices::SMT_assert(SMT_expr a){
	yices_assert(ctx,(yices_expr)a.i);
}

int yices::SMT_check(SMT_expr a, std::set<std::string> * true_booleans) {
	//yices_pp_expr ((yices_expr)a);
	yices_set_arith_only(1);
	yices_assert(ctx,(yices_expr)a.i);
	//*Out << "\n";
	lbool res = yices_check(ctx);
	if (res == l_undef) {
		*Out << "undef\n";
		return -1;
	}
	if (res == l_true) {
		PDEBUG(
		*Out << "sat\n";
		);
		yices_var_decl_iterator it = yices_create_var_decl_iterator(ctx);
		yices_model m              = yices_get_model(ctx);
		
		PDEBUG(
		//Save position of current standard output
		fpos_t pos;
		fgetpos(stdout, &pos);
		int fd = dup(fileno(stdout));
		if (freopen("/tmp/yices_output.txt", "w", stdout) != NULL) {
			yices_display_model(m);
			//Flush stdout so any buffered messages are delivered
			fflush(stdout);
			//Close file and restore standard output to stdout - which should be the terminal
			dup2(fd, fileno(stdout));
			close(fd);
			clearerr(stdout);
			fsetpos(stdout, &pos);

			FILE * tmp = fopen ("/tmp/yices_output.txt" , "r");
			if (tmp != NULL) {
				fseek(tmp,0,SEEK_SET);
				char c;
				while ((c = (char)fgetc(tmp))!= EOF)
					*Out << c;

				*Out << "\n";
				fclose(tmp);
			}
		}
		);
		while (yices_iterator_has_next(it)) {
			yices_var_decl d         = yices_iterator_next(it);
			//*Out <<  yices_get_var_decl_name(d) << " = ";
			std::string name (yices_get_var_decl_name(d));
			switch(yices_get_value(m, d)) {
				case l_true: 
					true_booleans->insert(name);
					//*Out << "true\n"; 
					break;
				case l_false: 
					//*Out << "false\n"; 
					break;
				case l_undef: 
					//*Out << "unknown\n"; 
					break;
			}
		}
		yices_del_iterator(it);
	} else {
		PDEBUG(
		*Out << "unsat\n";
		);
		return 0;
	}
	return 1;
}

void yices::push_context() {
	yices_push(ctx);
}

void yices::pop_context() {
	yices_pop(ctx);
}
#endif
