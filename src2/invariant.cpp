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

    DenseMap<MDNode*, std::vector<MDNode*>> _mdnMap; //Map for MDNodes.
    unsigned _mdnNext; 
    
    void createMetadataSlot(MDNode *N, std::vector<MDNode*> &mdnVec)
    {
     
	for (unsigned i = 0, e = N->getNumOperands(); i!=e; ++i)
	{
	    if(MDNode *Op = dyn_cast_or_null<MDNode>(N->getOperand(i)))
	    {
	        //O << "MDNode Operands\n";
	    	if (std::find(mdnVec.begin(), mdnVec.end(), N) == mdnVec.end()) 
			    mdnVec.push_back(N);
		createMetadataSlot(Op, mdnVec);
	    }
	    else
	    {
		//N->dump();
	    	if (std::find(mdnVec.begin(), mdnVec.end(), N) == mdnVec.end()) 
			    mdnVec.push_back(N);
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
		    MDNode *node = I.getMetadata("screen.annotation");
		    std::string invariant = "";
		    if(node){
			    outs()<<"Found screen.annotation: ";
			    outs()<< node->getNumOperands() << "\n";
			    
			    MDString *str = dyn_cast<MDString>((node->getOperand(0)).get());
		    	    if(str){
				    invariant = str->getString().str();
			    }
		    	    outs()<< invariant << "\n";
		    }
		    continue;
		    for(unsigned i = 0, e = MDForInst.size(); i!=e; ++i)
		    {
			MDNode *meta = MDForInst[i].second;
			outs()<<MDForInst[i].first<<"\n";
			meta->dump();
			continue;
			auto I = _mdnMap.find(meta);
			if(I!=_mdnMap.end())
			{
				continue;
			}
			std::vector<MDNode*> mdnVec;
			createMetadataSlot(meta, mdnVec);
			_mdnMap[meta] = mdnVec;
			if (mdnVec.size() > 1)
			{
				meta->dump();
				for (MDNode* i : mdnVec) 
					i->dump();
			}
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
