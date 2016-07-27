#ifndef __TRAVERSE
#define __TRAVERSE

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

namespace screen {

// @brief A Traverse is the abstract type that can traverse through a function
// and invoke a callback for each instruction visited.
class Traverser {
public:
    using Cb = std::function<void(const llvm::Instruction &I)>;

    Traverser()
    :m_cb([](const llvm::Instruction &I){ }) {}

    // @brief Register a callback to be invoked on each instruction
    void setCallback(Cb cb) {
        m_cb = cb;
    }

    // @brief The 'entry point' to start the traversal. This will execute the
    // callback provided by 'setCallback' on every instructino in the order that
    // is implemented by a deriving class.
    virtual bool traverse(const llvm::Function *F) = 0;

protected:
    Cb m_cb;
};

// @brief TraverseLinearly invokes a registered callback for every instruction
// in a functino, top-down.
class TraverseLinearly : public Traverser {
public:
    // @brief Traverse the instructions of a function top-down.
    virtual bool traverse(const llvm::Function *F) override;
};


// @brief TraverseCfg invokes a registered callback for every instruction in a 
// function and all functions it calls, recursively.
class TraverseCfg : public Traverser {
public:

    using VisitedPath = std::vector<const llvm::Function *>;

    // @brief Traverse the instructions of a control flow graph anchored at F.
    virtual bool traverse(const llvm::Function *F) override;

    // @brief Return the path that we took so far.
    VisitedPath pathVisited(std::string name);

    void startPath(std::string name) {
        m_visitedMap[name] = m_visited.size() - 1;
    }

    void endPath(std::string name) {
        m_visitedMap.erase(name);
    }

private:
    bool traverse(const llvm::BasicBlock *BB);

    VisitedPath m_visited;

    enum BBState {kUnvisited, kInProgress, kVisited};

    llvm::DenseMap<const llvm::BasicBlock *, BBState> m_stateMap;

    std::map<std::string, VisitedPath::size_type> m_visitedMap;
};
} // namespace screen


#endif
