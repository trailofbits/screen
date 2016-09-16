/**
 * \file GenerateSMT.cc
 * \brief Implementation of the GenerateSMT class
 * \author Julien Henry
 */
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/Dominators.h"

#include "GenerateSMT.h"
#include "Analyzer.h"
#include "Live.h"
#include "Node.h"
#include "Debug.h"
#include "SMTpass.h"

using namespace std;

char GenerateSMT::ID = 0;
static RegisterPass<GenerateSMT>
X("GenerateSMT","SMT-lib formula generation pass",false,true);


const char * GenerateSMT::getPassName() const {
	return "SMT-Lib Generation Pass";
}


GenerateSMT::GenerateSMT() : ModulePass(ID) {

}

GenerateSMT::~GenerateSMT() {

}

void GenerateSMT::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<Live>();
	AU.addRequired<DominatorTreeWrapperPass>();
	AU.setPreservesAll();
}

bool GenerateSMT::runOnFunction(Function &F) {
  DT = &(getAnalysis<DominatorTreeWrapperPass>(F).getDomTree());

	LSMT->getRho(F);

	LSMT->man->SMT_print(LSMT->getRho(F));

	*Out << "\n\n-------\n\n";

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		BasicBlock * b = &*i;
		printBasicBlock(b);
	}
	return 0;
}

void GenerateSMT::printBasicBlock(BasicBlock* b) {

	int N = 0;

	for (BasicBlock::iterator i = b->begin(), e = b->end(); i != e; ++i) {
		N++;
	}
	//BasicBlock * dominator = DT->getNode(b)->getIDom()->getBlock();
	llvm::DomTreeNodeBase<llvm::BasicBlock>*dominator;
	if (pred_begin(b) != pred_end(b)) {
		dominator = DT->getNode(b)->getIDom();
	} else {
		dominator = NULL;
	}

	*Out << "BasicBlock " << SMTpass::getNodeName(b,false)  << ": " << N << " instruction(s)";
	if (dominator != NULL)
		*Out << ", Dominator = " << SMTpass::getNodeName(dominator->getBlock(),false);
	else
		*Out << ", Dominator = NULL";
	*Out << *b << "\n";

}

bool GenerateSMT::runOnModule(Module &M) {
	LSMT = SMTpass::getInstance();
	Function * F;

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = &*mIt;
		if (F->isDeclaration()) continue;
		runOnFunction(*F);
	}

	return 0;
}
