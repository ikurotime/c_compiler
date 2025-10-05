# Husk

A simple programming language compiler built with C++ and LLVM.

## Features

- Integer arithmetic with `+` operator
- `print()` function to output results
- Compiles to LLVM IR (cross-platform)

## Syntax

```c
print(10 + 4);  // Outputs: 14
```

## Prerequisites

- CMake 3.20+
- C++23 compiler
- LLVM 18+

Install LLVM on macOS:
```bash
brew install llvm
export LLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm
```

## Build

```bash
# Clean and configure
rm -rf build
cmake -S . -B build

# Build
cmake --build ./build/
```

## Usage

```bash
# Compile Husk source to LLVM IR
./build/husk ./main.hsk

# Compile LLVM IR to native executable
clang out.ll -o out

# Run the executable
./out
```

## Example

**main.hsk:**
```c
print(10 + 4);
```

**Output:**
```
14
```

## Architecture

```
Husk Source (.hsk)
  ↓ Tokenizer
Tokens
  ↓ Parser
AST (Abstract Syntax Tree)
  ↓ CodeGen (LLVM)
LLVM IR (.ll)
  ↓ clang/llc
Native Executable
```

## Current Language Features

- **Arithmetic**: `+` (addition)
- **Print**: `print(expression);`
- **Integer literals**
- **Expressions**: Simple binary operations

## Roadmap

- [ ] More operators (`-`, `*`, `/`)
- [ ] Variables (`let x = 5;`)
- [ ] Functions
- [ ] Control flow (if/else, loops)
