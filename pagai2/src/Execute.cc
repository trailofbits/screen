/**
 * \file Execute.cc
 * \brief Implementation of the Execute class
 * \author Julien Henry
 */
#include <fstream>
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/IPO.h"

#include "AIpf.h"
#include "AIpf_incr.h"
#include "AIopt.h"
#include "AIopt_incr.h"
#include "AIGopan.h"
#include "AIClassic.h"
#include "AIdis.h"
#include "AIGuided.h"
#include "ModulePassWrapper.h"
#include "Node.h"
#include "Execute.h"
#include "Live.h"
#include "SMTpass.h"
#include "SMTlib.h"
#include "Compare.h"
#include "CompareDomain.h"
#include "CompareNarrowing.h"
#include "Analyzer.h"
#include "GenerateSMT.h"
#include "instrOverflow.h"
#include "globaltolocal.h"
#include "taginline.h"
#include "RemoveUndet.h"
#include "expandequalities.h"
#include "expandassume.h"
#include "NameAllValues.h"
#include "IdentifyLoops.h"

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Basic/Version.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/IR/Module.h"

#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Path.h"

#include <memory>

using namespace llvm;

static cl::opt<std::string>
DefaultDataLayout("default-data-layout", 
          cl::desc("data layout string to use if not specified by module"),
          cl::value_desc("layout-string"), cl::init(""));

bool is_Cfile(std::string InputFilename) {
	if (InputFilename.compare(InputFilename.size()-2,2,".c") == 0)
		return true;
	if (InputFilename.compare(InputFilename.size()-3,3,".cc") == 0)
		return true;
	if (InputFilename.compare(InputFilename.size()-4,4,".cpp") == 0)
		return true;
	return false;
}

// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// GetMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement GetMainExecutable
// without being given the address of a function in the main executable).
std::string GetExecutablePath(const char *Argv0) {
  // This just needs to be some symbol in the binary; C++ doesn't
  // allow taking the address of ::main however.
  void *MainAddr = (void*) (intptr_t) GetExecutablePath;
  return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}

std::string parse_conf() {
	std::ifstream conffile;
	std::map<std::string,std::string> conf;
	try {
		std::string c = GetExecutablePath("pagai") + ".conf" ;
		conffile.open(c.c_str());
		std::string key,value;
		while (conffile >> key >> value){
			conf[key] = value;
		}
		if (conf.count("ResourceDir"))
			return conf["ResourceDir"];

    	} catch (std::exception& e) {
		*Dbg << "error while parsing pagai.conf\n";
	}
	return "";
}

