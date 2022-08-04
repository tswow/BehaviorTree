cmake_minimum_required(VERSION 3.12)
include(FetchContent)
Project(BehaviorTree)

set(BT_BUILD_TESTS ON CACHE BOOL "If tests should be built BehaviorTree")

add_library(BehaviorTree INTERFACE src/BehaviorTree.h src/BehaviorTree.ipp)

set_target_properties(BehaviorTree PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src"
    CXX_STANDARD 17
)

if(${BT_BUILD_TESTS})
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.0.0-preview3
    )
    FetchContent_MakeAvailable(Catch2)
    add_executable(BehaviorTreeTests test/BehaviorTree.cpp)
    target_link_libraries(BehaviorTreeTests PRIVATE Catch2::Catch2WithMain BehaviorTree)
    set_target_properties(BehaviorTreeTests PROPERTIES CXX_STANDARD 17)
endif()