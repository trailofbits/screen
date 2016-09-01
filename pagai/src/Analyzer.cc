/**
 * \file Analyzer.cc
 * \brief Implementation of the Analyzer class (main class)
 * \author Julien Henry
 */
#include <string>
#include <iostream>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include "Execute.h" 
#include "Analyzer.h" 
#include "Debug.h" 

namespace po = boost::program_options; 
po::variables_map vm; 

SMTSolver Solver;
Techniques technique;
bool oflcheck;
bool bagnara_widening;
bool defined_main;
bool use_source_name;
bool printAll;
std::string main_function;
Apron_Manager_Type ap_manager[2];
bool Narrowing[2];
bool Threshold[2];
llvm::raw_ostream *Out;
llvm::raw_ostream *Dbg;
std::string filename;
std::string annotatedFilename;
std::string annotatedBCFilename;
int npass;
int timeout;
std::map<Techniques,int> Passes;
std::vector<enum Techniques> TechniquesToCompare;

SMTSolver getSMTSolver() {return Solver;}
Techniques getTechnique() {return technique;}
bool compareTechniques() {return vm.count("compare");}
bool compareDomain() {return vm.count("comparedomains");}
bool compareNarrowing() {return vm.count("comparenarrowing");}
bool onlyOutputsRho() {return vm.count("printformula");}
bool skipNonLinear() {return vm.count("skipnonlinear");}
bool useSourceName() {return use_source_name;}
bool printAllInvariants() {return printAll;}
bool check_overflow() {return oflcheck && vm.count("undefined-check");}
bool pointer_arithmetic() {return vm.count("pointers");}
bool inline_functions() {return !vm.count("noinline");}
bool brutal_unrolling() {return vm.count("loop-unroll");}
bool loop_rotate() {return !vm.count("no-loop-rotate");}
bool global2local() {return !vm.count("no-global2local");}
bool dumpll() {return vm.count("dump-ll");}
bool WCETSettings() {return vm.count("wcet");}
bool generateMetadata() {return annotatedBCFilename.size();}
bool InvariantAsMetadata() {return vm.count("output-bc-v2");}
std::string getAnnotatedBCFilename() {return annotatedBCFilename;}
void set_useSourceName(bool b) {use_source_name = (b && !vm.count("force-old-output"));}
enum outputs preferedOutput() {if (vm.count("force_old_output")) return LLVM_OUTPUT; else return C_OUTPUT;}
bool OutputAnnotatedFile() {return annotatedFilename.length() > 0;}
std::string getAnnotatedFilename() {return annotatedFilename;}
int getTimeout() {return timeout;}
bool hasTimeout() {return vm.count("timeout");}
std::string getFilename() {return vm["input"].as<std::string>();}
bool SVComp() {return vm.count("svcomp");}
Apron_Manager_Type getApronManager() {return ap_manager[0];}
Apron_Manager_Type getApronManager(int i) {return ap_manager[i];}
bool useNewNarrowing() {return vm.count("new-narrowing");}
bool useNewNarrowing(int i) {if (i==0) return vm.count("new-narrowing"); else return vm.count("new-narrowing2");}
bool useThreshold() {return Threshold[0];}
bool useThreshold(int i) {return Threshold[i];}
bool definedMain() {return defined_main;}
std::string getMain() {return main_function;}
bool quiet_mode() {return vm.count("quiet");} 
bool log_smt_into_file() {return vm.count("log_smt");}
bool optimizeBC() {return vm.count("optimize");}
bool InstCombining() {return vm.count("instcombining");}
std::vector<enum Techniques> * getComparedTechniques() {return &TechniquesToCompare;}

std::string TechniquesToString(Techniques t) {
	switch (t) {
		case LOOKAHEAD_WIDENING:
			return "LOOKAHEAD WIDENING";
		case PATH_FOCUSING: 
			return "PATH FOCUSING";
		case PATH_FOCUSING_INCR: 
			return "PATH FOCUSING INCR";
		case LW_WITH_PF:
			return "COMBINED";
		case COMBINED_INCR:
			return "COMBINED INCR";
		case SIMPLE:
			return "CLASSIC";
		case GUIDED:
			return "GUIDED";
		case LW_WITH_PF_DISJ:
			return "DISJUNCTIVE";
		default:
			abort();
	}
}


bool setApronManager(std::string d, int i) {
	
	if (!d.compare("box")) {
		ap_manager[i] = BOX;
	} else if (!d.compare("oct")) {
		ap_manager[i] = OCT;
	} else if (!d.compare("pk")) {
		ap_manager[i] = PK;
	} else if (!d.compare("pkeq")) {
		ap_manager[i] = PKEQ;
#ifdef OPT_OCT_ENABLED
        } else if (!d.compare("opt_oct")) {
	        ap_manager[i] = OPT_OCT;
#endif
#ifdef PPL_ENABLED
	} else if (!d.compare("ppl_poly_bagnara")) {
		ap_manager[i] = PPL_POLY_BAGNARA;
	} else if (!d.compare("ppl_poly")) {
		ap_manager[i] = PPL_POLY;
	} else if (!d.compare("ppl_grid")) {
		ap_manager[i] = PPL_GRID;
	} else if (!d.compare("pkgrid")) {
		ap_manager[i] = PKGRID;
#endif
	} else {
		std::cout << "Wrong parameter defining the abstract domain\n";
		return 1;
	}
	return 0;
}

