# c_compiler

A learning compiler written in C.

## Build

    make            # debug build
    make BUILD_TYPE=release
    make test
    make clean

Output binary: `build/compiler`

## Layout

- `src/main.c` — entry point: reads the source file, drives the pipeline
- `src/lexer.c/h` — tokenizer: source text → tokens
- future stages: `parser.c` (tokens → AST), `codegen.c` (AST → assembly)

## Usage

    ./build/compiler program.c
