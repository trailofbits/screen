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
#include <llvm/IR/Instructions.h>

#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <set>

#include "traverse.h"
#include <sstream>

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

static cl::opt<bool> kDebugFlag("screen-debug", cl::Optional,
    cl::desc("Print extra debug information"), cl::init(false));

namespace {

struct ScreenPass : public ModulePass {
    std::ofstream out_fd;
    bool started;
    raw_ostream &O;

    ScreenPass()
    : ModulePass(ID)
    , started(false)
    , O(kDebugFlag ? outs() : nulls())
    {
      out_fd.open(kOutputName);
      out_fd << "[";
      out_fd.flush();
    }

    ~ScreenPass()
    {
        out_fd.close();
    }

    static char ID; 
    std::vector<std::vector<Function *>> cfg_paths_funcs;

    //TODO: intelligently pair annotations, check if same parent function  then
    //proxmity in code? (inst count of func to determine this)

    // @brief A generally unprocessed LLVM instruction span. Used to mark code
    // areas of interest.
    struct Region {
        Region() :start(nullptr) ,end(nullptr) { }

        const Instruction *start;
        const Instruction *end;
    };

    // @brief A BranchCond is a set of the compare isntruction and its' predicate and operands
    // CmpInst::FCMP_FALSE == 0
    struct BranchCond {
        BranchCond() :inst(nullptr) ,pred(CmpInst::FCMP_FALSE), ops() { }

        const Instruction *inst;
	CmpInst::Predicate pred;
	std::vector<Value*> ops;
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

	std::vector<BranchCond> BranchCondVec;
        std::vector<TraverseCfg::VisitedPath> callPaths;

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


    // TODO reason about bounds and call this when we need constraint information
    std::string reason_cmp_ops(Value *op){ 
	    std::string ret = "";
	    // reason about operand values, if there is a store it takes care of it
	    if (ConstantInt *CI = dyn_cast<ConstantInt>(op)){
		std::ostringstream os;
		os << CI->getSExtValue();
		ret = os.str();
	    }else if(dyn_cast<ConstantPointerNull>(op)){
	        ret = "NULL"; 
	    }else if(CallInst *call = dyn_cast<CallInst>(op)){
	        if(DebugLoc Loc = call->getDebugLoc()){
		    unsigned Line = Loc.getLine();
		    outs()<<"Got debug info, line numer at: "<< Line << "\n";
		}else{
		    outs()<<"Compile with -g -O0 for debug info @ critical vulnerabilities\n";
		}
		ret = "function_return_value:"+((call->getCalledFunction())->getName()).str(); 
	    }else{
		// value is a variable, trace uses
		int use_counter = 0;
		int phi_counter = 0;
		for (auto &U : op->uses())
		{
		    if(Instruction *temp = dyn_cast<Instruction>(U)){
			use_counter += 1;
			if(temp->getOpcode() == Instruction::PHI){
			    phi_counter += 1;
			}
		    }
		}
		if(phi_counter > 0 || use_counter > 0){
		    ret = "local_var";
		}else{
		    ret = "external_var";
		
		}
	    }
	    // this case for 100% user controlled variables
	    return ret;
    }

    void handle_cmp(CmpInst *cmpInst, std::vector<BranchCond> &BranchCondVec) {

	// a valid cmp must have 2 operands
	if (cmpInst->getNumOperands() >= 2) {
	    // get operands
	    Value *firstOperand = cmpInst->getOperand(0);
	    Value *secondOperand = cmpInst->getOperand(1);
	    // get predicate
	    CmpInst::Predicate p = cmpInst->getPredicate();
	    // store <inst> <pred> <op1> <op2>
	    BranchCond cmp_set;
	    cmp_set.inst = cmpInst;
	    cmp_set.pred = p;
	    cmp_set.ops.push_back(firstOperand);
	    cmp_set.ops.push_back(secondOperand);
	    BranchCondVec.push_back(cmp_set);
	}
    }
    // @brief Helper method that actually handles the accounting of instructions
    // This is called from function analysis and arbitrary span analysis.
    void surveyInstruction(const Instruction &I, RegionStats &stats, std::vector<BranchCond> &BranchCondVec)
    {
	if (isa<BranchInst>(I)) {
            stats.branches += 1;
        
	    // get Condition of the branch instruction
	    if(cast<BranchInst>(I).isConditional()){
	    	Value *condition = cast<BranchInst>(I).getCondition();
		if (!condition || !condition->hasOneUse())
		      return;

		if(llvm::CmpInst *CondInst = llvm::dyn_cast<llvm::CmpInst>(condition)){
			handle_cmp(CondInst, BranchCondVec);
		}

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
                surveyInstruction(I, info, info.BranchCondVec);
            });
            T.traverse(F);

            map[F] = info;
            return map;
        };

        auto funcStats = std::accumulate(Fs.begin(), Fs.end(), map, accFunc);

        for (auto r : funcStats) {
            auto f = r.first;
            auto span = r.second;

            O << " - func name: " << f->getName() << ", branches: "
              << span.branches << ", instructions " << span.instructions
              << "\n";

            dumpRegionStats(f->getName(), span, span.BranchCondVec);
        }