std::string ApronManagerToString(Apron_Manager_Type D) {
	switch (D) {
		case BOX:
			return "BOX";
		case OCT:
			return "OCT";
		case PK:
			return "PK";
		case PKEQ:
			return "PKEQ";
#ifdef OPT_OCT_ENABLED
	        case OPT_OCT:
		        return "OPT_OCT";
#endif
#ifdef PPL_ENABLED
		case PPL_POLY:
			return "PPL_POLY";
		case PPL_POLY_BAGNARA:
			return "PPL_POLY_BAGNARA";
		case PPL_GRID:
			return "PPL_GRID";
		case PKGRID:
			return "PKGRID";
#endif
	}
}

enum Techniques TechniqueFromString(bool &error, std::string d) {
	error = false;
	if (!d.compare("lw")) {
		return LOOKAHEAD_WIDENING;
	} else if (!d.compare("pf")) {
		return PATH_FOCUSING;
	} else if (!d.compare("pf_incr")) {
		return PATH_FOCUSING_INCR;
	} else if (!d.compare("lw+pf")) {
		return LW_WITH_PF;
	} else if (!d.compare("s")) {
		return SIMPLE;
	} else if (!d.compare("g")) {
		return GUIDED;
	} else if (!d.compare("dis")) {
		return LW_WITH_PF_DISJ;
	} else if (!d.compare("incr")) {
		return COMBINED_INCR;
	}
	error = true;
	return SIMPLE;
}

bool setTechnique(std::string d) {
	bool error;
	enum Techniques r = TechniqueFromString(error,d);
	technique = r;
	if (error) {
		std::cout << "Wrong parameter defining the technique you want to use\n";
		return 1;
	}
	return 0;
}

bool setSolver(std::string d) {
	if (!d.compare("z3")) {
		Solver = Z3;
	} else if (!d.compare("z3_qfnra")) {
		Solver = Z3_QFNRA;
	} else if (!d.compare("mathsat")) {
		Solver = MATHSAT;
	} else if (!d.compare("smtinterpol")) {
		Solver = SMTINTERPOL;
	} else if (!d.compare("cvc3")) {
		Solver = CVC3;
	} else if (!d.compare("cvc4")) {
		Solver = CVC4;
#ifdef HAS_Z3 
	} else if (!d.compare("z3_api")) {
		Solver = API_Z3;
#endif
#ifdef HAS_YICES
	} else if (!d.compare("yices_api")) {
		Solver = API_YICES;
#endif
	} else {
		std::cout << "Wrong parameter defining the solver\n";
		return 1;
	}
	return 0;
}

bool setMain(std::string m) {
	main_function.assign(m);
	defined_main = true;
	return 0;
}

bool setTimeout(std::string d) {
	bool error = false;
	try {
		timeout = boost::lexical_cast< int >( d );
	} catch( const boost::bad_lexical_cast & ) {
		//unable to convert
		error = true;
	}
	try {
		double timeout_double = boost::lexical_cast< double >( d );
		TIMEOUT_LIMIT = sys::TimeValue(timeout_double);
	} catch( const boost::bad_lexical_cast & ) {
		//unable to convert
		error = true;
	}
	return error;
}

bool isMain(llvm::Function * F) {
	if (!definedMain()) return false;
	return (main_function.compare(F->getName().str()) == 0);
}



std::string solver_help() {

   std::string doc = 
	"\
	* z3 \n\
	* z3_qfnra\n\
	* mathsat\n\
	* smtinterpol\n\
	* cvc3\n\
	* cvc4\n";
#ifdef HAS_Z3 
   doc +=
	"\
	* z3_api\n";
#endif
#ifdef HAS_YICES
   doc +=
	"\
	* yices_api\n";
#endif
   return doc;
}

std::string domain_help() {
   std::string doc = 
"abstract domain\n\
	* box (Apron boxes)\n\
	* oct (Octagons)\n\
	* pk (NewPolka strict polyhedra)\n\
	* pkeq (NewPolka linear equalities)";
#ifdef PPL_ENABLED
   doc += 
	"\n\
	* ppl_poly (PPL strict polyhedra)\n \
	* ppl_poly_bagnara (ppl_poly + widening from Bagnara & al, SAS2003)\n \
	* ppl_grid (PPL grids)\n \
	* pkgrid (Polka strict polyhedra + PPL grids)";
#endif
   return doc.c_str();
}

