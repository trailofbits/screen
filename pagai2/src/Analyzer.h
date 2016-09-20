/**
 * \file Analyzer.h
 * \brief Declaration of the Analyzer class and widely-used types and functions
 * \author Julien Henry
 */

/** \mainpage PAGAI
 *
 * \section intro_sec Usage
 *
 * PAGAI is a prototype of static analyser, working on top of the LLVM compiler infrastructure. 
 * It implements various state-of-the-art analysis techniques by abstract interpretation 
 * and decision procedures, and computes numerical invariants over a program expressed 
 * as an LLVM bitcode file. The tool is open source, and downloadable here : 
 * http://forge.imag.fr/projects/pagai/
 *
 * For a list of all available options :
 * \code
 * pagai -h or pagai --help \n 
 * \endcode
 */

#ifndef ANALYZER_H
#define ANALYZER_H

#include <set>
#include <map>
#include <vector>

#include "llvm/Support/FormattedStream.h"


#if LLVM_VERSION_MAJOR>3 || LLVM_VERSION_MAJOR==3 && LLVM_VERSION_MINOR>=6
#include "llvm/Analysis/CFG.h"
#else
#include "llvm/Analysis/CFG.h"
#endif

#include "llvm/IR/Function.h"


enum Apron_Manager_Type {
	BOX,
	OCT,
	PK,
#ifdef OPT_OCT_ENABLED
        OPT_OCT,
#endif
#ifdef PPL_ENABLED
	PPL_POLY,
	PPL_POLY_BAGNARA,
	PPL_GRID,
	PKGRID,
#endif
	PKEQ
};

enum Techniques {
	SIMPLE,
	LOOKAHEAD_WIDENING,
	GUIDED,
	PATH_FOCUSING,
	PATH_FOCUSING_INCR,
	LW_WITH_PF,
	COMBINED_INCR,
	LW_WITH_PF_DISJ,
};

enum SMTSolver {
	MATHSAT,
	Z3,
	Z3_QFNRA,
	SMTINTERPOL,
	CVC3, 
	CVC4, 
	API_Z3,
	API_YICES
};

enum outputs {
	LLVM_OUTPUT,
	C_OUTPUT
};

std::string TechniquesToString(Techniques t);
enum Techniques TechniqueFromString(bool &error, std::string d);

std::string ApronManagerToString(Apron_Manager_Type D);

SMTSolver getSMTSolver();

Techniques getTechnique();

bool compareTechniques();
std::vector<enum Techniques> * getComparedTechniques();

bool compareDomain();
bool compareNarrowing();

bool onlyOutputsRho();

// ignores ALL multiplications and divisions
bool skipNonLinear();

bool printAllInvariants();

// check for overflows on some arithmetical operations
bool check_overflow();

// handle pointers as if they were numerical integers
bool pointer_arithmetic();

// tune for SV-COMP
bool SVComp();

enum outputs preferedOutput();
bool useSourceName();
void set_useSourceName(bool b);

bool OutputAnnotatedFile();
std::string getAnnotatedFilename();
std::string getSourceFilename();

std::string getFilename();

int getTimeout();
bool hasTimeout();

bool definedMain();
std::string getMain();
bool isMain(llvm::Function * F);

bool quiet_mode();
bool log_smt_into_file();
bool generateMetadata();
std::string getAnnotatedBCFilename();
bool InvariantAsMetadata();

Apron_Manager_Type getApronManager();
Apron_Manager_Type getApronManager(int i);

bool useNewNarrowing();
bool useNewNarrowing(int i);

bool useThreshold();
bool useThreshold(int i);

bool optimizeBC();

bool WCETSettings();
bool inline_functions();
bool brutal_unrolling();
bool global2local();
bool loop_rotate();
bool InstCombining();
bool dumpll();

// stream used to write the output file
extern llvm::raw_ostream *Out;
// stream with debug printing and warning messages
extern llvm::raw_ostream *Dbg;

inline void changeColor(llvm::raw_ostream::Colors color, llvm::raw_ostream * oss = Dbg) {
		if (oss->has_colors()) oss->changeColor(color,true);
}
inline void resetColor(llvm::raw_ostream * oss = Dbg) {
		if (oss->has_colors()) oss->resetColor();
}

#endif
