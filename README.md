<div align="center">

<br>

```
███████╗████████╗ █████╗  ██████╗██╗  ██╗    ██╗   ██╗███╗   ███╗
██╔════╝╚══██╔══╝██╔══██╗██╔════╝██║ ██╔╝    ██║   ██║████╗ ████║
███████╗   ██║   ███████║██║     █████╔╝     ██║   ██║██╔████╔██║
╚════██║   ██║   ██╔══██║██║     ██╔═██╗     ╚██╗ ██╔╝██║╚██╔╝██║
███████║   ██║   ██║  ██║╚██████╗██║  ██╗     ╚████╔╝ ██║ ╚═╝ ██║
╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝     ╚═══╝  ╚═╝     ╚═╝
```

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

Compile a source file into a standalone bytecode file:

```bash
./build/cvm --emit-bytecode program.cvb --no-run path/to/program.cvm
```

Run a previously compiled bytecode file directly on the VM:

```bash
./build/cvm --run-bytecode program.cvb
```

## Inspect Compiler Stages

These flags can be used individually or combined. By default, the VM still runs after printing the requested stage output. Add `--no-run` if you only want inspection output.

| Command | What it shows |
|---|---|
| `./build/cvm --source program.cvm` | Raw source text |
| `./build/cvm --tokens program.cvm` | Lexer token stream |
| `./build/cvm --ast program.cvm` | Parsed AST |
| `./build/cvm --bytecode program.cvm` | Bytecode disassembly |
| `./build/cvm --emit-bytecode program.cvb --no-run program.cvm` | Saves compiled bytecode to a file |
| `./build/cvm --run-bytecode program.cvb` | Loads bytecode from a file and runs it on the VM |
| `./build/cvm --all-stages program.cvm` | Source, tokens, AST, and bytecode |
| `./build/cvm --all-stages --no-run program.cvm` | Full compiler pipeline without executing the VM |

## Strong Demo Flow

```bash
./build/cvm --source --tokens examples/example.cvm
./build/cvm --ast --no-run examples/example.cvm
./build/cvm --bytecode --no-run examples/example.cvm
./build/cvm --emit-bytecode examples/example.cvb --no-run examples/example.cvm
./build/cvm --run-bytecode examples/example.cvb
```

That gives you a clean story:

1. Show the source program.
2. Show lexical analysis.
3. Show the AST.
4. Show the compiled bytecode.
5. Save the bytecode to a file.
6. Run that bytecode file on the VM.

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

## Acknowledgments

- [Crafting Interpreters](https://craftinginterpreters.com/) for outstanding explanations of interpreters, parsing, and language implementation concepts
- [PeachCompiler by nibblebits](https://github.com/nibblebits/PeachCompiler) for reference ideas around compiler structure, parsing flow, and low-level implementation details
- [Compiler Design Course Playlist by Sudhakar Atchala](https://www.youtube.com/watch?v=CcnhLKPmPEM&list=PLXj4XH7LcRfC9pGMWuM6UWE3V4YZ9TZzM) for strong theoretical foundations in compiler construction
- [Creating a Compiler Playlist by Pixeled](https://www.youtube.com/watch?v=vcSijrRsrY0&list=PLUDlas_Zy_qC7c5tCgTMYq2idyyT24lqs) for practical compiler-building walkthroughs and implementation guidance

<div align="center">

**CVM++ is built to be read, demonstrated, and learned from.**

</div>
