/**
 * \file SMTlib2driver.cc
 * \brief Implementation of the SMTlib2driver class
 * \author Julien Henry
 */
#include "Analyzer.h"
#include "SMTlib2driver.h"
#include "SMTlib2parser.hh"

SMTlib2driver::SMTlib2driver() : trace_scanning (false), trace_parsing (false) {
}

SMTlib2driver::~SMTlib2driver() {
}

int SMTlib2driver::parse (FILE* f) {
	file = f;
	scan_begin ();
	yy::SMTlib2parser parser (*this);
	parser.set_debug_level (trace_parsing);
	int res = parser.parse ();
	// scan_end ();
	return 1;
}

void SMTlib2driver::error (const yy::location& l, const std::string& m) {
	std::cerr << l << ": " << m << std::endl;
	std::cerr << "log:\n" << log.str() << "\n";
}
     
void SMTlib2driver::error (const std::string& m) {
	std::cerr << m << std::endl;
}
