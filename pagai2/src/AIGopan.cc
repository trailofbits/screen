/**
 * \file AIGopan.cc
 * \brief Implementation of the AIGopan pass
 * \author Julien Henry
 */
#include "AIGopan.h"
#include "ModulePassWrapper.h"

using namespace llvm;

static RegisterPass<AIGopan> X("AIGopanPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIGopan, 0> > Y0("AIGopanPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIGopan, 1> > Y1("AIGopanPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIGopan::ID = 0;

const char * AIGopan::getPassName() const {
	return "AIGopan";
}
