#pragma once

#include <functional>
#include <vector>
#include <cstdint>
#include <variant>

//
// Declarations
//
template <typename C = std::monostate, typename LC = std::monostate, typename DC = LC>
struct LeafEndpoint;

template <typename C = std::monostate, typename LC = std::monostate, typename DC = LC>
struct MultiplexerEndpoint;

template <typename C = std::monostate, typename LC = std::monostate, typename DC = LC>
class TreeExecutor;

template <typename C = std::monostate, typename LC = std::monostate, typename DC = LC>
class Branch;

template <typename C = std::monostate, typename LC = std::monostate, typename DC = LC>
class Leaf;

template <typename C = std::monostate, typename LC = std::monostate, typename DC = LC>
class Multiplexer;

template <typename C = std::monostate, typename LC = std::monostate, typename DC = LC>
class BehaviorTreeContext;

template <typename C = std::monostate, typename M = std::monostate>
using DecoratorCallback = std::function<int(C&,M&)>;

template <typename C = std::monostate, typename M = std::monostate>
using LeafCallback = std::function<int(C&,M&)>;

template <typename T>
using Builder = std::function<void(T*)>;

enum class BranchType: int
{
    SEQUENCE,
    SELECTOR
};

enum Result: int
{
    INSTANT =  0,
    SUCCESS = -1,
    FAILURE = -2
};

enum class TraversalMode: int
{
    SUCCESS,
    FAILURE,
    TRAVERSAL
};

TraversalMode ResultToTraversal(Result result);

//
// Builder Classes
//

template <typename C, typename LC, typename DC>
class Node
{
public:
    virtual ~Node();
    Node(BehaviorTreeContext<C,LC,DC>* gen);
protected:
    BehaviorTreeContext<C,LC,DC>* m_gen;
    std::vector<DecoratorCallback<C,DC>> m_decorations;
    friend class TreeExecutor<C,LC,DC>;
    friend class BehaviorTreeContext<C,LC,DC>;
};

template <typename C, typename LC, typename DC, typename T>
class DecorableNode : public Node<C,LC,DC>
{
public:
    T* decorate(DecoratorCallback<C,DC> predicate);
    T* decorate(std::vector<DecoratorCallback<C,DC>> predicates);
protected:
    DecorableNode(BehaviorTreeContext<C,LC,DC>* gen);
};

template <typename C, typename LC, typename DC>
class Leaf : public DecorableNode<C,LC,DC, Leaf<C,LC,DC>>
{
public:
    Leaf(BehaviorTreeContext<C,LC,DC>* gen, LeafCallback<C,LC> exec);
private:
    LeafCallback<C,LC> m_exec;
    friend class BehaviorTreeContext<C,LC,DC>;
    friend class TreeExecutor<C,LC,DC>;
    friend struct LeafEndpoint<C,LC,DC>;
};

template <typename C, typename LC, typename DC, typename T>
class BranchNode: public DecorableNode<C,LC,DC,T>
{
public:
    T* AddSequence(Builder<Branch<C,LC,DC>> builder);
    T* AddSelector(Builder<Branch<C,LC,DC>> builder);
    T* AddLeaf(LeafCallback<C,LC> exec, Builder<Leaf<C,LC,DC>> builder);
    T* AddLeaf(LeafCallback<C,LC> exec);
    T* AddNode(Node<C,LC,DC>* node);
    T* AddMultiplexer(LeafCallback<C,LC> callback, Builder<Multiplexer<C,LC,DC>> builder);
    T* AddMultiplexer(Builder<Multiplexer<C,LC,DC>> builder);
    BranchNode(BehaviorTreeContext<C,LC,DC>* ctx);
protected:
    std::vector<Node<C,LC,DC>*> m_children;
    friend class TreeExecutor<C,LC,DC>;
};

template <typename C, typename LC, typename DC>
class Multiplexer : public BranchNode<C,LC,DC, Multiplexer<C,LC,DC>>
{
public:
    Multiplexer(BehaviorTreeContext<C,LC,DC>* gen, LeafCallback<C,LC> exec = nullptr);
protected:
    LeafCallback<C,LC> m_callback;
    friend class TreeExecutor<C,LC,DC>;
    friend class BehaviorTreeContext<C,LC,DC>;
    friend struct MultiplexerEndpoint<C,LC,DC>;
};

template <typename C, typename LC, typename DC>
class Branch : public BranchNode<C,LC,DC,Branch<C,LC,DC>>
{
public:
    Branch<C,LC,DC>* set_loops(uint64_t loops);
    Branch<C,LC,DC>* set_attempts(uint64_t retries);
    Branch(BehaviorTreeContext<C,LC,DC>* gen, BranchType type);
protected:
    BranchType m_type;
    uint64_t m_loops = 1;
    uint64_t m_attempts = 1;
    friend class TreeExecutor<C,LC,DC>;
    friend class BehaviorTreeContext<C,LC,DC>;
};

//
// Execution
//

struct TreeTimer
{
    uint64_t m_delay = 0;
    uint64_t m_start = 0;
    bool HasPassed(uint64_t now);
    void Set(uint64_t now, uint64_t delay);
    void Disable();
    void Clear();
};

template <typename DC>
struct DecoratorStackEntry
{
    DC m_memory;
    int m_nodeStackIndex;
    int m_decoratorIndex;
    TreeTimer m_timer;
};

template <typename C, typename LC, typename DC>
struct NodeStackEntry
{
    Node<C,LC,DC>* m_node;
    LC m_memory;
    int m_decoStackSize;
    int m_ctr = 0;
    int m_loop = 0;
    int m_retry = 0;
};

template <typename C, typename LC, typename DC>
class TreeExecutor
{
public:
    TreeExecutor(BehaviorTreeContext<C,LC,DC>* ctx, Node<C,LC,DC>* root);
    void Update(C& ctx, uint64_t now);
    size_t NodeStackDepth();
private:
    Node<C,LC,DC>* m_root;
    BehaviorTreeContext<C, LC, DC>* m_ctx;
    TreeTimer m_endTimer;
    std::vector<TreeExecutor<C,LC,DC>> m_subtrees;
    std::vector<NodeStackEntry<C,LC,DC>> m_nodeStack;
    std::vector<DecoratorStackEntry<DC>> m_decoratorStack;
    friend class BehaviorTreeContext<C,LC,DC>;
};

//
// Context
//

template <typename C, typename LC, typename DC>
class BehaviorTreeContext
{
public:
    void DestroyAllNodes();
    Branch<C,LC,DC>* CreateSequence();
    Multiplexer<C,LC,DC>* CreateMultiplexer(LeafCallback<C,LC> callback);
    Multiplexer<C,LC,DC>* CreateMultiplexer();
    Branch<C,LC,DC>* CreateSelector();
    Leaf<C,LC,DC>* CreateLeaf(LeafCallback<C,LC> exec);
    void VerifyNode(Node<C,LC,DC>* node);
private:
    std::vector<std::unique_ptr<Leaf<C,LC,DC>>> m_leaves;
    std::vector<std::unique_ptr<Multiplexer<C,LC,DC>>> m_multiplexers;
    std::vector<std::unique_ptr<Branch<C,LC,DC>>> m_branches;
};

#include "BehaviorTree.ipp"
