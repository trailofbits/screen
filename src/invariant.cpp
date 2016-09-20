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

    struct Region {
      Region() :start(nullptr) ,end(nullptr) { }

      const Instruction *start;
      const Instruction *end;
    };


    virtual const char *getPassName() const
    {
      return "screen-invariant";
    }

    std::string annotationToString(const IntrinsicInst &I) {
      GlobalVariable* annotationStrVar =
        dyn_cast<GlobalVariable>(I.getOperand(1)->stripPointerCasts());

      ConstantDataArray* annotationStrValArray =
        dyn_cast<ConstantDataArray> (annotationStrVar->getInitializer());

      StringRef annotationStrValCString =
        annotationStrValArray->getAsCString();

      return std::string {annotationStrValCString};
    }

    enum AnnotationType {
      kAnnotationStart,
      kAnnotationEnd,
      kInvalidAnnotation
    };

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
          // 4 = strlen("_end")
          auto name = postfix.substr(0, postfix.size() - 4);
          return { kAnnotationEnd, name};
        }

        return { kInvalidAnnotation, "" };
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


    bool endsWith(std::string const &str, std::string const &end) {
      if (str.length() >= end.length()) {
        return (0 == str.compare(str.length() - end.length(), end.length(), end));
      } else {
        return false;
      }
    }

    bool addAnnotations(Module &M, std::string file) {
      RangeParse parser(file);
      if (!parser.valid()) {
        return false;
      }
      DebugInfoFinder finder;
      finder.processModule(M);

      // Add annotations to functions
      for (auto &entry : parser.locations()) {
        if (entry.kind != RangeParse::kFunction)
          continue;

        for (auto sub : finder.subprograms()) {
          auto fileName = sub->getFile()->getFilename().str();
          auto funcName = sub->getName().str();

          if (endsWith(fileName, entry.file)) {
            if (funcName == entry.name) {
              Function *F = M.getFunction(sub->getName());
              externallyAnnotatedFunctions.push_back(F);
              entry.added = true;
            }
          }
        }
      }

      using InstructionPair = std::tuple<const Instruction *, const Instruction *>;
      std::map<std::string, InstructionPair> toAdd;

      // Collect instruction annotations
      for (auto &F : M.functions()) {
        for (auto &BB : F) {
          for (auto &I : BB) {
            if (DILocation *L = I.getDebugLoc()) {
              unsigned line = L->getLine();
              for (auto &entry : parser.locations()) {
                if (entry.added == true)
                  continue;

                if (entry.kind == RangeParse::kFunction)
                  continue;

                auto file = entry.file;
                auto lineno = entry.lineno;

                if (!endsWith(L->getFilename(), file))
                  continue;

                if (lineno != line)
                  continue;

                if (entry.kind == RangeParse::kStart) {
                  std::get<0>(toAdd[entry.name]) = &I;
                } else if (entry.kind == RangeParse::kEnd) {
                  std::get<1>(toAdd[entry.name]) = &I;
                }

                entry.added = true;
              }
            }
          }
        }
      }

      for (auto &tup : toAdd) {
        const Instruction *start, *end;
        std::tie(start, end) = std::get<1>(tup);
        auto name = std::get<0>(tup);
        if (start == nullptr || end == nullptr) {
          errs() << "Unmatched range definition for '" << name << "\n";
        };

        std::remove_reference<decltype(externallyAnnotatedRanges[0])>::type namedRange;
        std::get<0>(namedRange) = name;
        std::get<1>(namedRange) = start;
        std::get<2>(namedRange) = end;
        outs() << "Adding " << name << "\n";
        externallyAnnotatedRanges.push_back(namedRange);
      }



      return true;
    }

    virtual bool runOnModule(Module &M)
    {
      if (kOutputName == "") {
        errs() << "Must provide output file name\n";
        return false;
      }
      if (kAnnotationFile.getNumOccurrences() > 0) {
        if (addAnnotations(M, kAnnotationFile) == false) {
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