        return funcStats;
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
    void function_annotation_stats(Module &M){

        auto funcStats = getAnnotatedFunctionStats(M);

        O << "Func results: " << funcStats.size() << "\n";
        for (auto r : funcStats) {
            auto f = r.first;
            auto span = r.second;

            O << " - func name: " << f->getName() << ", branches: "
              << span.branches << ", instructions " << span.instructions
              << "\n";
        }
    }
    
    void dumpRegionStats(const std::string &name, const RegionStats &R, std::vector<BranchCond> cmps)
    {
        if (started)
            out_fd << ",";

        out_fd << "{ \"" << name << "\": {\n"
               << "     \"branches\": " << R.branches << ",\n"
               << "     \"instructions\": " << R.instructions << ",\n";

        if (!R.callPaths.empty()) {
               out_fd << "     \"cfg\": [\n";

            for (size_t p = 0; p < R.callPaths.size(); p++) {
                const auto &path = R.callPaths[p];

                out_fd << "              [";
                for (size_t i = 0; i < path.size(); i++) {
                    out_fd << "\"" << path[i].second->getName().str() << "\"";
                    if (i != path.size() - 1) {
                        out_fd << ", ";
                    }
                }

                out_fd << "]";
                if (p != R.callPaths.size()-1) {
                    out_fd << ", ";
                }
                out_fd << "\n";

            }
            out_fd << "       ]";
        }
        int count = 0;	
        for (auto &BranchCond : cmps) {
	    std::string predicate = "";
	    if (BranchCond.pred == 40){
	    	predicate = "signed_less_than";
	    }else if(BranchCond.pred == 32){
	    	predicate = "equals";
	    }else if(BranchCond.pred == 33){
	    	predicate = "not_equals";
	    }else if(BranchCond.pred == 34){
	    	predicate = "greater_than";
	    }else if(BranchCond.pred == 35){
	    	predicate = "greater_or_equal_than";
	    }else if(BranchCond.pred == 36){
	    	predicate = "less_than";
	    }else if(BranchCond.pred == 37){
	    	predicate = "less_or_equal_than";
	    }else if(BranchCond.pred == 38){
	    	predicate = "signed_greater_than";
	    }else if(BranchCond.pred == 39){
	    	predicate = "signed_greater_or_equal_than";
	    }else if(BranchCond.pred == 40){
	    	predicate = "signed_less_than";
	    }else if(BranchCond.pred == 41){
	    	predicate = "signed_less_or_equal_than";
	    }else{
	    	predicate = std::to_string(BranchCond.pred);
	    } 
            if(count == 0 && !R.callPaths.empty()){
	    	out_fd << ",\n";
	    }
	    if(count != 0){
	    	out_fd << ",\n";
	    }
	    
	    std::string reason1 = reason_cmp_ops((BranchCond.ops)[0]);
	    std::string reason2 = reason_cmp_ops((BranchCond.ops)[1]);
	    out_fd << "     \"cmp_inst_"<<count<<"\": [\"" << predicate << "\", \"" << reason1 << "\", \"" << reason2 << "\"]";
	    
	    count += 1;
	}
	if(count == 0){
		out_fd<<"\n";
	}
        out_fd << "}}\n";
        out_fd.flush();

        started = true;
    }

    void cfgReworkDemo(const Module &M)
    {
        Function *entry = M.getFunction(kSymbolName);
        if(!entry){
            errs() << "[E] no start symbol " << kSymbolName << "\n";
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
                    if (completed.find(name) != completed.end()) {
                        completed[name].callPaths.push_back(T.pathVisited(name));
                    } else {
                        RegionStats trackedSpan = inProgress[name];

                        trackedSpan.end = span.end;

                        trackedSpan.callPaths.push_back(T.pathVisited(name));

                        completed[name] = trackedSpan;

                        inProgress.erase(name);
                    }
                }
            }

            // Just a plain old instruction, add it to current counters.
            for (auto &info : inProgress) {
                surveyInstruction(I, info.second, info.second.BranchCondVec);
            }
        });
        T.traverse(entry);

        for (auto &stats : completed) {
          dumpRegionStats(stats.first, stats.second, stats.second.BranchCondVec);
        }

    }

    virtual bool runOnModule(Module &M) 
    {
        // runOnFunction is run on every function in each compilation unit
        // errs()<<"SCreening Paths of Program: "<<M.getName()<<"\n";    
        // errs()<<"\n[-] Using start symbol: "<<kSymbolName<<"\n";
        // errs()<<"\n\n[ STARTING MAIN ANALYSIS ]\n";


        // next stage, recover CFG, starting at main do a depth first search for annotation_start
        //
        cfgReworkDemo(M);

        function_annotation_stats(M);

        out_fd << "]\n";
        out_fd.flush();

        return true;

    }

};

}


char ScreenPass::ID = 0;
static RegisterPass<ScreenPass> X("screen", "screen", false, false);