void execute::exec(std::string InputFilename, std::string OutputFilename, std::vector<std::string> IncludePaths) {

	raw_fd_ostream *FDOut = NULL;

	if (OutputFilename != "") {

		std::error_code error;
		FDOut = new raw_fd_ostream(OutputFilename.c_str(), error, sys::fs::F_None);
		if (error) {
		        errs() << error.message() << '\n';
			delete FDOut;
			return;
		}
		Out = new formatted_raw_ostream(*FDOut);
		// Make sure that the Output file gets unlinked from the disk if we get a
		// SIGINT
		//sys::RemoveFileOnSignal(sys::Path(OutputFilename));
	} else {
		Out = &llvm::outs();
		Out->SetUnbuffered();
	}
	//Dbg = &llvm::fdbgs();
	Dbg = &llvm::outs();

	*Dbg << "THIS IS DEBUG PRINTING\n";
	
	std::unique_ptr<llvm::Module> M;

	// Arguments to pass to the clang frontend
	std::vector<const char *> args;
	args.push_back(InputFilename.c_str());
	args.push_back("-g");
	if (check_overflow()) {
		//args.push_back("-ftrapv");
		args.push_back("-fsanitize=bool");
		args.push_back("-fsanitize=enum");
		args.push_back("-fsanitize=null");
		args.push_back("-fsanitize=signed-integer-overflow");
		//args.push_back("-fsanitize=unsigned-integer-overflow");
		args.push_back("-fsanitize=integer-divide-by-zero");
		args.push_back("-fsanitize=float-divide-by-zero");
		args.push_back("-fsanitize=float-cast-overflow");
		args.push_back("-fsanitize=array-bounds");
		args.push_back("-fsanitize=local-bounds");
		//args.push_back("-fsanitize=local-bounds");
	}
	
	args.push_back("-I");
	args.push_back("/usr/local/include");
	args.push_back("-I");
	args.push_back("/usr/include");
	for (std::vector<std::string>::iterator it = IncludePaths.begin(), et = IncludePaths.end(); it != et; it++) {
	   args.push_back("-I");
	   args.push_back(const_cast<const char*>(it->c_str()));
	}
	//args.push_back(IncludePaths.front().c_str());

	clang::EmitLLVMOnlyAction Act;
	 
	clang::DiagnosticOptions * Diagopt = new clang::DiagnosticOptions();
	Diagopt->ShowColors=1;
	Diagopt->ShowPresumedLoc=1;

	clang::DiagnosticIDs *  DiagID = new clang::DiagnosticIDs();
	clang::DiagnosticsEngine * Diags = new clang::DiagnosticsEngine(DiagID, Diagopt);
	clang::DiagnosticConsumer * client = new clang::TextDiagnosticPrinter(*Dbg,Diagopt);
	Diags->setClient(client);

	// Create the compiler invocation
	std::unique_ptr<clang::CompilerInvocation> CI(new clang::CompilerInvocation);
	clang::CompilerInvocation::CreateFromArgs(*CI, &args[0], &args[0] + args.size(), *Diags);
	
	clang::CompilerInstance Clang;
	// equivalent to the -gcolumn-info command line option for clang
	CI->getCodeGenOpts().DebugColumnInfo = 1;
	Clang.setInvocation(CI.release());
	Clang.setDiagnostics(Diags);

	Clang.getHeaderSearchOpts().UseStandardSystemIncludes=1;
	Clang.getHeaderSearchOpts().UseStandardCXXIncludes=1;
	Clang.getHeaderSearchOpts().UseBuiltinIncludes=1;
	//Clang.getHeaderSearchOpts().UseLibcxx=1;
	
	std::string p = parse_conf();
	if (p.size() == 0) std::string p = LLVM_INSTALL_PATH;

	std::string sep;
	if (llvm::sys::path::is_separator('/'))
		sep = "/";
	else 
		sep = "\\";
	p += sep + "lib" + sep + "clang" + sep + CLANG_VERSION_STRING;
    	Clang.getHeaderSearchOpts().ResourceDir = p;
	
	*Dbg << "// ResourceDir is " << Clang.getHeaderSearchOpts().ResourceDir << "\n";

	if (!is_Cfile(InputFilename)) {
		SMDiagnostic SM;
		LLVMContext & Context = getGlobalContext();
		M = parseIRFile(InputFilename,SM,Context); 
	} else {
		if (!Clang.ExecuteAction(Act)) {
		//*Dbg << "Unable to produce LLVM bitcode. Please use Clang with the appropriate options.\n";
		    return;
		}
		// run takeLLVMContext(): otherwise the LLVMContext
		// is deleted when the Action is deleted,
		// which destroys also M, thus M is then doubly destroyed.
		Act.takeLLVMContext();
		M = Act.takeModule();
	}

	if (M == NULL) {
		*Dbg << "ERROR: Unable to read bitcode file.\n";
		return;
	}

	PassRegistry &Registry = *PassRegistry::getPassRegistry();
	initializeAnalysis(Registry);

	// Build up all of the passes that we want to do to the module.
  legacy::PassManager InitialPasses;
  legacy::PassManager AnalysisPasses;

	if (!WCETSettings())
		InitialPasses.add(new RemoveUndet());
	if (optimizeBC()) {
		// may degrade precision of the analysis
		PassManagerBuilder Builder; 
		Builder.OptLevel = 3; 
		Builder.populateModulePassManager(InitialPasses);
	}

	FunctionPass *LoopInfoPass = new LoopInfoWrapperPass();

	InitialPasses.add(createGCLoweringPass());
	
	// this pass converts SwitchInst instructions into a sequence of
	// binary branch instructions, easier to deal with
	InitialPasses.add(createLowerSwitchPass());	
	InitialPasses.add(createLowerInvokePass());
	InitialPasses.add(LoopInfoPass);
	InitialPasses.add(new ExpandAssume());
	if (InstCombining()) {
		InitialPasses.add(createInstructionCombiningPass());
	}
	if (!WCETSettings()) {
		InitialPasses.add(new ExpandEqualities());
	}
	//Passes.add(createLoopSimplifyPass());	

	// in case we want to run an Alias analysis pass : 
	//Passes.add(createGlobalsModRefPass());
	//Passes.add(createBasicAliasAnalysisPass());
	//Passes.add(createScalarEvolutionAliasAnalysisPass());
	//Passes.add(createTypeBasedAliasAnalysisPass());
	//
	TagInline * taginlinepass = new TagInline();
	if (inline_functions()) {
		InitialPasses.add(taginlinepass); // this pass has to be run before the internalizepass, since it builds the list of functions to analyze
	}
		
	if (check_overflow()) InitialPasses.add(new instrOverflow());

	InitialPasses.add(new NameAllValues());

	// make sure everything is run before AI analysis
	InitialPasses.run(*M);
	if (inline_functions()) {
    legacy::PassManager InlinePasses;
		InlinePasses.add(llvm::createAlwaysInlinerPass());
		InlinePasses.add(createInternalizePass(TagInline::GetFunctionsToAnalyze()));
		InlinePasses.add(createGlobalDCEPass());
		InlinePasses.add(createGlobalOptimizerPass());
		InlinePasses.run(*M);
	}
	
  legacy::PassManager OptPasses;

	if (global2local())
	  OptPasses.add(new GlobalToLocal());

	if (loop_rotate())
	  OptPasses.add(createLoopRotatePass());

	if (!WCETSettings())
		OptPasses.add(new RemoveUndet());

	if (brutal_unrolling()) {
		OptPasses.add(createLoopSimplifyPass());
		OptPasses.add(createLCSSAPass());
		OptPasses.add(createIndVarSimplifyPass());
		OptPasses.add(createScalarReplAggregatesPass());
		//OptPasses.add(createLoopUnrollPass(INT_MAX,INT_MAX,1,0));
	}
	OptPasses.add(createPromoteMemoryToRegisterPass());
	OptPasses.run(*M);

	if (dumpll()) {
		*Out << *M;
		return;
	}
	AnalysisPasses.add(new IdentifyLoops());
	if (onlyOutputsRho()) {
		AnalysisPasses.add(new GenerateSMT());
	} else if (compareTechniques()) {
		AnalysisPasses.add(new Compare(getComparedTechniques()));
	} else if (compareNarrowing()) {
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				AnalysisPasses.add(new CompareNarrowing<LOOKAHEAD_WIDENING>());
				break;
			case GUIDED:
				AnalysisPasses.add(new CompareNarrowing<GUIDED>());
				break;
			case PATH_FOCUSING:
				AnalysisPasses.add(new CompareNarrowing<PATH_FOCUSING>());
				break;
			case PATH_FOCUSING_INCR:
				AnalysisPasses.add(new CompareNarrowing<PATH_FOCUSING_INCR>());
				break;
			case LW_WITH_PF:
				AnalysisPasses.add(new CompareNarrowing<LW_WITH_PF>());
				break;
			case COMBINED_INCR:
				AnalysisPasses.add(new CompareNarrowing<COMBINED_INCR>());
				break;
			case SIMPLE:
				AnalysisPasses.add(new CompareNarrowing<SIMPLE>());
				break;
			case LW_WITH_PF_DISJ:
				AnalysisPasses.add(new CompareNarrowing<LW_WITH_PF_DISJ>());
				break;
		}
	} else if (compareDomain()) {
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				AnalysisPasses.add(new CompareDomain<LOOKAHEAD_WIDENING>());
				break;
			case GUIDED:
				AnalysisPasses.add(new CompareNarrowing<GUIDED>());
				break;
			case PATH_FOCUSING:
				AnalysisPasses.add(new CompareDomain<PATH_FOCUSING>());
				break;
			case PATH_FOCUSING_INCR:
				AnalysisPasses.add(new CompareDomain<PATH_FOCUSING_INCR>());
				break;
			case LW_WITH_PF:
				AnalysisPasses.add(new CompareDomain<LW_WITH_PF>());
				break;
			case COMBINED_INCR:
				AnalysisPasses.add(new CompareDomain<COMBINED_INCR>());
				break;
			case SIMPLE:
				AnalysisPasses.add(new CompareDomain<SIMPLE>());
				break;
			case LW_WITH_PF_DISJ:
				AnalysisPasses.add(new CompareDomain<LW_WITH_PF_DISJ>());
				break;
		}
	} else { 
		ModulePass *AIPass;
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				AIPass = new ModulePassWrapper<AIGopan, 0>();
				break;
			case GUIDED:
				AIPass = new ModulePassWrapper<AIGuided, 0>();
				break;
			case PATH_FOCUSING:
				AIPass = new ModulePassWrapper<AIpf, 0>();
				break;
			case PATH_FOCUSING_INCR:
				AIPass = new ModulePassWrapper<AIpf_incr, 0>();
				break;
			case LW_WITH_PF:
				AIPass = new ModulePassWrapper<AIopt, 0>();
				break;
			case COMBINED_INCR:
				AIPass = new ModulePassWrapper<AIopt_incr, 0>();
				break;
			case SIMPLE:
				AIPass = new ModulePassWrapper<AIClassic, 0>();
				break;
			case LW_WITH_PF_DISJ:
				AIPass = new ModulePassWrapper<AIdis, 0>();
				break;
		}
		AnalysisPasses.add(AIPass);
	}
	AnalysisPasses.run(*M);

	//*Out << *M;
	std::error_code error;

	if (generateMetadata()) {
	  raw_fd_ostream * BitcodeOutput = new raw_fd_ostream(getAnnotatedBCFilename().c_str(), error, sys::fs::F_None);
	  WriteBitcodeToFile(M.get(), *BitcodeOutput);
		BitcodeOutput->close();
	}

	if (onlyOutputsRho()) {
		return;
	}

#ifdef TOTO
	// we properly delete all the created Nodes
	std::map<BasicBlock*,Node*>::iterator it = Nodes.begin(), et = Nodes.end();
	for (;it != et; it++) {
		delete (*it).second;
	}
#endif

	Pr::releaseMemory();
	SMTpass::releaseMemory();
	ReleaseTimingData();
	Expr::clear_exprs();

	if (OutputFilename != "") {
		delete Out;
	}
}

