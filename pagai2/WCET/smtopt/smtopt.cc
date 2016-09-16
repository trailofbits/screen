//============================================================================
// Name        : wcet.cpp
// Author      : Diego Caminha
// Version     :
// Copyright   : 
// Description :
//============================================================================

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <climits>
#include <sstream>
#include <time.h>


#include "z3.h"
#include "z3_api.h"

using namespace std;

// input options
bool verbose = false;
bool fullmodel = false;
int max_bound = 0;

Z3_context createSMT(const string &smtFormula){
	Z3_context ctx;
	Z3_config config = Z3_mk_config();
	Z3_set_param_value(config, "MODEL", "true");
	ctx = Z3_mk_context(config);
	//Z3_set_logic(ctx,"QF_LIA");


	//Z3_parse_smtlib_string(ctx, smtFormula.c_str(), 0, 0, 0, 0, 0, 0);
	//Z3_ast f = Z3_get_smtlib_formula(ctx, 0);
    Z3_set_ast_print_mode(ctx,Z3_PRINT_SMTLIB_FULL);

    {
	    Z3_context ctx;
	    ctx = Z3_mk_context(config);
	    Z3_ast f = Z3_parse_smtlib2_string(ctx, smtFormula.c_str(), 0, 0, 0, 0, 0, 0);
        ofstream smtfile;
        smtfile.open ("smtopt.smt");
        smtfile << Z3_benchmark_to_smtlib_string(ctx,"smtopt.smt", "unknown", "unknown", "", 0, NULL, f) << "\n";
        smtfile.close();
    }

	ifstream smtfile ("smtopt.smt");
    std::string smtlibFormula = "";
    std::string line;
	while (getline(smtfile, line))
		smtlibFormula += line + '\n';
	Z3_parse_smtlib_string(ctx, smtlibFormula.c_str(), 0, 0, 0, 0, 0, 0);
	Z3_ast f = Z3_get_smtlib_formula(ctx, 0);

	Z3_assert_cnstr(ctx, f);
    //std::cout << Z3_ast_to_string(ctx,f) << "\n";
	Z3_del_config(config);
	return ctx;
}

void printSolution(const Z3_context &ctx, const char *varName, const Z3_ast &var, const long long &solution){
	Z3_model m = 0;
	Z3_lbool result;
	char tmp[20];
	sprintf(tmp, "%lld", solution);

	Z3_assert_cnstr(ctx, Z3_mk_ge(ctx, var, Z3_mk_numeral(ctx, tmp, Z3_mk_int_sort(ctx))));
	if (fullmodel)
		result = Z3_check_and_get_model(ctx, &m);
	else
		result = Z3_check(ctx);
	if (m){
		printf("------------- MODEL -------------\n");
		printf("%s", Z3_model_to_string(ctx, m));
		printf("------------- MODEL -------------\n");
		Z3_del_model(ctx, m);
	}

	if (result == Z3_L_TRUE)
		printf("The maximum value of %s is %lld .\n", varName, solution);
	else if (result == Z3_L_UNDEF)
		printf("The probable (unknown result from Z3) maximum value of %s is %lld.\n", varName, solution);
	else
		printf("No solution was found.\n");
}

void printHelp(){
	printf("SMT Optimizer (version 0.91)\n");
	printf("Usage: smtopt [file] [var_to_maximize] [options]\n");
	printf("  -v\tverbose\n");
	printf("  -m\tfull model\n");
}

