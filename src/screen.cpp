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
#include <fstream>
#include <numeric>
#include <algorithm>
#include <set>

#include "traverse.h"

using namespace llvm;
using namespace screen;

static cl::opt<std::string> kSymbolName("screen-start-symbol",
    cl::desc("A function at which to start the analysis, main for most cases"),
    cl::Required);

static cl::opt<std::string> kOutputName("screen-output",
    cl::desc("Provide an output file for screen output"),
    cl::Required);

static cl::opt<std::string> kPrefix("screen-prefix", cl::Optional,
    cl::desc("The prefix for screen annotations"), cl::init("screen_"));

namespace {

struct ScreenPass : public ModulePass {
    std::error_code out_fd_err;
    std::ofstream out_fd;
    bool started;

    ScreenPass()
    : ModulePass(ID)
    , started(false)
    {
      out_fd.open(kOutputName);
      out_fd << "[";
    }

    ~ScreenPass()
    {
    }

    static char ID; 
    std::vector<std::vector<Function *>> cfg_paths_funcs;

    //TODO: intelligently pair annotations, check if same parent function, then
    //proxmity in code? (inst count of func to determine this)

    // @brief A generally unprocessed LLVM instruction span. Used to mark code
    // areas of interest.
    struct Region {
        Region() :start(nullptr) ,end(nullptr) { }

        const Instruction *start;
        const Instruction *end;
    };

    // @brief A Region that also includes data that we're tracking about it
    // 
    // A Region is supposed to mark code we're interested in for later analysis,
    // where as RegionStats is the structure used to collect data baout a region
    // as we're processing it.
    struct RegionStats : public Region {
        RegionStats() : branches(0), instructions(0) { }

        int branches;
        int instructions;

        std::vector<const Function *> callPath;

    };

    // @brief A map of Function* to the statistics about it
    using FuncStatsMap = std::map<const Function*, RegionStats>;

    // @brief A map of arbitrary regions to the statistics about them
    using RegionMap = std::map<std::string, Region>;

    using RegionStatsMap = std::map<std::string, RegionStats>;

    virtual const char *getPassName() const 
    {
        return "screen";
    }

    // @brief Helper method that actually handles the accounting of instructions
    // This is called from function analysis and arbitrary span analysis.
    void surveyInstruction(const Instruction &I, RegionStats &stats)
    {
        if (isa<BranchInst>(I)) {
            stats.branches += 1;
        
	    // get Condition of the branch instruction
	    if(cast<BranchInst>(I).isConditional()){
	    	Value *condition = cast<BranchInst>(I).getCondition();
		if (!condition || !condition->hasOneUse())
		      return;
	
		// condition->dump(); 
	    }

	}
        stats.instructions += 1;
    }

    // @brief Take an IntrinsicInstr representing an annotation and return its
    // string value.
    std::string annotationToString(const IntrinsicInst &I) {
        GlobalVariable* annotationStrVar =
            dyn_cast<GlobalVariable>(I.getOperand(1)->stripPointerCasts());

        ConstantDataArray* annotationStrValArray =
            dyn_cast<ConstantDataArray> (annotationStrVar->getInitializer());

        StringRef annotationStrValCString =
            annotationStrValArray->getAsCString();

        return std::string {annotationStrValCString};
    }

    // @brief Return some basic statistics in the form of RegionStats objects
    // about a collection of annotated functions in a module.
    FuncStatsMap getAnnotatedFunctionStats(const Module &M)
    {
        FuncStatsMap map;

        auto Fs = collectAnnotatedFunctions(M);
        
        auto accFunc = [this](FuncStatsMap &map, const Function *F) -> FuncStatsMap& {
            RegionStats info;

            TraverseLinearly T;
            T.setCallback([&info, this](const Instruction &I) {
                surveyInstruction(I, info);
            });
            T.traverse(F);

            map[F] = info;
            return map;
        };

        auto stats = std::accumulate(Fs.begin(), Fs.end(), map, accFunc);

        return stats;
    }

    // @brief Possible annotation types. This might grow once we add support for
    // more complex annotations.
    enum AnnotationType {
        kAnnotationStart,
        kAnnotationEnd,
        kInvalidAnnotation
    };

    // @brief Parse the annotation and return a pair indicating what type it is
    // and the exatract name.
    //
    // For instance:
    //   "screen_foo_start" -> { kAnnotationStart, "foo" }
    //   "screen_foo_bar_end" -> { kAnnotationEnd, "foo_bar" }
    //
    std::pair<AnnotationType, std::string>
    parseAnnotation(std::string annotation) {
        if (annotation.compare(0, kPrefix.length(), kPrefix) != 0) {
            return {kInvalidAnnotation, ""};
        }

        std::string start = "_start";
        std::string end = "_end";
        std::string prefix{kPrefix};

        auto a_ptr = annotation.begin();
        auto p_ptr = prefix.begin();

        for( ; *a_ptr == *p_ptr && a_ptr != annotation.end() &&
                                   p_ptr != prefix.end();
               a_ptr++, p_ptr++) {
        }


        std::string postfix(a_ptr, annotation.end());

        if (std::equal(start.rbegin(), start.rend(), annotation.rbegin())) {
            // 6 = strlen("_start")
            auto name = postfix.substr(0, postfix.size() - 6);
            return { kAnnotationStart, name};
        } else if (std::equal(end.rbegin(), end.rend(), annotation.rbegin())) {
            // 6 = strlen("_end")
            auto name = postfix.substr(0, postfix.size() - 4);
            return { kAnnotationEnd, name};
        }

        return { kInvalidAnnotation, "" };
    }

