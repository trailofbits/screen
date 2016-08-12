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

static cl::opt<bool> kDebugFlag("range-debug", cl::Optional,
    cl::desc("Print extra debug information"), cl::init(false));

namespace {
  struct RangePass : public ModulePass {
    
    static char ID;
    vector<RangeAnalysis *>RangeAnalyses;
    bool started;
    raw_ostream &O;
    
    RangePass() 
    : ModulePass(ID) 
    , started(false)
    , O(kDebugFlag ? outs() : nulls())
    {}

    virtual bool runOnModule(Module &M) {
    	//initialize analysis
	O << "Collecting variable value ranges from module: " << M.getName() << "\n";
    	for (Function &F: M){
		if (!F.isDeclaration()){
			O << "[+] Collecting function scope: " << F.getName() << "\n";
			RangeAnalyses.push_back(new RangeAnalysis(F));
		}
	}
	O <<"Printing results...\n";
    	for (unsigned int i = 0 ; i < RangeAnalyses.size() ; i++){
        	RangeAnalyses[i]->runWorklist();
        	O << "\nPrint CFG (with flow) : " << RangeAnalyses[i]->functionName<< "\n";
        	RangeAnalyses[i]->JSONCFG(O);
    	}
    	return false;
    }

  };
}

char RangePass::ID = 0;
static RegisterPass<RangePass> X("range_analysis", "Variable Range Analysis Pass", false, false);
