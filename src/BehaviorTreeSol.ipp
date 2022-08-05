#pragma once

#include "BehaviorTree.h"

#include <sol/sol.hpp>

template <typename C, typename LC, typename DC, typename T>
void LuaRegisterDecorableNode(std::string const& name, sol::state & state)
{
    sol::usertype<DecorableNode<C,LC,DC,T>> node = state.new_usertype<DecorableNode<C, LC, DC,T>>(name + "DecorableNode", sol::base_classes, sol::bases<Node<C, LC, DC>>());
    node.set_function("Decorate", sol::overload(
        [](DecorableNode<C, LC, DC,T>& node, sol::protected_function callback)
        {
            node.Decorate([=](C& ctx, DC& dc) { return callback(ctx, dc); });
        },
        [](DecorableNode<C, LC, DC,T>& node, sol::table table)
        {
            for (auto const& [key, value] : table)
            {
                node.Decorate([=](C& ctx, DC& dc) { return value.as<sol::protected_function>()(ctx, dc); });
            }
        }
    ));
}

template <typename C, typename LC, typename DC, typename T>
void LuaRegisterBranchNode(std::string const& name, sol::state& state)
{
    sol::usertype<BranchNode<C, LC, DC, T>> node = state.new_usertype<BranchNode<C, LC, DC, T>>(name + "BranchNode", sol::base_classes, sol::bases<DecorableNode<C, LC, DC, T>>());
    node.set_function("AddSequence",
        [](BranchNode<C, LC, DC, T>* self, sol::function fn) {
            self->AddSequence([=](Branch<C, LC, DC>* branch) { fn(branch); });
            return self;
        }
    );
    node.set_function("AddSelector",
        [](BranchNode<C, LC, DC, T>* self, sol::function fn) {
            self->AddSelector([=](Branch<C, LC, DC>* branch) { fn(branch); });
            return self;
        }
    );
    node.set_function("AddNode",
        [](BranchNode<C, LC, DC, T>* self, Node<C,LC,DC>* fn) {
            self->AddNode(fn);
            return self;
        }
    );

    node.set_function("AddLeaf", sol::overload(
        [](BranchNode<C, LC, DC, T>* self, sol::function exec) {
            return self->AddLeaf([=](C& ctx, LC& lc) { return exec(ctx, lc); });
        },
        [](BranchNode<C, LC, DC, T>* self, sol::function exec, sol::function callback) {
            return self->AddLeaf(
                [=](C& ctx, LC& lc) { return exec(ctx, lc); },
                [=](Node<C, LC, DC>* node) { callback(node); }
            );
        }
    ));

    node.set_function("AddMultiplexer", sol::overload(
        [](BranchNode<C, LC, DC, T>* self, sol::function callback) {
            return self->AddMultiplexer(
                [=](Multiplexer<C, LC, DC>* node) { callback(node); }
            );
        },
        [](BranchNode<C, LC, DC, T>* self, sol::function exec, sol::function callback) {
            return self->AddMultiplexer(
                [=](C& ctx, LC& lc) { return exec(ctx, lc); },
                [=](Multiplexer<C, LC, DC>* node) { callback(node); }
            );
        }
    ));
}

