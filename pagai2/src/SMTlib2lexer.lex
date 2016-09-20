%{
# include <cstdlib>
# include <cerrno>
# include <climits>
# include <string>

#include "Analyzer.h"
#include "SMTlib2driver.h"
#include "SMTlib2parser.hh"

typedef yy::SMTlib2parser::token token;

/* Work around an incompatibility in flex (at least versions
        2.5.31 through 2.5.33): it generates code that does
        not conform to C89.  See Debian bug 333231
        <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.  */
# undef yywrap
# define yywrap() 1

#define yyterminate() return token::END

%}
%option noyywrap nounput batch debug

blank       [\t ]
letter      [A-Za-z]
num			[0-9]
posinteger	{num}+
posreal		{posinteger}\.{posinteger}
negreal		-{blank}{posreal}
real		{negreal}|{posreal}
neginteger	-{blank}{posinteger}
integer		{neginteger}|{posinteger}
var			{letter}|{num}|_|\.|\%|!|-
varname		{var}+

string       \"(\\.|[^"])*\"

%{
# define YY_USER_ACTION  yylloc->columns (yyleng);
%}

%%

%{
  yylloc->step ();
%}
{blank}				{driver.log << " ";yylloc->step ();}
[\n]				{driver.log << "\n";yylloc->lines (yyleng); yylloc->step ();}

"("					{driver.log << "(";return(token::LEFTPAR);}
")"					{driver.log << ")";return(token::RIGHTPAR);}
 /* ")\n"	      yylloc->lines (yyleng); return(token::RIGHTPAR); */		
"/"					{driver.log << "/";return(token::DIVIDE);}		
"div"				{driver.log << "div";return(token::DIVIDE);}		
"*"					{driver.log << "*";return(token::MULTIPLY);}		
"+"					{driver.log << "+";return(token::ADD);}
"unknown"			{driver.log << "unknown";return(token::UNKNOWN);}
"unsat"				{driver.log << "unsat";return(token::UNSAT);}			
"sat"				{driver.log << "sat";return(token::SAT);}
"error"             {driver.log << "error";return(token::ERROR);}
"unsupported"       {driver.log << "unsupported";return(token::UNSUPPORTED);}
"success"           {driver.log << "success";return(token::SUCCESS);}
"true"				{driver.log << "true";return(token::TRUE);}		
"false"				{driver.log << "false";return(token::FALSE);}			
"model"				{driver.log << "model";return(token::MODEL);}			
"Int"				{driver.log << "Int";return(token::TYPE);}
"Real"				{driver.log << "Real";return(token::TYPE);}			
"Bool"				{driver.log << "Bool";return(token::BOOLTYPE);}			
"define-fun"		{driver.log << "define-fun";return(token::DEFINEFUN);}
{real}				{driver.log << "real";return(token::REALVALUE);}
{integer}			{driver.log << "integer";return(token::INTVALUE);}
"-"					{driver.log << "-";return(token::MINUS);}
"mod"				{driver.log << "mod";return(token::MODULO);}

{varname}			{
						yylval->sval=new std::string(yytext);
						driver.log << *yylval->sval;
						return(token::VARNAME);
					}
{string}			{
						yylval->sval=new std::string(yytext);
						driver.log << *yylval->sval;
						return(token::STRING);
					}


.					{
						std::string errormsg("invalid character :");
						errormsg.append(yytext);
						driver.error (*yylloc, errormsg);
					}

<<EOF>>				{yyterminate();}
%%

void SMTlib2driver::scan_begin () {
yy_flex_debug = trace_scanning;
	yyin = file;
}
     
void SMTlib2driver::scan_end () {
	fclose (yyin);
}
