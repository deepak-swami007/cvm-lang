<div align="center">

# ⚡ CVM++
**A High-Performance Custom Virtual Machine & Bytecode Compiler**

[![C++20](https://img.shields.io/badge/C++-20-blue.svg?style=for-the-badge&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/compiler_support)
[![CMake](https://img.shields.io/badge/CMake-3.10+-green.svg?style=for-the-badge&logo=cmake)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

*An end-to-end language pipeline built from scratch with strict typing, robust memory safety, and lightning-fast execution.*

</div>

---

## 📖 Table of Contents
- [Philosophy](#-philosophy)
- [Architecture](#-architecture)
- [Language Specification](#-language-specification)
  - [Type System](#type-system)
  - [Operators & Operations](#operators--operations)
  - [Control Flow](#control-flow)
- [Safety & Error Handling](#-safety--error-handling)
- [Getting Started](#-getting-started)

---

## 🎯 Philosophy

CVM++ was born out of a desire to build a complete, uncompromising language pipeline from the ground up. By circumventing existing interpreter frameworks and LLVM, CVM++ offers a pure look into compiler design. It enforces strict type safety at the bytecode level, prioritizing developer predictability and bounded execution over implicit magic. 

---

## 🏗 Architecture

CVM++ processes source code through a heavily optimized, five-stage pipeline:

1. **Lexical Analysis (Lexer):** Converts raw character streams into semantic tokens.
2. **Parsing (Parser):** Assembles tokens into an Abstract Syntax Tree (AST) using a recursive descent algorithm.
3. **AST Validation:** Enforces syntax rules and scoping constraints prior to compilation.
4. **Bytecode Compilation (Compiler):** Translates the AST into flattened, highly optimized opcode sequences (Chunks).
5. **Execution (Virtual Machine):** A stack-based execution engine that strictly manages primitive types, handles short-circuit evaluation, and executes the final bytecode.

---

## ⚡ Language Specification

CVM++ is statically typed at the declaration level, with extensive support for dynamic inference during initialization. 

### Type System
The language supports a rich set of primitives, complete with hardware-level boundary checking:
- **`int`** — 32-bit signed integer.
- **`long`** — 64-bit signed integer.
- **`float`** — 32-bit floating point.
- **`double`** — 64-bit floating point.
- **`bool`** — Boolean logic.
- **`char`** — Single-byte characters.
- **`auto` / `let`** — Runtime type inference based on assignment value.
- **`nil`** — Safe uninitialized state representation.

### Operators & Operations
- **Mathematical:** Standard arithmetic, modulo, and exponentiation.
- **Bitwise:** Low-level integer manipulation (AND, OR, NOT).
- **Logical:** Short-circuiting boolean operations.
- **Relational:** Comprehensive equality and magnitude comparisons.
- **Assignment:** Standard and compound assignment operators for concise mutations.

### Control Flow
- **Conditionals:** Standard branching logic.
- **Iterators:** Pre-condition loops and fully featured counter loops.
- **Modifiers:** Mid-loop interruption and iteration skipping mechanisms.
- **I/O Integration:** Native standard output streams and strictly typed standard input prompts.

---

## 🛡 Safety & Error Handling

CVM++ doesn't just fail; it explains *why*. The VM includes deep runtime bounds checking to prevent hardware faults.

- **Syntax & Lexer Analytics:** Catch malformed expressions instantly.
- **Memory & Stack Protections:** Prevents stack underflows and bounds violations at the opcode level.
- **Mathematical Boundaries:** Strict guards against division/modulo by zero.
- **Overflow Guards:** Hardline protections against integer wraps and floating-point explosions.
- **Type Coercion Guards:** Prevents unsafe implicit downcasts that result in precision loss.
- **Input Validation:** Halts execution gracefully when standard input violates the declared variable type.

---

## 🚀 Getting Started

### Prerequisites
To compile CVM++, ensure your environment meets the modern C++ standard:
- **C++20** compatible compiler (`clang++`, `g++`, or `MSVC`).
- **CMake** (version 3.10 or higher).

### Build Instructions

Execute the following commands from the project root to compile the virtual machine:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Execution

Launch the compiled binary to execute your scripts.

```bash
# Run the Interactive REPL
./build/cvm

# Execute a Script File
./build/cvm path/to/script.cvm

# Execute with Infinite-Loop Protections (Step Limits)
./build/cvm --max-steps 10000 path/to/script.cvm
```

---

<div align="center">
<i>Built with precision and passion.</i>
</div>
