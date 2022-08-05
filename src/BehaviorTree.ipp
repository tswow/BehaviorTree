#pragma once

#include "BehaviorTree.h"

#include <stdexcept>

//
// Constructors
//

template <typename C,typename LC,typename DC>
Node<C,LC,DC>::~Node() {}

template <typename C, typename LC, typename DC>
Node<C,LC,DC>::Node(BehaviorTreeContext<C,LC,DC>* gen)
    : m_gen(gen)
{}

template <typename C, typename LC, typename DC, typename T>
DecorableNode<C,LC,DC, T>::DecorableNode(BehaviorTreeContext<C,LC,DC>* gen)
    : Node<C,LC,DC>(gen)
{

}

template <typename C, typename LC, typename DC>
Leaf<C,LC,DC>::Leaf(BehaviorTreeContext<C,LC,DC>* gen, LeafCallback<C,LC> exec)
    : DecorableNode<C,LC,DC,Leaf<C,LC,DC>>(gen), m_exec(exec)
{

}

template <typename C, typename LC, typename DC>
Multiplexer<C,LC,DC>::Multiplexer(BehaviorTreeContext<C,LC,DC>* gen, LeafCallback<C,LC> exec)
    : BranchNode<C,LC,DC,Multiplexer<C,LC,DC>>(gen)
    , m_callback(exec)
{

}

template <typename C, typename LC, typename DC, typename T>
BranchNode<C,LC,DC,T>::BranchNode(BehaviorTreeContext<C,LC,DC>* ctx)
    : DecorableNode<C,LC,DC,T>(ctx)
{
}

template <typename C, typename LC, typename DC>
Branch<C,LC,DC>::Branch(BehaviorTreeContext<C,LC,DC>* gen, BranchType type)
    : BranchNode<C,LC,DC,Branch<C,LC,DC>>(gen)
    , m_type(type)
{

}

template <typename C, typename LC, typename DC>
Branch<C,LC,DC>* Branch<C,LC,DC>::SetLoops(uint64_t loops)
{
    m_loops = loops;
    return this;
}

template <typename C, typename LC, typename DC>
Branch<C,LC,DC>* Branch<C,LC,DC>::SetAttempts(uint64_t attempts)
{
    m_attempts = attempts;
    return this;
}

template <typename C, typename LC, typename DC>
TreeExecutor<C,LC,DC>::TreeExecutor(BehaviorTreeContext<C,LC,DC>* ctx, Node<C,LC,DC>* root)
    : m_ctx(ctx)
    , m_root(root)
{}

template <typename C, typename LC, typename DC>
size_t TreeExecutor<C,LC,DC>::NodeStackDepth()
{
    return m_nodeStack.size();
}


//
// Creator Methods
//
template <typename C, typename LC, typename DC>
void BehaviorTreeContext<C,LC,DC>::DestroyAllNodes()
{
    m_leaves.clear();
    m_multiplexers.clear();
    m_branches.clear();
}

template <typename C, typename LC, typename DC>
Branch<C,LC,DC>* BehaviorTreeContext<C,LC,DC>::CreateSequence()
{
    return m_branches.emplace_back(std::make_unique<Branch<C,LC,DC>>(this, BranchType::SEQUENCE)).get();
}

template <typename C, typename LC, typename DC>
Branch<C,LC,DC>* BehaviorTreeContext<C,LC,DC>::CreateSelector()
{
    return m_branches.emplace_back(std::make_unique<Branch<C,LC,DC>>(this, BranchType::SELECTOR)).get();
}

template <typename C, typename LC, typename DC>
Multiplexer<C,LC,DC>* BehaviorTreeContext<C,LC,DC>::CreateMultiplexer(LeafCallback<C,LC> callback)
{
    return m_multiplexers.emplace_back(std::make_unique<Multiplexer<C,LC,DC>>(this, callback)).get();
}

template <typename C, typename LC, typename DC>
Multiplexer<C,LC,DC>* BehaviorTreeContext<C,LC,DC>::CreateMultiplexer()
{
    return m_multiplexers.emplace_back(std::make_unique<Multiplexer<C,LC,DC>>(this)).get();
}

