
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"

#include "traverse.h"

#include <iostream>

namespace screen {
using namespace llvm;

bool TraverseLinearly::traverse(const Function *F) 
{
    for (const BasicBlock &BB: *F) {
        for (const Instruction &I: BB) {
            m_cb(I);
        }
    }
    return true;
}

bool TraverseCfg::traverse(const llvm::BasicBlock *BB)
{
    m_stateMap[BB] = kInProgress;

    // Before attempting to look at blocks dominated by this one, handle all
    // outgoing calls.
    for (const Instruction &I: *BB) {

        // Invoke our user callback on each instruction of current block
        m_cb(I);

        if(const CallInst *C = dyn_cast<CallInst>(&I)) {
            const Function *called = C->getCalledFunction();

            if (called == nullptr)
                continue;

            // Invoke the callback when it's an annotation so it can decide on
            // whether to start or end tracking
            if (called->isIntrinsic() &&
                     !called->getName().startswith("llvm.var.annotation"))
                continue;

            traverse(called, C);
        }
    }

    // Recurse into all successor blocks
    const TerminatorInst *TInst = BB->getTerminator();
    for (unsigned I = 0, NSucc = TInst->getNumSuccessors(); I < NSucc; ++I) {
        const llvm::BasicBlock *Succ = TInst->getSuccessor(I);
        BBState state = m_stateMap[Succ];

        if (state == kUnvisited) {
            if (!traverse(Succ)) {
                return false;
            }
        } else if (state == kInProgress) {
            // return false;
            //TODO: figure out why this is true too return false;
        }
    }

    m_stateMap[BB] = kVisited;
    return true;
}

bool TraverseCfg::traverse(const Function *F, const CallInst *CI)
{
    
    if (F == nullptr)
        return false;

    // Ignore this if we have visited this in the current iteration
    auto existing = std::find_if(m_visited.begin(), m_visited.end(),
            [F](Breadcrumb b) { return b.second == F; });
    if (existing != m_visited.end())  {
        return false;
    } 

    // Ignore if we have no function body (definition has not been linked yet)
    if (F->begin() == F->end())
        return false;

    m_visited.push_back(std::make_pair(CI, F));

    traverse(&F->getEntryBlock());

    m_visited.pop_back();
    return true;
}

TraverseCfg::VisitedPath TraverseCfg::pathVisited(std::string name)
{
    auto start = m_visitedMap.find(name);

    if (start == m_visitedMap.end())
        return VisitedPath{};

    VisitedPath path(m_visited.begin() + start->second, m_visited.end());

    return path;
}

} // namespace screen
