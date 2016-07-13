#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <iostream>

using namespace llvm;

namespace {

	struct screen : public ModulePass {
		screen() : ModulePass(ID) { }

		static char ID; 
		std::vector<Function*> mod_fns;
		std::map<llvm::Function *,int> fns_br;

		virtual const char *getPassName() const 
		{
			return "screen_pass";
		}

		virtual bool runOnModule(Module &M) 
		{
			// runOnFunction is run on every function in each compilation unit
			outs()<<"SCreening Paths of Program: "<<M.getName()<<"\n";	
		
			// store instructions for analysis between
			//TODO: intelligently pair annotations, check if same parent function, then proxmity in code? (inst count of func to determine this)
			llvm::IntrinsicInst* ann_start = NULL;
			llvm::IntrinsicInst* ann_end = NULL;
			// gather instruction annotations
			Function *F = M.getFunction("llvm.var.annotation");
			if(F){
				for (User* U : F->users()) {
					if (IntrinsicInst* annotateCall = dyn_cast<IntrinsicInst>(U)) {
						GlobalVariable* annotationStrVar = dyn_cast<GlobalVariable> (annotateCall->getOperand(1)->stripPointerCasts());
						ConstantDataArray* annotationStrValArray = dyn_cast<ConstantDataArray> (annotationStrVar->getInitializer());
						StringRef annotationStrValCString = annotationStrValArray->getAsCString();
						if(annotationStrValCString == "screen_paths_start"){
							outs()<<"\t[-] Found Starting Annotation\n";
							Value* annotatedVar = dyn_cast<Value>(annotateCall->getOperand(0)->stripPointerCasts());
							std::string inst_as_str = annotatedVar->getName().str().c_str();
							outs()<<"\tInstStart = "<<inst_as_str<<"\n";
							ann_start = annotateCall;
						}
						if(annotationStrValCString == "screen_paths_end"){
							outs()<<"\t[-] Found Ending Annotation\n";
							Value* annotatedVar = dyn_cast<Value>(annotateCall->getOperand(0)->stripPointerCasts());
							std::string inst_as_str = annotatedVar->getName().str().c_str();
							outs()<<"\tInstEnd = "<<inst_as_str<<"\n";
							ann_end = annotateCall;
						}
					}
				}
			}	


			// gather function annotations
			auto global_annos = M.getNamedGlobal("llvm.global.annotations");
			if (global_annos) {
			  auto a = cast<ConstantArray>(global_annos->getOperand(0));
			  for (int i=0; i<a->getNumOperands(); i++) {
			    auto e = cast<ConstantStruct>(a->getOperand(i));

			    if (auto fn = dyn_cast<Function>(e->getOperand(0)->getOperand(0))) {
			      auto anno = cast<ConstantDataArray>(cast<GlobalVariable>(e->getOperand(1)->getOperand(0))->getOperand(0))->getAsCString();

			      if (anno == "screen_function_paths") { 
					outs()<<"\nDetected sensitive code region, tracking code paths for function: "<<fn->getName()<<"\n";
					// add to array or something: 
					mod_fns.push_back(fn); 
			    }
			  }
			}
			}	
		
			int ann_count = 0;	
			int ann_count_brc = 0;	
			bool ann_start_count = false;
			for (Function &F: M)
			{
				// count for annotations covering an entire function
				if(std::find(mod_fns.begin(), mod_fns.end(), &F) != mod_fns.end()){
					int count_brn = 0;
					for (BasicBlock &B: F)
					{
						// Within each function, iterate over each basic block

					
						for (Instruction &I: B)
						{
							// Within each basic block, iterate over each instruction
							if(isa<BranchInst>(I)){
								count_brn += 1;	
							}
						}
					}
					fns_br[&F] = count_brn;
				}

				//count in between instruction annotations
				for(BasicBlock &B: F){
					for(Instruction &I: B){
						if(IntrinsicInst* ann = dyn_cast<IntrinsicInst>(&I)){
							if(ann->isIdenticalTo(ann_start)){
								ann_start_count = true;
							}
							if(ann->isIdenticalTo(ann_end)){
								ann_start_count = false;
							}
						}
						if(ann_start_count){
							ann_count += 1;
							if(isa<BranchInst>(I)){
								ann_count_brc += 1;
							}
						}
					}
				}
			}
			outs()<<"\n[+] Dumping annotation path results...\n";
			outs()<<"In between annotated variables\n"<<"\t[ "<<ann_count<<" ] Instructions\n"<<"\t[ "<<ann_count_brc<<" ] Branch Instructions\n";
			outs()<<"\n[+] Dumping function path results...\n";
			for(std::map<llvm::Function *,int>::iterator elem = fns_br.begin(); elem != fns_br.end(); ++elem)
			{
				  outs() << "Function: "<<(elem->first)->getName() << " => Branches: " << elem->second << "\n";
			}
			return true;
		}
	
	};

}


char screen::ID = 0;
static RegisterPass<screen> X("screen", "screen", false, false);

