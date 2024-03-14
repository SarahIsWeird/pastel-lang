#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Target.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Support.h>

#include "lexer/token.h"
#include "util/ptr_list.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast.h"
#include "codegen/compiler.h"

wchar_t *read_all(const char *path, size_t *size) {
    FILE *file = fopen(path, "r");
    size_t fsize = 0;

    wchar_t *buffer = (wchar_t *) malloc(1);
    wchar_t line_buffer[512];

    while (fgetws(line_buffer, 512, file)) {
        size_t line_length = wcslen(line_buffer);

        wchar_t *new_buffer = (wchar_t *) realloc(buffer, (fsize + line_length + 1) * sizeof(wchar_t));
        if (new_buffer == NULL) {
            fprintf(stderr, "Out of memory!\n");
            free(buffer);
            return NULL;
        }

        buffer = new_buffer;
        wcscpy(buffer + fsize, line_buffer);
        fsize += line_length;
    }

    *size = fsize;

    fclose(file);
    return buffer;
}

int foo(int a) {
    wprintf(L"%d\n", a);
    return a;
}

void print_n(int v) {
    wprintf(L"%d\n", v);
}

void dump_ast(ptr_list_t *top_level_stmts) {
    size_t i;
    for (i = 0; i < ptr_list_size(top_level_stmts); i++) {
        stmt_t *stmt = (stmt_t *) ptr_list_at(top_level_stmts, i);
        print_stmt(stmt, 0);
    }

    wprintf(L"\n");
}

int run_jit(compiler_t *compiler) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmParser();
    LLVMInitializeNativeAsmPrinter();

    LLVMLinkInMCJIT();

    struct LLVMMCJITCompilerOptions options;
    LLVMInitializeMCJITCompilerOptions(&options, sizeof(options));

    LLVMExecutionEngineRef jit;
    char *error;
    LLVMBool res = LLVMCreateMCJITCompilerForModule(
            &jit,
            compiler_get_module(compiler),
            &options,
            sizeof(options),
            &error
    );

    if (res) {
        fprintf(stderr, "Error creating JIT: %s\n", error);
        exit(1);
    }

    LLVMAddSymbol("foo", foo);
    LLVMAddSymbol("print_n", print_n);

    int result = LLVMRunFunctionAsMain(jit, compiler_get_main(compiler), 0, NULL, NULL);

    if (!compiler_is_main_void(compiler)) {
        wprintf(L"Result: %d\n", result);
    }

    return result;
}

int main() {
    size_t size;
    wchar_t *test = read_all("test/test.pstl", &size);

    lexer_t *lexer = lexer_new(test, size);
    lexer_lex_all(lexer);

    ptr_list_t *tokens = lexer_get_tokens(lexer);

    parser_t *parser = parser_new(tokens);
    ptr_list_t *top_level_stmts = parser_parse_all(parser);

    dump_ast(top_level_stmts);

    compiler_t *compiler = compiler_new(top_level_stmts, OPT_ALL);
    if (compiler_compile(compiler)) {
        return 1;
    }

    compiler_dump_all(compiler, 1);

    run_jit(compiler);

    ptr_list_free(top_level_stmts);
    ptr_list_free(tokens);

    return 0;
}
