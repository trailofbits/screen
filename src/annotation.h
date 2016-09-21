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

namespace {

  // @brief A generally unprocessed LLVM instruction span. Used to mark code
  // areas of interest.
  struct Region {
    Region() :start(nullptr) ,end(nullptr) { }

    const Instruction *start;
    const Instruction *end;
  };

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

  // @brief Possible annotation types. This might grow once we add support for
  // more complex annotations.
  enum AnnotationType {
    kAnnotationStart,
    kAnnotationEnd,
    kInvalidAnnotation
  };

  bool endsWith(std::string const &str, std::string const &end) {
    if (str.length() >= end.length()) {
      return (0 == str.compare(str.length() - end.length(), end.length(), end));
    } else {
      return false;
    }
  }

  bool addAnnotations(Module &M, std::string file,
      std::vector<const Function *> externFs,
      std::vector<std::tuple<std::string, const Instruction *, const Instruction *>> externRs) {
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
            externFs.push_back(F);
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

      std::remove_reference<decltype(externRs[0])>::type namedRange;
      std::get<0>(namedRange) = name;
      std::get<1>(namedRange) = start;
      std::get<2>(namedRange) = end;
      outs() << "Adding " << name << "\n";
      externRs.push_back(namedRange);
    }



    return true;
  }
}
