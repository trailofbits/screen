
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"

#include "traverse.h"

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

        if(const CallInst *C = dyn_cast<CallInst>(&I)) {
            const Function *called = C->getCalledFunction();

            if (called == nullptr)
                continue;

            if (called->isIntrinsic())
                continue;

            traverse(called);
        }
        // Invoke our user callback on each instruction of current block
        m_cb(I);
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
            return false;
        }
    }

    m_stateMap[BB] = kVisited;
    return true;
}

bool TraverseCfg::traverse(const Function *F)
{
    // Ignore this if we have visited this in the current iteration
    auto existing = std::find(m_visited.begin(), m_visited.end(), F);
    if (existing != m_visited.end()) 
        return false;

    // Ignore if we have no function body (definition has not been linked yet)
    if (F->begin() == F->end())
        return false;

    m_visited.push_back(F);

    traverse(&F->getEntryBlock());

    m_visited.pop_back();
    return true;
}
std::vector<const llvm::Function *> &TraverseCfg::pathVisited(void)
{
    return m_visited;
}

} // namespace screen
