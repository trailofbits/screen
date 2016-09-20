/**
 * \file ModulePassWrapper.h
 * \brief Declares the ModulePassWrapper template class
 * \author Matthieu Moy, Julien Henry
 */
#ifndef MODULEPASSWRAPPER_H
#define MODULEPASSWRAPPER_H

#include "Analyzer.h"

using namespace llvm;

/**
 * \brief Wrapper to allow instanciating an LLVM pass multiple times.
 *
 * The pass to wrapp must have a constructor accepting an ID and
 * transmitting it to the constructor of ModulePass.
 *
 * The first template argument is the pass to wrap, and the second is
 * the number of the instance. To use 2 instances of a pass Foo,
 * register them with e.g.
 *
 * @code
 * static RegisterPass<ModulePassWrapper<Foo, 0> > Y0("Foo_wrapped0", "Pass Foo", false, true);
 * static RegisterPass<ModulePassWrapper<Foo, 1> > Y1("Foo_wrapped1", "Pass Foo", false, true);
 * @endcode
 *
 * Then use <code>ModulePassWrapper<Foo, 0></code> and
 * <code>ModulePassWrapper<Foo, 1></code> just like any other LLVM
 * passes:
 *
 * @code
 * Passes.add(new ModulePassWrapper<AIopt, 0>());
 * // ...
 * void SomePass::getAnalysisUsage(AnalysisUsage &AU) const {
 * 	AU.addRequired<ModulePassWrapper<AIopt, 0> >();
 * }
 * @endcode
 *
 * Note that the class wrapped into ModulePassWrapper does not need
 * to have a <code>static char ID</code> itself, and needs not be
 * registered as a pass.
 */
template<typename P, int i = 0>
class ModulePassWrapper : public P {
	public:
		/**
		 * \brief Pass Identifier
		 *
		 * It is crucial for LLVM's pass manager that
		 * this ID is different (in address) from a class to another,
		 * but the template instantiation mechanism will make sure it
		 * is the case.
		 */
		static char ID;

		ModulePassWrapper()
			: P(ID,getApronManager(i),useNewNarrowing(i),useThreshold(i))
			{}

		~ModulePassWrapper() {}
};

template<typename P, int i>
char ModulePassWrapper<P, i>::ID = i;

#endif // MODULEPASSWRAPPER_H
