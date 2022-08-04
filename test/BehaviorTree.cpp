#include "BehaviorTree.h"

#include <catch2/catch_test_macros.hpp>

#include <iostream>

using TP = std::vector<uint32_t>;
#define VERIFY_VEC(v1,v2) REQUIRE(v1 == TP(v2))

using MS = std::monostate;

TEST_CASE("Leaf without delay") {
    BehaviorTreeContext<TP> ctx;
    Leaf<TP>* leaf = ctx.CreateLeaf([](TP& vec,MS&){ vec.push_back(0); return 0; });
    TreeExecutor<TP> exec(&ctx,leaf);
    TP vec;

    SECTION("Called once") {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0 }));
    }

    SECTION("Called twice") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,0 }));
    }
}

TEST_CASE("Leaf with delay") {
    BehaviorTreeContext<TP> ctx;
    Leaf<TP>* leaf = ctx.CreateLeaf([](TP& vec,MS&) { vec.push_back(0); return 1; });
    TreeExecutor<TP> exec(&ctx,leaf);
    TP vec;

    SECTION("Called once") {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0 }));
    }

    SECTION("Called twice without delay") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0 }));
    }

    SECTION("Called twice with delay") {
        exec.Update(vec, 0);
        exec.Update(vec, 1);
        REQUIRE(vec == TP({ 0,0 }));
    }

    SECTION("Called twice with overflowing delay") {
        exec.Update(vec, 0);
        exec.Update(vec, 4);
        REQUIRE(vec == TP({ 0,0 }));
    }
}

TEST_CASE("Decorator without delay") {
    BehaviorTreeContext<TP> ctx;
    Leaf<TP>* leaf = ctx.CreateLeaf([](TP& vec,MS&) { return 0; });
    leaf->decorate([](TP& vec,MS&) { vec.push_back(0); return 0; });
    TreeExecutor<TP> exec(&ctx,leaf);
    TP vec;

    SECTION("Called once") {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0 }));
    }

    SECTION("Called twice") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,0 }));
    }
}

TEST_CASE("Decorator with delay") {
    BehaviorTreeContext<TP> ctx;
    Leaf<TP>* leaf = ctx.CreateLeaf([](TP& vec,MS&) { return 0; });
    leaf->decorate([](TP& vec,MS&) { vec.push_back(0); return 1; });
    TreeExecutor<TP> exec(&ctx,leaf);
    TP vec;

    SECTION("Called once") {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0 }));
    }

    SECTION("Called twice without delay") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0 }));
    }

    SECTION("Called twice with delay") {
        exec.Update(vec, 0);
        exec.Update(vec, 2);
        REQUIRE(vec == TP({ 0,0 }));
    }
}

TEST_CASE("Leaf + decorator")
{
    BehaviorTreeContext<TP> ctx;
    Leaf<TP>* leaf = ctx.CreateLeaf([](TP& vec,MS&) { vec.push_back(0); return 0; });
    leaf->decorate([](TP& vec,MS&) { vec.push_back(1); return 0; });
    TreeExecutor<TP> exec(&ctx,leaf);
    TP vec;

    SECTION("Called once") {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 1,0 }));
    }

    SECTION("Called twice") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 1,0,1,0 }));
    }
}

TEST_CASE("Leaf + decorator + decorator")
{
    BehaviorTreeContext<TP> ctx;
    Leaf<TP>* leaf = ctx.CreateLeaf([](TP& vec,MS&) { vec.push_back(0); return 0; });
    leaf->decorate([](TP& vec,MS&) { vec.push_back(1); return 0; });
    leaf->decorate([](TP& vec,MS&) { vec.push_back(2); return 0; });
    TreeExecutor<TP> exec(&ctx,leaf);
    TP vec;

    SECTION("Called once") {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 1,2,0 }));
    }

    SECTION("Called twice") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 1,2,0,1,2,0 }));
    }
}

TEST_CASE("Sequence + Leaf(Success) + Leaf(1->Success)")
{
    BehaviorTreeContext<TP> ctx;
    
    Branch<TP>* branch = ctx.CreateSequence();
    branch->AddLeaf([](TP& vec,MS&) { vec.push_back(0); return Result::SUCCESS; });
    branch->AddLeaf([](TP& vec,MS&) { vec.push_back(1); return vec.size() == 3 ? Result::SUCCESS : 1; });
    TreeExecutor<TP> exec(&ctx,branch);
    TP vec;

    SECTION("Called once")
    {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,1 }));
    }

    SECTION("Called twice without delay")
    {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,1 }));
    }

    SECTION("Called twice with delay")
    {
        exec.Update(vec, 0);
        exec.Update(vec, 1);
        REQUIRE(vec == TP({ 0,1,1 }));
        REQUIRE(exec.NodeStackDepth() == 0);
    }
}

TEST_CASE("Selector + Leaf(FAILURE->SUCCESS) + Leaf(SUCCESSS)")
{
    BehaviorTreeContext<TP> ctx;
    Branch<TP>* branch = ctx.CreateSelector();
    branch->AddLeaf([](TP& vec,MS&) { vec.push_back(0); return vec.size() > 1 ? Result::SUCCESS : Result::FAILURE; });
    branch->AddLeaf([](TP& vec,MS&) { vec.push_back(1); return Result::SUCCESS; });
    TreeExecutor<TP> exec(&ctx,branch);
    TP vec;

    SECTION("Called once")
    {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,1 }));
        REQUIRE(exec.NodeStackDepth() == 0);
    }

    SECTION("Called twice")
    {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,1,0 }));
        REQUIRE(exec.NodeStackDepth() == 0);
    }
}

