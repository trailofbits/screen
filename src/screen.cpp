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
#include "llvm/Support/FileSystem.h"
#include "llvm/ADT/Statistic.h"
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/CommandLine.h>

#include <iostream>

using namespace llvm;

static cl::opt<std::string> kSymbolName("screen-start-symbol",
                                        cl::desc("Provide a symbol in the program to treat as the _start, usually main for most cases"),
                                        cl::Required);

static cl::opt<std::string> kOutputName("screen-output",
                                        cl::desc("Provide an output file for screen output"),
                                        cl::Required);

namespace {

    using sys::fs::OpenFlags;
    struct screen : public ModulePass {

        std::error_code start_sym_err;
        std::error_code out_fd_err;
    	raw_fd_ostream out_fd;
	std::string start_sym = kSymbolName;

        screen()
        : out_fd(kOutputName, out_fd_err, OpenFlags::F_RW)
        , ModulePass(ID)
        {

        }

        static char ID; 
        std::vector<std::vector<Function *>> cfg_paths_funcs;
        std::map<llvm::Function *,int*> fns_br;
        //TODO: intelligently pair annotations, check if same parent function, then proxmity in code? (inst count of func to determine this)
        llvm::IntrinsicInst* ann_start = NULL;
        llvm::IntrinsicInst* ann_end = NULL;

        virtual const char *getPassName() const 
        {
            return "screen_pass";
        }

