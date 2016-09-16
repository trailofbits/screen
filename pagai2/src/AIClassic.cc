/**
 * \file AIClassic.cc
 * \brief Implementation of the AIClassic Pass
 * \author Julien Henry
 */
#include "AIClassic.h"
#include "ModulePassWrapper.h"

using namespace llvm;

static RegisterPass<AIClassic> X("AIClassicPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIClassic, 0> > Y0("AIClassicPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIClassic, 1> > Y1("AIClassicPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIClassic::ID = 0;

const char * AIClassic::getPassName() const {
	return "AIClassic";
}
