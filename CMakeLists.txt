cmake_minimum_required(VERSION 3.27)
project(pastel C CXX)

set(CMAKE_C_STANDARD 90)

find_package(LLVM REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})

add_executable(pastel src/main.c
        src/lexer/token.c
        include/lexer/token.h
        src/lexer/lexer.c
        include/lexer/lexer.h
        src/util/ptr_list.c
        src/util/ptr_list.h
        src/parser/ast.c
        include/parser/ast.h
        src/parser/parser.c
        include/parser/parser.h
        src/codegen/compiler.c
        include/codegen/compiler.h
        src/codegen/types.h
        src/codegen/utils.c
        src/codegen/utils.h
        src/codegen/casting.c
        src/codegen/casting.h
        src/codegen/expr/binop.c
        src/codegen/expr/binop.h
        src/codegen/expr/expr.c
        src/codegen/expr/expr.h
        src/codegen/expr/value.c
        src/codegen/expr/value.h
        src/codegen/expr/if.c
        src/codegen/expr/if.h
        src/codegen/expr/call.c
        src/codegen/expr/call.h
        src/codegen/stmt/function.c
        src/codegen/stmt/function.h
        src/codegen/stmt/value.c
        src/codegen/stmt/value.h
        src/codegen/stmt/loop.c
        src/codegen/stmt/loop.h
        src/codegen/stmt/stmt.c
        src/codegen/stmt/stmt.h
        src/util/util.h
)

target_include_directories(pastel PUBLIC include)
target_include_directories(pastel PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(pastel LLVM)