        void simple_demo(Module &M, std::vector<Function*> mod_fns){
        
            // old way: the basic count branches and instructions when everything is in the same function, ignoring callinst, demo     
            int ann_count = 0;    
            int ann_count_brc = 0;    
            bool ann_start_count = false;
            for (Function &F: M)
            {
                // count for annotations covering an entire function
                if(std::find(mod_fns.begin(), mod_fns.end(), &F) != mod_fns.end()){
                    int count_brn = 0;
                    int count_inst = 0;
                    for (BasicBlock &B: F)
                    {
                        // Within each function, iterate over each basic block

                    
                        for (Instruction &I: B)
                        {
                            // Within each basic block, iterate over each instruction
                            count_inst += 1;    
                            if(isa<BranchInst>(I)){
                                count_brn += 1;    
                            }
                        }
                    }
                    int *n = new int[2];
                           n[0] = count_brn;
                    n[1] = count_inst;
                    fns_br[&F] = n;
                }

                //count in between instruction annotations
                for(BasicBlock &B: F){
                    for(Instruction &I: B){
                        if(IntrinsicInst* ann = dyn_cast<IntrinsicInst>(&I)){
                            if(ann_start != nullptr && ann->isIdenticalTo(ann_start)){
                                ann_start_count = true;
                            }
                            if(ann_end != nullptr && ann->isIdenticalTo(ann_end)){
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

            // out_fd << ann_count << "\n";

            outs()<<"\n[+] Dumping annotation path results...\n";
            out_fd <<"annotated_variable_pair"<<"\t"<<ann_count<<"\t"<<ann_count_brc<<"\n";
            outs()<<"In between annotated variables\n"<<"\t[ "<<ann_count<<" ] Instructions\n"<<"\t[ "<<ann_count_brc<<" ] Branch Instructions\n";
            outs()<<"\n[+] Dumping function path results...\n";
            for(std::map<llvm::Function *,int*>::iterator elem = fns_br.begin(); elem != fns_br.end(); ++elem)
            {
                out_fd <<elem->first->getName() << "\t" << elem->second[1] << "\t" << elem->second[0] << "\n";
                outs()<<"In annotated function: "<<(elem->first)->getName()<<"\n"<<"\t[ "<<(elem->second)[1]<<" ] Instructions\n"<<"\t[ "<<(elem->second)[0]<<" ] Branch Instructions\n";
            }
        
        
        }
        
        void follow_call(Function *f, std::vector<Function *>  &paths_funcs){
            paths_funcs.push_back(f);
	    Function::iterator bb = f->begin();
	    // gather called functions for that first BB
	    // TODO: reason about subsequent BB's of the called function (after entry BB) 
	    for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst){
		    if(CallInst *call = dyn_cast<CallInst>(inst)){
			// depth first recursion to collect function calls
			follow_call(call->getCalledFunction(), paths_funcs);    
		    }
		
	    }

            return;
        }

        void recurse_to_gather_paths(BasicBlock* bb, std::vector<std::vector<Function *>> &cfg_paths_funcs){
            for(BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst){
                if(CallInst *call = dyn_cast<CallInst>(inst)){
                    // depth first recursion to collect function calls inside 1 bb
                    follow_call(call->getCalledFunction(), cfg_paths_funcs[cfg_paths_funcs.size()-1]);    
                }
            
            }
            
            const TerminatorInst *TInst = bb->getTerminator();
            int succ_count = 0;
            std::vector<Function *> fork_save(cfg_paths_funcs[cfg_paths_funcs.size()-1]);
            for (unsigned I = 0, NSucc = TInst->getNumSuccessors(); I < NSucc; ++I) {
                BasicBlock *bb_succ = TInst->getSuccessor(I);
                // entry bb and returning bb are continuations of other paths, so dont fork at themt
                // fork cfg path, assume current path is last one added to the vector
                if(succ_count > 0){
                    std::vector<Function *> forked(fork_save);
                    cfg_paths_funcs.push_back(forked);
                }
                succ_count += 1;
                // now recurse through all the bb's successors
                recurse_to_gather_paths(bb_succ, cfg_paths_funcs);    
            }
            return;    
        
        }

        void dump_cfg(){
            outs()<<"[ CallInst CFG ]\nPulling out CallInst paths for each possible program execution path\n";
            // dump paths and their function calls
            for(int i = 0;i<cfg_paths_funcs.size();i++){
                outs()<<"\nPATH ["<<i<<"]\n";
                for(int j = 0;j<cfg_paths_funcs[i].size()-1;++j){
                    outs()<<(cfg_paths_funcs[i][j])->getName()<<"() -> ";
                
                }
                outs()<<(cfg_paths_funcs[i][ cfg_paths_funcs[i].size()-1])->getName()<<"()";
            
            }
        
        }

        std::vector<Function*> collectAnnotatedFunctions(const Module &M) {
            std::vector<Function*> annotatedFunctions;

            auto gAnnotationsPtr = M.getNamedGlobal("llvm.global.annotations");
            if (!gAnnotationsPtr)
              return annotatedFunctions;

            auto gAnnotations =
                cast<ConstantArray>(gAnnotationsPtr->getOperand(0));

            // Go through all annotations
            for (auto i = gAnnotations->op_begin(); i != gAnnotations->op_end();
                    i++) {
                auto annotation = cast<ConstantStruct>(*i);
  
                // Get the Function* that this attribute applies to
                auto function = dyn_cast<Function>(annotation->getOperand(0)->getOperand(0));
                if (!function)
                    continue;
  
                // If we do have a function, pull out its name and make sure
                // it's what we're looking for
                auto v = cast<GlobalVariable>(annotation->getOperand(1)->getOperand(0));
                auto label = cast<ConstantDataArray>(v->getOperand(0))->getAsCString();
  
                if (label == "screen_function_paths") { 
                    outs() << "Detected sensitive code region, tracking code " <<
                              "paths for function: "<<function->getName()<<"\n";
                    annotatedFunctions.push_back(function); 
                }
            }
            return annotatedFunctions;
            
        }
        virtual bool runOnModule(Module &M) 
        {
            // runOnFunction is run on every function in each compilation unit
            outs()<<"SCreening Paths of Program: "<<M.getName()<<"\n";    
	    outs()<<"\n[-] Using start symbol: "<<start_sym<<"\n";

	    // gather instruction annotations
            Function *F = M.getFunction("llvm.var.annotation");
            if(F){
                for (User* U : F->users()) {
                    if (IntrinsicInst* annotateCall = dyn_cast<IntrinsicInst>(U)) {
                        GlobalVariable* annotationStrVar = dyn_cast<GlobalVariable> (annotateCall->getOperand(1)->stripPointerCasts());
                        ConstantDataArray* annotationStrValArray = dyn_cast<ConstantDataArray> (annotationStrVar->getInitializer());
                        StringRef annotationStrValCString = annotationStrValArray->getAsCString();
                        if(annotationStrValCString == "screen_paths_start"){
                            outs()<<"[-] Found Starting Annotation\n";
                            Value* annotatedVar = dyn_cast<Value>(annotateCall->getOperand(0)->stripPointerCasts());
                            std::string inst_as_str = annotatedVar->getName().str().c_str();
                            outs()<<"\tInstStart = "<<inst_as_str<<"\n";
                            ann_start = annotateCall;
                        }
                        if(annotationStrValCString == "screen_paths_end"){
                            outs()<<"[-] Found Ending Annotation\n";
                            Value* annotatedVar = dyn_cast<Value>(annotateCall->getOperand(0)->stripPointerCasts());
                            std::string inst_as_str = annotatedVar->getName().str().c_str();
                            outs()<<"\tInstEnd = "<<inst_as_str<<"\n";
                            ann_end = annotateCall;
                        }
                    }
                }
            }    


	    outs()<<"\n\n[ STARTING MAIN ANALYSIS ]\n";
            // gather function annotations
            auto annotatedFunctions = collectAnnotatedFunctions(M);
    
            // DEMO: putting this in a function to save it and make it more readable
            simple_demo(M, annotatedFunctions);

            // next stage, recover CFG, starting at main do a depth first search for annotation_start
            Function *main = M.getFunction(start_sym);
	    if(!main){
		outs()<<"[ ERROR ] no start symbol "<<start_sym<<"\n";
	    	return false;
	    }
            // first function is always main
            std::vector<Function *> first_path;
            first_path.push_back(main);
            cfg_paths_funcs.push_back(first_path);
        
            int current_bb_num = 0;    
            Function::iterator bb = main->begin();
            recurse_to_gather_paths(&*bb, cfg_paths_funcs);

            dump_cfg();

            return true;
        }
    
    };

}


char screen::ID = 0;
static RegisterPass<screen> X("screen", "screen", false, false);