template <typename C, typename LC, typename DC>
void LuaRegisterBehaviorTree(sol::state & state, std::string const& nameBase, BehaviorTreeContext<C,LC,DC>* globalCtx = nullptr, std::string const& globalCtxPrefix = "", std::string const& globalCtxInfix = "", std::string const& globalCtxSuffix = "")
{
    sol::usertype<Node<C,LC,DC>> node = state.new_usertype<Node<C, LC, DC>>(nameBase + "Node");

    LuaRegisterDecorableNode<C, LC, DC, Branch<C, LC, DC>>(nameBase + "Branch", state);
    LuaRegisterBranchNode<C, LC, DC, Branch<C, LC, DC>>(nameBase + "Branch", state);
    sol::usertype<Branch<C, LC, DC>> branch = state.new_usertype<Branch<C, LC, DC>>(
        nameBase + "Branch",
        sol::base_classes, sol::bases<BranchNode<C,LC,DC,Branch<C,LC,DC>>,DecorableNode<C,LC,DC,Branch<C,LC,DC>>,Node<C,LC,DC>>()
    );
    branch.set_function("SetLoops", [](Branch<C, LC, DC>* branch, int loops) {
        branch->SetLoops(loops);
    });
    branch.set_function("SetAttempts", [](Branch<C, LC, DC>* branch, int attempts) {
        branch->SetAttempts(attempts);
    });

    LuaRegisterDecorableNode<C, LC, DC, Multiplexer<C, LC, DC>>(nameBase + "Multiplexer", state);
    LuaRegisterBranchNode<C, LC, DC, Multiplexer<C, LC, DC>>(nameBase + "Multiplexer", state);
    sol::usertype<Multiplexer<C, LC, DC>> multiplexer = state.new_usertype<Multiplexer<C, LC, DC>>(
        nameBase + "Multiplexer",
        sol::base_classes, sol::bases<DecorableNode<C,LC,DC,Multiplexer<C,LC,DC>>,Node<C,LC,DC>>()
    );

    LuaRegisterDecorableNode<C, LC, DC, Leaf<C, LC, DC>>(nameBase + "Leaf", state);
    sol::usertype<Leaf<C, LC, DC>> leaf = state.new_usertype<Leaf<C, LC, DC>>(
        nameBase + "Leaf",
        sol::base_classes, sol::bases<DecorableNode<C,LC,DC,Leaf<C,LC,DC>>,Node<C,LC,DC>>()
    );

    sol::usertype<BehaviorTreeContext<C, LC, DC>> context = state.new_usertype<BehaviorTreeContext<C, LC, DC>>(nameBase+"BehaviorTreeContext");
    context.set_function("CreateSequence", &BehaviorTreeContext<C,LC,DC>::CreateSequence);
    context.set_function("CreateSelector", &BehaviorTreeContext<C,LC,DC>::CreateSelector);
    auto createLeaf =
        [](BehaviorTreeContext<C, LC, DC>* ctx, sol::protected_function func)
        {
            return ctx->CreateLeaf([=](C& ctx, LC& lc) {
                return func(ctx, lc);
            });
        };
    auto createMultiplexerA =
        [](BehaviorTreeContext<C, LC, DC>* ctx, sol::protected_function func)
        {
            return ctx->CreateMultiplexer([=](C& ctx, LC& lc) {
                return func(ctx, lc);
                });
        };
    auto createMultiplexerB =
        [](BehaviorTreeContext<C, LC, DC>* ctx)
        {
            return ctx->CreateMultiplexer();
        }
    ;

    context.set_function("CreateLeaf",createLeaf);
    context.set_function("CreateMultiplexer", sol::overload(createMultiplexerA,createMultiplexerB));
    if (globalCtx)
    {
        state.set_function(globalCtxPrefix + "Create " + globalCtxInfix + "Sequence" + globalCtxSuffix,
            [=]() { return globalCtx->CreateSequence(); });
        state.set_function(globalCtxPrefix + "Create" + globalCtxInfix + "Selector" + globalCtxSuffix,
            [=]() { return globalCtx->CreateSelector(); });
        state.set_function(globalCtxPrefix + "Create" + globalCtxInfix + "Leaf" + globalCtxSuffix,
            [=](sol::protected_function func) { return createLeaf(globalCtx,func); });
        state.set_function(globalCtxPrefix + "Create"  + globalCtxInfix + "Multiplexer" + globalCtxSuffix, sol::overload(
            [=](sol::protected_function callback) { return createMultiplexerA(globalCtx,callback); },
            [=]() { return createMultiplexerB(globalCtx); }
        ));
    }
}