template <typename C, typename LC, typename DC>
Leaf<C,LC,DC>* BehaviorTreeContext<C,LC,DC>::CreateLeaf(LeafCallback<C,LC> exec)
{
    return m_leaves.emplace_back(std::make_unique<Leaf<C,LC,DC>>(this, exec)).get();
}

template <typename C, typename LC, typename DC>
void BehaviorTreeContext<C,LC,DC>::VerifyNode(Node<C,LC,DC>* node)
{
}

//
// Decorable Methods
//

template <typename C, typename LC, typename DC, typename T>
T* DecorableNode<C,LC,DC, T>::Decorate(DecoratorCallback<C,DC> predicate)
{
    this->m_decorations.push_back(predicate);
    return dynamic_cast<T*>(this);
}

template <typename C, typename LC, typename DC, typename T>
T* DecorableNode<C,LC,DC,T>::Decorate(std::vector<DecoratorCallback<C,DC>> predicates)
{
    this->m_decorations.insert(this->m_decorations.end(), predicates.begin(), predicates.end());
    return dynamic_cast<T*>(this);
}

//
// Branch Methods
//

template <typename C, typename LC, typename DC, typename T>
T* BranchNode<C,LC,DC,T>::AddSequence(Builder<Branch<C,LC,DC>> builder)
{
    Branch<C,LC,DC>* b = this->m_gen->CreateSequence();
    m_children.push_back(b);
    builder(b);
    return dynamic_cast<T*>(this);
}

template <typename C, typename LC, typename DC,typename T>
T* BranchNode<C,LC,DC,T>::AddSelector(Builder<Branch<C,LC,DC>> builder)
{
    Branch<C,LC,DC>* b = this->m_gen->CreateSelector();
    m_children.push_back(b);
    builder(b);
    return dynamic_cast<T*>(this);
}

template <typename C, typename LC, typename DC,typename T>
T* BranchNode<C,LC,DC,T>::AddLeaf(LeafCallback<C,LC> exec, Builder<Leaf<C,LC,DC>> builder)
{
    Leaf<C,LC,DC>* b = this->m_gen->CreateLeaf(exec);
    m_children.push_back(b);
    builder(b);
    return dynamic_cast<T*>(this);
}

template <typename C, typename LC, typename DC, typename T>
T* BranchNode<C,LC,DC,T>::AddLeaf(LeafCallback<C,LC> exec)
{
    Leaf<C,LC,DC>* b = this->m_gen->CreateLeaf(exec);
    m_children.push_back(b);
    return dynamic_cast<T*>(this);
}

template <typename C, typename LC, typename DC,typename T>
T* BranchNode<C,LC,DC,T>::AddNode(Node<C,LC,DC>* node)
{
    m_children.push_back(node);
    return dynamic_cast<T*>(this);
}

template <typename C, typename LC, typename DC,typename T>
T* BranchNode<C,LC,DC,T>::AddMultiplexer(LeafCallback<C,LC> callback, Builder<Multiplexer<C,LC,DC>> builder)
{
    Multiplexer<C,LC,DC>* multiplexer = this->m_gen->CreateMultiplexer(callback);
    this->m_children.emplace_back(multiplexer);
    builder(multiplexer);
    return dynamic_cast<T*>(this);
}

template <typename C, typename LC, typename DC,typename T>
T* BranchNode<C,LC,DC,T>::AddMultiplexer(Builder<Multiplexer<C,LC,DC>> builder)
{
    Multiplexer<C,LC,DC>* multiplexer = this->m_gen->CreateMultiplexer();
    this->m_children.emplace_back(multiplexer);
    builder(multiplexer);
    return dynamic_cast<T*>(this);
}

// Executor

inline TraversalMode ResultToTraversal(Result result)
{
    switch (result)
    {
    case Result::SUCCESS:
        return TraversalMode::SUCCESS;
    case Result::FAILURE:
        return TraversalMode::FAILURE;
    default:
        throw std::runtime_error("Invalid result for traversal");
    }
}

bool TreeTimer::HasPassed(uint64_t now)
{
    return m_start + m_delay <= now;
}

void TreeTimer::Set(uint64_t now, uint64_t delay)
{
    m_start = now;
    m_delay = delay;
}

void TreeTimer::Disable()
{
    m_start = 0;
    m_delay = UINT64_MAX;
}