TEST_CASE("Deep decorators")
{
    BehaviorTreeContext<TP> ctx;
    Branch<TP>* root = ctx.CreateSequence()
        ->AddSequence([](Branch<TP>* builder) { builder
            ->decorate([](TP& vec,MS&) { vec.push_back(0); return 0; })
            ->AddLeaf([](TP& vec,MS&) { vec.push_back(1); return 0; })
        ;})
        ->decorate([](TP& vec,MS&) { vec.push_back(2); return 0; })
        ;
    TreeExecutor<TP> exec(&ctx,root);
    TP vec;

    SECTION("Called once") {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 2,0,1 }));
    }

    SECTION("Called twice") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 2,0,1,2,0,1 }));
    }
}

TEST_CASE("Deep decorator leaf unload") {
    BehaviorTreeContext<TP> ctx;
    Branch<TP>* root = ctx.CreateSequence()
        ->AddSequence([](Branch<TP>* builder) { builder
            ->decorate([](TP& vec,MS&) { vec.push_back(0); return 0; })
            ->AddLeaf([](TP& vec,MS&) { vec.push_back(1); return Result::SUCCESS; })
            ; })
        ->decorate([](TP& vec,MS&) { vec.push_back(2); return 0; })
        ->decorate([](TP& vec,MS&) { vec.push_back(3); return 0; })
        ;
    TreeExecutor<TP> exec(&ctx,root);
    TP vec;

    SECTION("Called twice") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 2,3,0,1,2,3,0,1 }));
    }
}

TEST_CASE("Deep decorator unload") {
    BehaviorTreeContext<TP> ctx;
    Branch<TP>* root = ctx.CreateSequence()
        ->AddSequence([](Branch<TP>* builder) { builder
            ->decorate([](TP& v,MS&) { v.push_back(0); return 0; })
            ->AddLeaf([](TP& v,MS&) { v.push_back(1); return Result::SUCCESS; })
            ; })
        ->AddSequence([](Branch<TP>* builder) { builder
            ->AddLeaf([](TP& v,MS&) { v.push_back(2); return 0; })
            ; })
        ->decorate([](TP& v,MS&) { v.push_back(3); return 0; })
        ;

    TreeExecutor<TP> exec(&ctx,root);
    TP vec;

    SECTION("Called twice") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 3,0,1,2,3,2 }));
    }
}

TEST_CASE("Sequence Loops") {
    int counter = 0;
    BehaviorTreeContext<int&> ctx;
    Branch<int&>* root = ctx.CreateSequence()
        ->AddSequence([](Branch<int&>* builder) { builder
            ->set_loops(25)
            ->AddLeaf([](int& v,MS&) { v++; return Result::SUCCESS; })
            ; })
        ;
    TreeExecutor<int&> exec(&ctx,root);
    exec.Update(counter, 0);
    REQUIRE(counter == 25);
}

TEST_CASE("Sequence attempts") {
    int counter = 0;
    BehaviorTreeContext<int&> ctx;
    Branch<int&> * root = ctx.CreateSequence()
        ->AddSequence([](Branch<int&>* builder) { builder
            ->set_attempts(25)
            ->AddLeaf([](int& v,MS&) { v++; return Result::FAILURE; })
            ; })
        ;
    TreeExecutor<int&> exec(&ctx,root);
    exec.Update(counter, 0);
    REQUIRE(counter == 25);
}

TEST_CASE("Multiplexers") {
    BehaviorTreeContext<TP> ctx;
    Multiplexer<TP>* root = ctx.CreateMultiplexer()
        ->AddLeaf([](TP& v,MS&) { v.push_back(0); return 0; })
        ->AddLeaf([](TP& v,MS&) { v.push_back(1); return 0; })
    ;
    TreeExecutor<TP> exec(&ctx,root);
    TP vec;

    SECTION("Called once") {
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,1 }));
    }

    SECTION("Called twice") {
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,1,0,1 }));
    }
}

struct InitInteger
{
    int i = 0;
};
TEST_CASE("Leaf state") {
    BehaviorTreeContext<TP,InitInteger> ctx;
    TP vec;

    SECTION("Clears after reset") {
        auto leaf = ctx.CreateLeaf([](TP& v, InitInteger& i) {
            v.push_back(i.i++);
            return Result::SUCCESS;
        });
        TreeExecutor<TP,InitInteger> exec(&ctx,leaf);
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,0 }));
    }

    SECTION("Stays without reset") {
        auto leaf = ctx.CreateLeaf([](TP& v, InitInteger& i) {
            v.push_back(i.i++);
            return 0;
        });
        TreeExecutor<TP, InitInteger> exec(&ctx,leaf);
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,1 }));
    }
}

TEST_CASE("Decorator state") {
    BehaviorTreeContext<TP, MS, InitInteger> ctx;
    TP vec;
    auto leaf = ctx.CreateLeaf([](TP& v, MS&) { return 0; });
    TreeExecutor<TP, MS, InitInteger> exec(&ctx,leaf);
    SECTION("Clears after reset") {
        leaf->decorate([](TP& v, InitInteger& i) { v.push_back(i.i++); return Result::FAILURE; });
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,0 }));
    }

    SECTION("Stays without reset") {
        leaf->decorate([](TP& v, InitInteger& i) { v.push_back(i.i++); return 0; });
        exec.Update(vec, 0);
        exec.Update(vec, 0);
        REQUIRE(vec == TP({ 0,1 }));
    }
}