    // @brief Find all functions in a module M that have screen annotations
    // applied to them.
    std::vector<const Function*> collectAnnotatedFunctions(const Module &M)
    {
        std::vector<const Function*> annotatedFunctions;

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

            auto annotationString = std::string(label);
            if (annotationString.compare(0, kPrefix.length(), kPrefix) == 0) {
                // errs() << "Detected sensitive code region, tracking code " <<
                //           "paths for function: "<<function->getName()<<"\n";
                annotatedFunctions.push_back(function); 
            }
        }
        return annotatedFunctions;
        
    }

    // @brief Given a module, find all relevant regions we are interested in and
    // do the actual analysis.
    //
    // @return map of RegionStats results.
    RegionStatsMap getAnnotatedInstructionStats(const Module &M)
    {

        RegionStatsMap inProgress, completed;

        auto spans = collectAnnotatedSpans(M);

        for (const Function &F: M)
        {
            TraverseLinearly T;
            T.setCallback( [&] (const Instruction &I) {
                // First, check if we need to update our currently tracked-
                // regions (i.e. if we just entered or left a region.
                for (auto namedSpan : spans) {

                    auto name = namedSpan.first;
                    auto span = namedSpan.second;

                    // If we see a starting instruction, just tag it as started
                    if (I.isIdenticalTo(span.start)) {
                        inProgress[name].start = span.start;

                    // But once we see an ending instruction, we can now 
                    // consider this span finished and move the region being
                    // tracked to the completed map.
                    } else if (I.isIdenticalTo(span.end)) {
                        auto trackedSpan = inProgress[name];

                        trackedSpan.end = span.end;

                        completed[name] = trackedSpan;
                        inProgress.erase(name);

                    }
                }

                // Just a plain old instruction, add it to current counters.
                for (auto &info : inProgress) {
                    surveyInstruction(I, info.second);
                }
            });
            T.traverse(&F);
        }

        // Anything still in progress at this point is an unfinished block.
        // TODO: This will not be true once we use this code for CFG traversals
        for (auto &info : inProgress) {
            errs() << "[W] Unfinished block: " << info.first << "\n";
        }
        return completed;
    }

    // @brief Collect relevant annotations in a Module and return them as a 
    // map of named Region objects.
    //
    // This is only relevant for arbitrary regions, not tagged functions.
    RegionMap collectAnnotatedSpans(const Module &M)
    {
        RegionMap spanMap;

        Function *F = M.getFunction("llvm.var.annotation");
        if(!F)
            return spanMap;

        for (User* U : F->users()) {
            if (IntrinsicInst* annotateCall = dyn_cast<IntrinsicInst>(U)) {
                auto str = annotationToString(*annotateCall);

                AnnotationType type;
                std::string name;
                std::tie(type, name) = parseAnnotation(str);

                switch(type) {
                case kAnnotationStart:
                    spanMap[name].start = annotateCall;
                    break;
                case kAnnotationEnd:
                    spanMap[name].end = annotateCall;
                    break;
                default:
                    errs() << "[W] Unknown annotation (" << str << ")\n";
                }
            }
        }

        // Do some error checking
        for (auto pair : spanMap) {
            auto name = pair.first;
            auto span = pair.second;

            if (span.start == nullptr || span.end == nullptr) {
                errs() << "[!] Mismatched annotations (" << name << ")\n";
            }
        }

        return spanMap;
    }

    // @brief Entry point for the basic passes
    void simple_demo(Module &M){

        auto spanStats = getAnnotatedInstructionStats(M);
        auto funcStats = getAnnotatedFunctionStats(M);

        // errs() << "Span results: " << spanStats.size() << "\n";
        for (auto entry : spanStats) {
            auto name = entry.first;
            auto r = entry.second;

            // errs() << " - name: " << name << ", branches: " << r.branches
            //        << ", instructions " << r.instructions << "\n";

        }

        // errs() << "Func results: " << funcStats.size() << "\n";
        for (auto r : funcStats) {
            auto f = r.first;
            auto span = r.second;

            // errs() << " - func name: " << f->getName() << ", branches: "
            //        << span.branches << ", instructions " << span.instructions
            //        << "\n";

            dumpRegionStats(f->getName(), span);
        }

    }
    