void TreeTimer::Clear()
{
    m_start = 0;
    m_delay = 0;
}


template <typename C, typename LC, typename DC>
void TreeExecutor<C,LC,DC>::Update(C& ctx, uint64_t now)
{
    // ====================================================================
    // Goto Call Setup
    // ====================================================================
#define __BT_TREE_LABEL(label)\
    goto error;\
    label:

#define __BT_TREE_GOTO_REBUILD(aNode,aReason)\
    node = aNode;\
    reason = aReason;\
    goto rebuild;
    Result reason;
    int node;

#define __BT_TREE_GOTO_TRAVERSE(aMode)\
    mode = aMode;\
    goto traverse;
    TraversalMode mode;

#define __BT_TREE_GOTO_ADD_CHILD(aChild)\
    child = aChild;\
    goto add_child;
    Node<C,LC,DC>* child;

#define __BT_TREE_GOTO_EXECUTE()\
    goto execute;

    // ====================================================================
    // Start
    // ====================================================================
    {
        if (m_nodeStack.size() == 0)
        {
            __BT_TREE_GOTO_ADD_CHILD(m_root)
        }
        for (int i = 0; i < m_decoratorStack.size(); ++i)
        {
            DecoratorStackEntry<DC>& decoratorEntry = m_decoratorStack[i];
            if (!decoratorEntry.m_timer.HasPassed(now))
            {
                continue;
            }
            DecoratorCallback<C,DC> decorator = m_nodeStack[decoratorEntry.m_nodeStackIndex].m_node->m_decorations[decoratorEntry.m_decoratorIndex];
            int result = decorator(ctx,decoratorEntry.m_memory);
            switch (result)
            {
            case Result::SUCCESS:
                decoratorEntry.m_timer.Disable();
                break;
            case Result::FAILURE:
                __BT_TREE_GOTO_REBUILD(decoratorEntry.m_nodeStackIndex, Result::FAILURE);
            default:
                decoratorEntry.m_timer.Set(now, result);
                break;
            }
        }
        __BT_TREE_GOTO_EXECUTE()
    }

    // ====================================================================
    __BT_TREE_LABEL(rebuild)
    // ====================================================================
    {
        if (node < 0)
        {
            m_nodeStack.clear();
            m_decoratorStack.clear();
            return;
        }
        else
        {
            m_nodeStack.resize(node + 1);
            int decoStackSize = m_nodeStack[m_nodeStack.size()-1].m_decoStackSize;
            m_decoratorStack.resize(decoStackSize);
            __BT_TREE_GOTO_TRAVERSE(ResultToTraversal(reason));
        }
    }

    // ====================================================================
    __BT_TREE_LABEL(traverse)
    // invariants:
    //  - node stack is not empty
    //  - top of node stack is a branch
    // ====================================================================
    {
        m_subtrees.clear();
        m_endTimer.Clear();
        NodeStackEntry<C,LC,DC>& entry = m_nodeStack[m_nodeStack.size() - 1];
        Branch<C,LC,DC>* branch = static_cast<Branch<C,LC,DC>*>(entry.m_node);
        switch (mode)
        {
        case TraversalMode::SUCCESS:
            switch (branch->m_type)
            {
            case BranchType::SELECTOR:
                entry.m_loop++;
                entry.m_ctr = 0;
                break;
            case BranchType::SEQUENCE:
                entry.m_ctr++;
                break;
            default:
                throw std::runtime_error("Invalid BranchType");
                break;
            }
            break;
        case TraversalMode::FAILURE:
            switch (branch->m_type)
            {
            case BranchType::SELECTOR:
                entry.m_ctr++;
                break;
            case BranchType::SEQUENCE:
                entry.m_retry++;
                entry.m_ctr = 0;
                break;
            default:
                throw std::runtime_error("Invalid BranchType");
                break;
            }
            break;
        }

        if (entry.m_ctr >= branch->m_children.size())
        {
            switch (branch->m_type)
            {
            case BranchType::SELECTOR:
                entry.m_retry++;
                entry.m_ctr = 0;
                break;
            case BranchType::SEQUENCE:
                entry.m_loop++;
                entry.m_ctr = 0;
                break;
            default:
                throw std::runtime_error("Invalid BranchType");
                break;
            }
        }

        if (entry.m_retry >= branch->m_attempts)
        {
            __BT_TREE_GOTO_REBUILD(int(m_nodeStack.size() - 2), Result::FAILURE);
        }
        else if (entry.m_loop >= branch->m_loops)
        {
            __BT_TREE_GOTO_REBUILD(int(m_nodeStack.size() - 2), Result::SUCCESS);
        }
        else {
            __BT_TREE_GOTO_ADD_CHILD(branch->m_children[entry.m_ctr]);
        }
    }

    // ====================================================================
    __BT_TREE_LABEL(add_child)
    // ====================================================================
    {
        for (int i = 0; i < child->m_decorations.size(); ++i)
        {
            DC dc;
            DecoratorCallback<C,DC> decorator = child->m_decorations[i];
            int oldSize = int(m_decoratorStack.size());
            int value = decorator(ctx,dc);
            switch (value)
            {
            case Result::SUCCESS:
                break;
            case Result::FAILURE:
                if (m_nodeStack.size() == 0)
                {
                    m_decoratorStack.clear();
                    return;
                }
                else
                {
                    __BT_TREE_GOTO_TRAVERSE(TraversalMode::FAILURE);
                }
                break;
            default:
                m_decoratorStack.push_back({ std::move(dc), int(m_nodeStack.size()), i, {now,uint64_t(value)} });
                break;
            }
        }
        m_nodeStack.push_back({ child,LC(),int(m_decoratorStack.size()) });

        if (Branch<C,LC,DC>* branch = dynamic_cast<Branch<C,LC,DC>*>(child))
        {
            __BT_TREE_GOTO_TRAVERSE(TraversalMode::TRAVERSAL)
        }
        else
        {
            if (Multiplexer<C,LC,DC>* multiplexer = dynamic_cast<Multiplexer<C,LC,DC>*>(child))
            {
                for (Node<C,LC,DC>* child : multiplexer->m_children)
                {
                    m_subtrees.push_back(TreeExecutor<C,LC,DC>(m_ctx,child));
                }
            }
            __BT_TREE_GOTO_EXECUTE()
        }
    }

    // ====================================================================
    __BT_TREE_LABEL(execute)
    // invariants:
    //   - m_nodeStack is not empty
    //   - top of m_nodeStack is not a branch
    // ====================================================================
    {
        NodeStackEntry<C,LC,DC>& entry = m_nodeStack[m_nodeStack.size() - 1];
        if (!m_endTimer.HasPassed(now))
        {
            return;
        }

        if (Leaf<C,LC,DC>* leaf = dynamic_cast<Leaf<C,LC,DC>*>(entry.m_node))
        {
            int res = leaf->m_exec(ctx,entry.m_memory);
            switch (res)
            {
            case Result::SUCCESS:
            case Result::FAILURE:
                __BT_TREE_GOTO_REBUILD(int(m_nodeStack.size()) - 2, Result(res))
                break;
            default:
                m_endTimer.Set(now, res);
                return;
            }
        }
        else if (Multiplexer<C,LC,DC>* multiplexer = dynamic_cast<Multiplexer<C,LC,DC>*>(entry.m_node))
        {
            int res = multiplexer->m_callback ? multiplexer->m_callback(ctx,entry.m_memory) : 0;
            switch (res)
            {
            case Result::SUCCESS:
            case Result::FAILURE:
                __BT_TREE_GOTO_REBUILD(int(m_nodeStack.size()) - 2, Result(res));
                break;
            default:
                m_endTimer.Set(now, res);
                for (TreeExecutor<C,LC,DC>& tree : m_subtrees)
                {
                    tree.Update(ctx, now);
                }
                return;
            }
        }
        else
        {
            throw std::runtime_error("Unhandled unknown Node subclass");
        }
    }
    
    // ====================================================================
    __BT_TREE_LABEL(error)
    // ====================================================================
    {
        throw std::runtime_error("Uh oh we shouldn't end up here");
    }
#undef __BT_TREE_LABEL
#undef __BT_TREE_GOTO_REBUILD
#undef __BT_TREE_GOTO_TRAVERSE
#undef __BT_TREE_GOTO_ADD_CHILD
#undef __BT_TREE_GOTO_EXECUTE
}

