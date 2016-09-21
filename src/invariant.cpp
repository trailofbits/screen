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
#include "llvm/ADT/DenseMap.h"
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/Instructions.h>
#include "llvm/IR/InstIterator.h"

#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <set>

#include "range_parse.h"
#include "traverse.h"
#include "annotation.h"
#include <sstream>

using namespace llvm;
using namespace screen;

static cl::opt<std::string> kSymbolName("invariant-start-symbol",
    cl::desc("A function at which to start the analysis, main for most cases"),
    cl::init(""));

static cl::opt<std::string> kOutputName("invariant-output",
    cl::desc("Provide an output file for screen output"),
    cl::init(""));

static cl::opt<std::string> kPrefix("invariant-prefix", cl::Optional,
    cl::desc("The prefix for screen annotations"), cl::init("screen_"));

static cl::opt<bool> kDebugFlag("invariant-debug", cl::Optional,
    cl::desc("Print extra debug information"), cl::init(false));

static cl::opt<std::string> kAnnotationFile("invariant-annotations",
    cl::desc("Provide an external file that contains relevant code sections"),
    cl::init(""));


namespace {

  struct InvariantPass : public ModulePass {
    std::ofstream inv_out_fd;
    bool started;
    raw_ostream &O;
    std::vector<const Function*> externallyAnnotatedFunctions;
    std::vector<std::tuple<std::string, const Instruction *, const Instruction *>> externallyAnnotatedRanges;
    std::vector<std::vector<std::string>> store_metadata;

    InvariantPass()
      : ModulePass(ID)
        , started(false)
        , O(kDebugFlag ? outs() : nulls())
        {
          inv_out_fd.open(kOutputName);
          inv_out_fd << "[";
          inv_out_fd.flush();
        }

    ~InvariantPass()
    {
      inv_out_fd.close();
    }

    static char ID;

    virtual const char *getPassName() const
    {
      return "screen-invariant";
    }

    using RegionMap = std::map<std::string, Region>;

    virtual bool cfgReworkDemo(const Module &M) {
      if(kSymbolName == "") {
        errs() << "[E] must provide start symbol name\n";
        return false;
      }
      Function *entry = M.getFunction(kSymbolName);
      if(!entry){
        errs() << "[E] no start symbol " << kSymbolName << "\n";
        return false;
      }

      TraverseCfg T;
      T.setCallback([&](const Instruction &I) {
          MDNode *node = I.getMetadata("screen.annotation");
          std::string invariant = "";
          std::vector<std::string> save;
          bool add = true;
          if(node){
            if(const BasicBlock *bb = I.getParent()){
              if(const Function *ff = bb->getParent()){
              outs()<<ff->getName();
              save.push_back(ff->getName().str());
              }else{
              add = false;
              }
            }
            MDString *str = dyn_cast<MDString>((node->getOperand(0)).get());
            if(str){
              invariant = str->getString().str();
              save.push_back(invariant);
            }
            if(invariant.length() < 2){
              add = false;
            }
            if(add){
              store_metadata.push_back(save);
            }
            outs()<< invariant << "\n";
          }
      });
      T.traverse(entry);
      return true;
    }

    virtual bool runOnModule(Module &M)
    {
      if (kOutputName == "") {
        errs() << "Must provide output file name\n";
        return false;
      }
      if (kAnnotationFile.getNumOccurrences() > 0) {
        if (addAnnotations(M, kAnnotationFile, externallyAnnotatedFunctions, externallyAnnotatedRanges) == false) {
          errs() << "Bad annotations file format\n";
          return false;
        }
      }

      O << "Collecting variable invariants from module: " << M.getName() << "\n";
      if(!cfgReworkDemo(M)) {
        O << "No annotations found. Pagai error\n";
      } else {
        int count = 0;
        for(auto const& value: store_metadata) {
          if(count != 0)
            inv_out_fd <<",\n";
          inv_out_fd << "{ \"" <<value[0]<<"-"<<value[1]<<" }";//<<value[2]<<"\" }\n";
          inv_out_fd.flush();
          count ++;
        }
        inv_out_fd << "]\n";
        inv_out_fd.flush();
        }

        return true;

    }
  };
}

char InvariantPass::ID = 0;
static RegisterPass<InvariantPass> X("invariant_analysis", "Variable Invariant Analysis Pass", false, false);
