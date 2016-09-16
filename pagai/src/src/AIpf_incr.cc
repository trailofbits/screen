/**
 * \file AIpf_incr.cc
 * \brief Implementation of the AIpf_incr pass
 * \author Julien Henry
 */
#include <vector>
#include <list>

#include "AIpf_incr.h"
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

static RegisterPass<AIpf_incr> X("AIpf_incrPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIpf_incr, 0> > Y0("AIpf_incrPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIpf_incr, 1> > Y1("AIpf_incrPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIpf_incr::ID = 0;

const char * AIpf_incr::getPassName() const {
	return "AIpf_incr";
}

void AIpf_incr::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<Live>();
	AU.addRequired<ModulePassWrapper<AIClassic, 0> >();
}

void AIpf_incr::assert_properties(params P, Function * F) {
	assert_invariant(P,F);
}

void AIpf_incr::intersect_with_known_properties(Abstract * Xtemp, Node * n, params P) {
	Xtemp->meet(n->X_s[P]);
}
