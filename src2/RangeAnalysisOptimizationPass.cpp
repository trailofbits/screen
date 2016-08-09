
//
//  RangeAnalysisOptimizationPass.cpp
//  
//
//  Created by Costas Zarifis on 22/05/2014.
//
//
#include "llvm/Pass.h"
#include "Variable.h"
#include "RangeAnalysis.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include <string>
#include <vector>

using namespace llvm;
using namespace std;

/*
 * BUGS FOUND AND FIXED :
 * 	-	Avoid static members for the passes. This generates a linking error when making the shared object.
 * 	-	raw_ostream& object does not like to be fed a std::endl symbol. Prefer to user "\n".
 */

namespace {
  struct RangeAnalysisOptimizationPass : public FunctionPass {
    static char ID;
    vector<RangeAnalysis *>RangeAnalyses;
    RangeAnalysisOptimizationPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
    	//initialize analysis
	outs()<<"Running range analysis on function... "<<F.getName()<<"\n";
    	RangeAnalyses.push_back(new RangeAnalysis(F));
    	return false;
    }

    //The dummy optimization does not modify the code, but performs various analyses and outputs their result here
    void print(raw_ostream &OS, const Module*) const {

    	//The pure static analysis. Functional testing
    	for (unsigned int i = 0 ; i < RangeAnalyses.size() ; i++){
        	RangeAnalyses[i]->runWorklist();
        	OS << "\nPrint CFG (with flow) : " << "\n";
        	RangeAnalyses[i]->JSONCFG(OS);
    	}

  	}
  };
}

char RangeAnalysisOptimizationPass::ID = 0;
static RegisterPass<RangeAnalysisOptimizationPass> X("range_analysis", "Range Analysis Per Function Pass", false, false);
