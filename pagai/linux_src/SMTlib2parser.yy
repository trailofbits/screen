%skeleton "lalr1.cc"
%defines "SMTlib2parser.hh"

%define parser_class_name {SMTlib2parser}

%code requires {
     # include <string>
     class SMTlib2driver;
}
%parse-param { SMTlib2driver& driver }
%lex-param   { SMTlib2driver& driver }

%locations
%initial-action
{
  // Initialize the initial location.
  //@$.begin.filename = @$.end.filename = &driver.file;
};

%debug
%error-verbose

%{
# include <cstdlib>
# include <cerrno>
# include <climits>
# include <string>
#include <sstream>
#include "Analyzer.h"
#include "SMTlib2driver.h"
 
%}

%union
{
  int ival;
  bool bval;
  double fval;
  std::string *sval;
};

%token END 0
%token MODEL
%token UNSUPPORTED ERROR SUCCESS
%token DEFINEFUN
%token DIVIDE MINUS MODULO MULTIPLY ADD
%token<bval> TRUE FALSE
%token SAT UNSAT UNKNOWN
%token LEFTPAR RIGHTPAR
%token<bval> INTVALUE 
%token<bval> REALVALUE
%token<bval> TYPE
%token BOOLTYPE
%token<sval> VARNAME
%token<sval> STRING

%start Smt

%type<sval> FunName
%type<bval> FunValue
%type<bval> FunValue_List
%type<bval> FunType
%type<bval> BoolValue

%%

Smt:
        successes Smt0 { YYACCEPT;};

successes:
        /* */ { }
        | SUCCESS successes { }

Smt0:
	 SAT 					{driver.ans = SAT;}
	|UNSAT					{driver.ans = UNSAT;}
	|UNKNOWN 				{driver.ans = UNKNOWN;}
	|Model 					{driver.ans = SAT;}
	|Model_MathSAT 			{driver.ans = SAT;}
	|Error					{driver.ans = ERROR;}
	;

Error:
	 LEFTPAR ERROR STRING RIGHTPAR {*Out << *$3 << "\n"; delete $3;}
	 ;

Assoc_list:
       Assoc Assoc_list
       | /*empty*/
       ;

Assoc:
	LEFTPAR VARNAME FunValue RIGHTPAR	{
											if ($3) { driver.model.insert(*$2); }
											delete $2;
										}

Model:
	 LEFTPAR MODEL Model_list RIGHTPAR;

Model_MathSAT:
	LEFTPAR Assoc_list RIGHTPAR;

Model_list:
		  DefineFun Model_list
		  | /*empty*/
		  ;

DefineFun:
		 LEFTPAR DEFINEFUN FunName FunArgs FunType FunValue RIGHTPAR
							{
								if ($5 && $6) 
									driver.model.insert(*$3);
								// FunName is associated to a string*, we
								// have to delete it
								delete $3;
							}
		 ;

FunName:
	   VARNAME				{$$ = $1;}
	   ;

FunArgs:
	   LEFTPAR Argslist RIGHTPAR;

FunValue_List:
		FunValue FunValue_List {$$ = false;}
		| /*empty*/ {$$ = false;}
		;

FunValue:
		INTVALUE						{$$ = false;}
		| REALVALUE						{$$ = false;}
		| LEFTPAR DIVIDE FunValue FunValue RIGHTPAR {$$ = false;}
		| LEFTPAR MULTIPLY FunValue_List RIGHTPAR {$$ = false;}
		| LEFTPAR ADD FunValue_List RIGHTPAR {$$ = false;}
		| LEFTPAR MINUS FunValue_List RIGHTPAR{$$ = false;}
		| LEFTPAR MODULO FunValue FunValue RIGHTPAR {$$ = false;}
		| BoolValue						{$$ = $1;}
		| LEFTPAR FunValue RIGHTPAR		{$$ = $2;}
		;

BoolValue:
		 TRUE							{$$ = true;}
		 | FALSE						{$$ = false;}
		 ;

Argslist:
		/*empty*/
			| VARNAME Argslist			{
											// we delete the unused string*
											// associated to VARNAME
											delete $1;
										}
			;

FunType:
	   BOOLTYPE							{$$ = true;}
	   | TYPE							{$$ = false;}
	   ;
%%

void yy::SMTlib2parser::error (const yy::SMTlib2parser::location_type& l,
                               const std::string& m) {
       driver.error (l, m);
     }
