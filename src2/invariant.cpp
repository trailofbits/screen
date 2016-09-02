#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/DebugInfo.h" 
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Pass.h"
#include "RangeAnalysis.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include <string>
#include <vector>

using namespace llvm;
using namespace std;

static cl::opt<bool> kDebugFlag("invariant-debug", cl::Optional,
    cl::desc("Print extra debug information"), cl::init(false));

namespace {
  struct InvariantPass : public ModulePass {
    
    static char ID;
    bool started;
    raw_ostream &O;
    
    InvariantPass() 
    : ModulePass(ID) 
    , started(false)
    , O(kDebugFlag ? outs() : nulls())
    {}

    DenseMap<MDNode*, unsigned> _mdnMap; //Map for MDNodes.
    unsigned _mdnNext; 
    
    void createMetadataSlot(MDNode *N)
    {
	auto I = _mdnMap.find(N);
	if(I!=_mdnMap.end())
	{
	    return;
	}
	N->dump();
	//the map also stores the number of each metadata node. It is the same order as in the dumped bc file.
	unsigned DestSlot = _mdnNext++;
	_mdnMap[N] = DestSlot;
     
	for (unsigned i = 0, e = N->getNumOperands(); i!=e; ++i)
	{
	    if(MDNode *Op = dyn_cast_or_null<MDNode>(N->getOperand(i)))
	    {
		createMetadataSlot(Op);
	    }
	}
    }


    bool collectDbginfo(Module &M)
    {
	_mdnNext = 0;
	SmallVector<pair<unsigned, MDNode*>, 4> MDForInst;
	for (Function &F: M)
	{
	    for (BasicBlock &B: F)
	    {
		for (Instruction &I: B)
		{
		    I.getAllMetadata(MDForInst);
		    for(unsigned i = 0, e = MDForInst.size(); i!=e; ++i)
		    {
			createMetadataSlot(MDForInst[i].second);
		    }
	    	    MDForInst.clear();	    
		}
	    } 	 
	}

	return true;
    }

    virtual bool runOnModule(Module &M) {
    	//initialize analysis
	O << "Collecting variable invariants from module: " << M.getName() << "\n";
    	if(!collectDbginfo(M))
	{
		O << "No annotations found. Pagai error\n";
	}
	return false;
    }

  };
}

char InvariantPass::ID = 0;
static RegisterPass<InvariantPass> X("invariant_analysis", "Variable Invariant Analysis Pass", false, false);
