#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "llvm/Analysis/CFG.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"

#include "Analyzer.h"
#include "Node.h"
#include "Pr.h"
#include "Debug.h"

class AnalysisPass {

	private:
		/** 
		 * \brief use for the SV-comp
		 * set to true if an error state has been found reachable
		 */
		bool assert_fail_found;

	public:
		/** 
		 * \brief pass unique identifier
		 */
		params passID;

		AnalysisPass() : assert_fail_found(false) {}

		/** 
		 * \brief process the sequence of positions (order by lines and columns) where an invariant has to be
		 * displayed, for each C files corresponding to the Module
		 */
		virtual void computeResultsPositions(
			Function * F,
			std::map<std::string,std::multimap<std::pair<int,int>,BasicBlock*> > * files 
		) = 0;

		/** 
		 * \brief returns true iff all the asserts in the Function are proved
		 * correct by the AIpass
		 */
		bool asserts_proved(Function * F);

		/** 
		 * \brief generates annotated C code for every C file used in this
		 * bitcode
		 */
		void generateAnnotatedFiles(Module * M, bool outputfile);
		void generateAnnotatedCode(llvm::raw_ostream * oss, std::string filename, std::multimap<std::pair<int,int>,BasicBlock*> * positions);

		/** 
		 * \brief inserts pagai invariants into the LLVM Module
		 */
		virtual void InstrumentLLVMBitcode(Function * F) = 0;

		/** 
		 * \brief print an invariant on oss, with an optional padding
		 */
		virtual void printInvariant(BasicBlock * b, std::string left, llvm::raw_ostream * oss) = 0;

		/** 
		 * \brief print the invariant the appropriate way 
		 * This method is typically called after the analysis of a function is finished
		 */
		void printResult(Function * F);

		/** 
		 * \brief print the invariant in LLVM IR style
		 */
		void printResult_oldoutput(Function * F);

		std::string getUndefinedBehaviourMessage(BasicBlock * b);

};
#endif