    // Take a function, add it to |paths_func|, then iterate through all calls
    // that go from here
    void follow_call(Function *f, std::vector<Function *>  &paths_funcs){

        paths_funcs.push_back(f);
        // errs() << "Visiting: " << f->getName() << "\n";
       
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
        // errs()<<"[ CallInst CFG ]\nPulling out CallInst paths for each possible program execution path\n";
        // dump paths and their function calls
        for(size_t i = 0;i<cfg_paths_funcs.size();i++){
            // errs()<<"\nPATH ["<<i<<"]\n";
            for(size_t j = 0;j<cfg_paths_funcs[i].size()-1;++j){
                // errs()<<(cfg_paths_funcs[i][j])->getName()<<"() -> ";
            
            }
            // errs()<<(cfg_paths_funcs[i][ cfg_paths_funcs[i].size()-1])->getName()<<"()";
        
        }
    
    }

    void descendFromFunction(Function *F) {
        std::vector<Function *> first_path;
        first_path.push_back(F);

        cfg_paths_funcs.push_back(first_path);
    
        Function::iterator bb = F->begin();

        recurse_to_gather_paths(&*bb, cfg_paths_funcs);

        dump_cfg();
    }

    void dumpRegionStats(const std::string &name, const RegionStats &R)
    {
        if (started)
            out_fd << ",";

        out_fd << "{ \"" << name << "\": {\n"
               << "     \"branches\": " << R.branches << ",\n"
               << "     \"instructions\": " << R.instructions;

        auto path = R.callPath;
        if (!path.empty()) {
               out_fd << ",\n     \"cfg\": [";

            for (size_t i = 0; i < path.size(); i++) {
              if (!path[i]) {
                continue;
              }
              out_fd << "\"" << path[i]->getName().str() << "\"";
              if (i != path.size() - 1) {
                  out_fd << ", ";
              }
                
            }
            out_fd << "]\n";
        } else {
          out_fd << "\n";
        }

        out_fd << "}}\n";
        out_fd.flush();

        started = true;
    }

    void cfgReworkDemo(const Module &M)
    {

        Function *entry = M.getFunction(kSymbolName);
        if(!entry){
            errs() << "[ ERROR ] no start symbol "<<kSymbolName<<"\n";
            return;
        }

        RegionStatsMap inProgress, completed;

        auto spans = collectAnnotatedSpans(M);

        TraverseCfg T;
        T.setCallback([&](const Instruction &I) {
            // First, check if we need to update our currently tracked-
            // regions (i.e. if we just entered or left a region.
            for (auto namedSpan : spans) {

                auto name = namedSpan.first;
                auto span = namedSpan.second;

                // If we see a starting instruction, just tag it as started
                if (I.isIdenticalTo(span.start)) {
                    inProgress[name].start = span.start;
                    T.startPath(name);

                // But once we see an ending instruction, we can now 
                // consider this span finished and move the region being
                // tracked to the completed map.
                } else if (I.isIdenticalTo(span.end)) {
                    RegionStats trackedSpan = inProgress[name];

                    trackedSpan.end = span.end;
                    trackedSpan.callPath = T.pathVisited(name);

                    errs() << "Storing path visited: \n   ";
                    for (auto F : trackedSpan.callPath) {
                      (void) F;
                    }

                    errs() << "Iterated: \n   ";

                    for (auto F : trackedSpan.callPath) {
                      errs() << "{\n";
                      if (F) {
                        errs() << F << " -> ";
                      }
                      errs() << "}\n";
                    }
                    errs() << "\n";

                    completed[name] = trackedSpan;

                    inProgress.erase(name);
                }
            }

            // Just a plain old instruction, add it to current counters.
            for (auto &info : inProgress) {
                surveyInstruction(I, info.second);
            }
        });
        T.traverse(entry);

        for (auto &stats : completed) {
          // auto span = stats.second;

          // errs() << "Name: " << stats.first << "\n";

          dumpRegionStats(stats.first, stats.second);

          /* 
          auto path = span.callPath;
          for (size_t i = 0; i < path.size(); i++) {
            errs() << path[i]->getName();
            if (i != path.size() - 1) {
                errs() << " -> ";
            }
          }
          errs() << "\n";
          */
        }

    }

    virtual bool runOnModule(Module &M) 
    {
        // runOnFunction is run on every function in each compilation unit
        // errs()<<"SCreening Paths of Program: "<<M.getName()<<"\n";    
        // errs()<<"\n[-] Using start symbol: "<<kSymbolName<<"\n";
        // errs()<<"\n\n[ STARTING MAIN ANALYSIS ]\n";


        // next stage, recover CFG, starting at main do a depth first search for annotation_start

        cfgReworkDemo(M);

        simple_demo(M);

        out_fd << "]\n";
        out_fd.flush();

        return true;
    }

};

}


char ScreenPass::ID = 0;
static RegisterPass<ScreenPass> X("screen", "screen", false, false);

