cmake_minimum_required(VERSION 3.15)
project(spatter_parser C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=vla)
endif()

include_directories(include)

set(SPATTER_LIB_SOURCES
    src/option.c
    src/result.c
    src/str_view.c
    src/list.c
    src/tree.c
    src/feature.c
    src/decision_tree.c
    src/model.c
    src/beam_search.c
    src/parser.c
    src/treebank.c
    src/eval.c
)

add_library(spatter STATIC ${SPATTER_LIB_SOURCES})

add_executable(spatter_cli src/main.c)
target_link_libraries(spatter_cli PRIVATE spatter)
if(NOT MSVC)
    target_link_libraries(spatter_cli PRIVATE m)
endif()

add_executable(spatter_tests
    tests/test_runner.c
    tests/test_functional_primitives.c
    tests/test_decision_tree.c
    tests/test_spatter_parser.c
)
target_link_libraries(spatter_tests PRIVATE spatter)
if(NOT MSVC)
    target_link_libraries(spatter_tests PRIVATE m)
endif()

enable_testing()
add_test(NAME spatter_all_tests COMMAND spatter_tests)