std::string technique_help() {
   std::string doc = "technique\n\
	* lw (Lookahead Widening, SAS'06)\n\
	* g (Guided Static Analysis, SAS'07)\n\
	* pf (Path Focusing, SAS'11)\n\
	* lw+pf (SAS'12)\n\
	* s (simple abstract interpretation)\n\
	* dis (lw+pf, using disjunctive invariants)\n\
	* pf_incr\n\
	* incr";
   return doc;
}

int main(int argc, char* argv[]) {

    execute run;
    int o;
    bool help = false;
    bool bad_use = false;
	char* arg;
	bool debug = false;

#ifdef HAS_Z3 
	Solver = API_Z3;
#else
	Solver = Z3;
#endif
	ap_manager[0] = PK;
	ap_manager[1] = PK;
	Narrowing[0] = false;
	Narrowing[1] = false;
	Threshold[0] = false;
	Threshold[1] = false;
	technique = LW_WITH_PF;
	defined_main = false;
	use_source_name = true;
	printAll = false;
	n_totalpaths = 0;
	n_paths = 0;
	npass = 0;
	timeout = 0;
	filename="";
	annotatedFilename = "";
	annotatedBCFilename = "";
	oflcheck = true;
	std::vector<std::string> include_paths;
	std::vector<std::string> compare_list;


    po::options_description desc("Options"); 
    desc.add_options()
      ("input,i", po::value<std::string>()->required(), "input")
      ("help,h", "Print help messages") 
      ("include-path,I", po::value< std::vector<std::string> >(&include_paths), "include path (same as -I for clang)")
      ("solver,s", po::value<std::string>()->default_value("z3_api"), solver_help().c_str())
      ("domain,d", po::value<std::string>()->default_value("pk"), domain_help().c_str())
      ("technique,t", po::value<std::string>()->default_value("lw+pf"), technique_help().c_str())
      ("new-narrowing", "When the decreasing sequence fails (SAS12)") 
      ("main", po::value<std::string>(), "label name of the entry point") 
      ("undefined-check", "check for some undefined behaviors") 
      ("pointers", "pointers") 
      ("optimize", "optimize") 
      ("instcombining", "instcombining") 
      ("loop-unroll", "unroll loops") 
      ("no-loop-rotate", "do not rotate loops") 
      ("no-globals2locals", "do not turn global variables into local variables") 
      ("skipnonlinear", "ignore non linear arithmetic") 
      ("noinline", "do not inline functions") 
      ("output,o", po::value<std::string>()->default_value(""), "C output")
      ("output-bc,b", po::value<std::string>(&annotatedBCFilename), "LLVM IR output")
      ("output-bc-v2", po::value<std::string>(&annotatedBCFilename), "LLVM IR output (v2)")
      ("svcomp", "SV-Comp mode") 
      ("wcet", "wcet mode") 
      ("debug", "debug") 
      ("compare,c", po::value< std::vector<std::string> >(&compare_list), "compare list of techniques")
      ("comparedomains", "compare abstract domains") 
      ("printformula", "print SMT formula") 
      ("printall", "print all") 
      ("quiet", "quiet mode") 
      ("dump-ll", "dump analyzed ll file") 
      ("force-old-output", "use old output") 
      ("timeout", po::value<std::string>(), "timeout")
      ("log-smt", "write all the SMT requests into a log file") 
      //("annotated", po::value<std::string>(&annotatedFilename), "name of the annotated C file")
      ("domain2", po::value<std::string>(), "not for use")
      ("new-narrowing2", "not for use") 
      ;

    po::positional_options_description positionalOptions; 
    positionalOptions.add("input", 1); 

    try {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(positionalOptions).run(), vm); 
    	po::notify(vm);    
    } catch (std::exception& e) {

	    std::cout << "ERROR\n" << e.what() << "\n" << desc << "\n";
	    exit(1);
    }

    if (vm.count("help")) {
	    std::cout << desc << "\n";
        return 1;
    }

    setSolver(vm["solver"].as<std::string>());
    setTechnique(vm["technique"].as<std::string>());
    setApronManager(vm["domain"].as<std::string>(),0);
    if (vm.count("timeout")) setTimeout(vm["timeout"].as<std::string>());
    if (vm.count("main")) setMain(vm["main"].as<std::string>());
    if (vm.count("domain2")) setApronManager(vm["domain2"].as<std::string>(),1);

    if (vm.count("svcomp")) {
	setMain("main");
	oflcheck = false;
	technique = PATH_FOCUSING;
    }

    if (vm.count("wcet")) {
	// settings for computing WCET with counters
	printAll = true;
        oflcheck = false;
	technique = PATH_FOCUSING;
    }

    for (std::vector<std::string>::iterator it = compare_list.begin(), et = compare_list.end(); it != et; it++) {
	enum Techniques technique = TechniqueFromString(bad_use,*it);
	TechniquesToCompare.push_back(technique);
    }

    run.exec(vm["input"].as<std::string>(),vm["output"].as<std::string>(), include_paths);
	
	return 0;
}

