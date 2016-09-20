/**
 * \file AIopt_incr.cc
 * \brief Implementation of the AIopt_incr pass
 * \author Julien Henry
 */
#include <vector>
#include <sstream>
#include <list>

#include "AIopt_incr.h"
#include "AIClassic.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "Live.h"
#include "SMTpass.h"
#include "Pr.h"
#include "Debug.h"
#include "Analyzer.h"
#include "PathTree.h"
#include "ModulePassWrapper.h"

using namespace llvm;

static RegisterPass<AIopt_incr> X("AIOpt_incrPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIopt_incr, 0> > Y0("AIOpt_incrPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIopt_incr, 1> > Y1("AIOpt_incrPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIopt_incr::ID = 0;

const char * AIopt_incr::getPassName() const {
	return "AIopt_incr";
}

void AIopt_incr::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<Live>();
	AU.addRequired<ModulePassWrapper<AIClassic, 0> >();
}

void AIopt_incr::assert_properties(params P, Function * F) {
	assert_invariant(P,F);
}

void AIopt_incr::intersect_with_known_properties(Abstract * Xtemp, Node * n, params P) {
	Xtemp->meet(n->X_s[P]);
}
