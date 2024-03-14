cmake_minimum_required(VERSION 3.27)
project(pastel C CXX)

set(CMAKE_C_STANDARD 90)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(pastel src/main.c
        src/lexer/token.c
        src/lexer/token.h
        src/lexer/lexer.c
        src/lexer/lexer.h
        src/util/ptr_list.c
        src/util/ptr_list.h
        src/parser/ast.c
        src/parser/ast.h
        src/parser/parser.c
        src/parser/parser.h
        src/codegen/compiler.c
        src/codegen/compiler.h
)

target_link_libraries(pastel LLVM)
