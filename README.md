# CVM++

<div align="center">

### A custom C++20 language pipeline with a handwritten lexer, parser, compiler, and virtual machine

*Source code in, bytecode out, execution on a stack-based VM.*

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/compiler_support/20)
[![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org/)
[![Status](https://img.shields.io/badge/Status-Working-success?style=for-the-badge)]()
[![VM](https://img.shields.io/badge/Runtime-Bytecode%20VM-black?style=for-the-badge)]()

</div>

## Jump To

[What This Project Is](#what-this-project-is) •
[Why It Stands Out](#why-it-stands-out) •
[Execution Pipeline](#execution-pipeline) •
[Language Snapshot](#language-snapshot) •
[Build](#build) •
[Run](#run) •
[Inspect Compiler Stages](#inspect-compiler-stages) •
[Strong Demo Flow](#strong-demo-flow) •
[Repository Layout](#repository-layout) •
[Documentation](#documentation) •
[Notes On Accuracy](#notes-on-accuracy) •
[Acknowledgments](#acknowledgments)

## What This Project Is

CVM++ is a complete end-to-end programming language implementation written in modern C++20. It takes a `.cvm` source file, tokenizes it, parses it into an AST, compiles that AST into bytecode, and executes the bytecode on a custom virtual machine.

Everything important is implemented by hand:

- Lexer
- Recursive-descent parser
- AST model
- Bytecode compiler
- Stack-based VM
- Runtime checks and error reporting

This repository is ideal for demonstrating how compilers and virtual machines actually work in practice, because the full pipeline is visible and easy to trace.

## Why It Stands Out

- Built from scratch without LLVM, parser generators, or interpreter frameworks
- Clear separation between lexical analysis, parsing, compilation, and execution
- Typed variable declarations with `let` / `auto` inference support
- Control flow support for `if`, `else`, `while`, `for`, `break`, and `continue`
- Bytecode disassembly support for showing the compiled program
- Safe runtime behavior with checks for overflow, bad input, division by zero, and invalid operations
- New stage-dump CLI flags for demoing tokens, AST, and bytecode directly from the terminal

## Execution Pipeline

```text
.cvm source
   |
   v
[Lexer] ----> token stream
   |
   v
[Parser] ---> abstract syntax tree
   |
   v
[Compiler] -> bytecode chunk
   |
   v
[VM] -------> program output
```

## Language Snapshot

| Area | Support |
|---|---|
| Types | `int`, `long`, `long long`, `float`, `double`, `bool`, `char`, `let`, `auto`, `nil` |
| Literals | Integers, decimal numbers, booleans, `nil`, character literals |
| Arithmetic | `+`, `-`, `*`, `/`, `%`, `^` |
| Comparison | `==`, `!=`, `<`, `<=`, `>`, `>=` |
| Logical | `&&`, `||`, `!` |
| Bitwise | `&`, `|`, `~` |
| Assignment | `=`, `+=`, `-=`, `*=`, `/=` |
| Statements | Variable declarations, expression statements, `print`, `input`, blocks |
| Control flow | `if`, `else`, `while`, `for`, `break`, `continue` |
| Debug views | Source, token stream, AST, bytecode disassembly |

## Build

```bash
cmake -S . -B build
cmake --build build
```

The executable is produced at:

```bash
./build/cvm
```

Optional sanitizer build:

```bash
cmake -S . -B build -DENABLE_SANITIZERS=ON
cmake --build build
```

## Run

This project executes `.cvm` source files. For reliable use, pass the script path explicitly.

```bash
./build/cvm path/to/program.cvm
```

Limit execution steps to guard against infinite loops:

```bash
./build/cvm --max-steps 10000 path/to/program.cvm
```

Show CLI help:

```bash
./build/cvm --help
```

## Inspect Compiler Stages

These flags can be used individually or combined. By default, the VM still runs after printing the requested stage output. Add `--no-run` if you only want inspection output.

| Command | What it shows |
|---|---|
| `./build/cvm --source program.cvm` | Raw source text |
| `./build/cvm --tokens program.cvm` | Lexer token stream |
| `./build/cvm --ast program.cvm` | Parsed AST |
| `./build/cvm --bytecode program.cvm` | Bytecode disassembly |
| `./build/cvm --all-stages program.cvm` | Source, tokens, AST, and bytecode |
| `./build/cvm --all-stages --no-run program.cvm` | Full compiler pipeline without executing the VM |

## Strong Demo Flow

If you need to record a demo video for submission, this sequence shows the project extremely well:

```bash
./build/cvm --source --tokens sample.cvm
./build/cvm --ast --no-run sample.cvm
./build/cvm --bytecode --no-run sample.cvm
./build/cvm sample.cvm
```

That gives you a clean story:

1. Show the source program.
2. Show lexical analysis.
3. Show the AST.
4. Show the compiled bytecode.
5. Run the program on the VM.

## Repository Layout

```text
CVM++/
├── include/cvm/
│   ├── ast.h
│   ├── bytecode.h
│   ├── compiler.h
│   ├── lexer.h
│   ├── parser.h
│   ├── token.h
│   ├── type.h
│   ├── value.h
│   └── vm.h
├── src/
│   ├── compiler.cpp
│   ├── lexer.cpp
│   ├── main.cpp
│   ├── parser.cpp
│   └── vm.cpp
├── CMakeLists.txt
├── PROJECT_REFERENCE.md
├── README.md
└── Crafting Interpreters PDF.pdf
```

## Documentation

For full submission-ready documentation, see:

- [PROJECT_REFERENCE.md](./PROJECT_REFERENCE.md) for build steps, usage instructions, stage commands, supported functionality, grammar, and current limitations

## Notes On Accuracy

This README is intentionally aligned to the code that exists in the repository today. It does not assume features that are not implemented. In particular:

- The project is a file-based compiler/VM runner, not an interactive REPL
- The current language does not yet implement user-defined functions
- The current runtime is centered around globals and a single bytecode chunk

## Acknowledgments

- [Crafting Interpreters](https://craftinginterpreters.com/) for the learning philosophy behind building language tools from scratch
- Modern C++20 for `std::variant`, `std::optional`, and the rest of the toolkit that made a clean implementation possible

<div align="center">

**CVM++ is built to be read, demonstrated, and learned from.**

</div>