bool readProblem(int argc, char *argv[], string &smtFormula, char * &varMaximize){
	string line;
	
	if (argc < 3){
		printHelp();
		return false;
	}

	try{
		ifstream smtfile (argv[1]);
		varMaximize = argv[2];

		smtFormula.clear();
		while (getline(smtfile, line))
			smtFormula += line + '\n';
		
		for (int i = 3; i < argc; ++i){
			if (strcmp(argv[i], "-v") == 0)
				verbose = true;
			else if (strcmp(argv[i], "-m") == 0)
				fullmodel = true;
			else if (strcmp(argv[i], "-M") == 0){
                i++;
                istringstream buffer(argv[i]);
                buffer >> max_bound;
            }
			else if (strcmp(argv[i], "-h") == 0){
                std::cout << argv[i] << " is not a valid argument\n";
				printHelp();
				return false;
			}
			else{
				printf("Error: Unknown option %s", argv[i]);
				return false;
			}
		}
	}
	catch (exception& e){
		cout << "Error: " << e.what() << endl;
		return false;
	}
	return true;
}

// look for a variable (and its declaration) in the formula
bool recoverVarInFormula(const Z3_context &ctx, const char *varName, Z3_ast &var, Z3_func_decl &decl){
	unsigned num_decl = Z3_get_smtlib_num_decls(ctx);

	// linear search
	for (unsigned i = 0; i < num_decl; i++) {
		Z3_symbol symb = Z3_get_decl_name(ctx, Z3_get_smtlib_decl(ctx, i));
		Z3_string s = Z3_get_symbol_string(ctx,  symb);
		if (strcmp(s, varName) == 0){
			decl = Z3_get_smtlib_decl(ctx, i);
			var = Z3_mk_const(ctx, symb, Z3_mk_int_sort(ctx));
			return true;
		}
	}
	printf("Variable %s not found.\n", varName);
	return false;
}

// Initially unbounded binary search
long long binarySearch(const Z3_context &ctx, const Z3_ast &var, const Z3_func_decl &decl){
	long long mid, min = 0, max = 0;
	bool upperBoundFound = false;
	char tmp[20];
	do{
		Z3_model m;
		Z3_push(ctx);
		mid = (min + max + 1)/2;
		sprintf(tmp, "%lld", mid);
		Z3_assert_cnstr(ctx, Z3_mk_ge(ctx, var, Z3_mk_numeral(ctx, tmp, Z3_mk_int_sort(ctx))));
		if (verbose){
			printf("Testing %s >= %lld ... ", Z3_ast_to_string(ctx,  var), mid);
			fflush(stdout);
		}
		Z3_lbool ans = Z3_check_and_get_model(ctx, &m);
		if (ans == Z3_L_UNDEF){
			printf("UNDEF\n");
			return min;
		}
		if (ans == Z3_L_FALSE){
			max = mid-1;
			upperBoundFound = true;
			if (verbose) printf("UNSAT. New interval = [%lld, %lld].\n", min, max);
			Z3_pop(ctx, 1);
		}
		else{
			Z3_ast t;
			//min = mid;
			Z3_eval_func_decl(ctx, m, decl, &t);
			Z3_get_numeral_int64(ctx, t, &min);
			if (!upperBoundFound) {
				if (max_bound == 0) {
					max = min*4+1;
				} else {
					max = max_bound;
				}
			}
			if (max > INT_MAX){
				printf("Warning: The variable looks unbounded. (aborted)\n");
				return min;
			}
			if (verbose) printf("SAT (value found = %lld). New interval = [%lld, %lld].\n", min, min, max);
		}
	}while(min < max);
	return min;
}

int main(int argc, char *argv[]) {
	Z3_context ctx;
	Z3_ast varToMax;
	Z3_func_decl varToMaxDecl;

	string smtFormula;
	char *varMaximize = NULL;
	long long solution;

	if (readProblem(argc, argv, smtFormula, varMaximize) == false)
		exit(0);
	ctx = createSMT(smtFormula);
	if (recoverVarInFormula(ctx, varMaximize, varToMax, varToMaxDecl) == false){
		Z3_del_context(ctx);
		exit(0);
	}
	clock_t start = clock();
	solution = binarySearch(ctx, varToMax, varToMaxDecl);
	clock_t end = clock();
	double timing = (double)(end - start) / CLOCKS_PER_SEC;
	printSolution(ctx, varMaximize, varToMax, solution);
	printf("Computation time is %f",timing);

	Z3_del_context(ctx);
}
