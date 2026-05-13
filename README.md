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

### A High-Performance Custom Virtual Machine & Bytecode Compiler

*Built completely from scratch — no LLVM, no interpreter frameworks, no shortcuts.*

<br>

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/compiler_support/20)
[![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-F7DF1E?style=for-the-badge)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey?style=for-the-badge)]()
[![Status](https://img.shields.io/badge/Status-Active-brightgreen?style=for-the-badge)]()

<br>

> *"The best way to understand how programming languages work is to build one yourself."*

<br>

[What is CVM++](#-what-is-cvm) · [Architecture](#%EF%B8%8F-architecture) · [Language](#-language-specification) · [Safety](#-safety--error-handling) · [Getting Started](#-getting-started)

<br>

</div>

---

## 🤔 What is CVM++?

CVM++ is a **complete, end-to-end programming language implementation** — written in modern C++20. It takes source code you write, chews through it across five distinct stages, and executes it on a custom-built virtual machine.

No LLVM. No Flex/Bison. No shortcuts. Every single component — the lexer, the parser, the AST validator, the bytecode compiler, and the VM itself — was designed and built by hand.

This project exists because the best way to truly understand how languages work under the hood isn't to read about them — it's to build one. CVM++ is the result of that obsession.

**What makes it stand out:**

- ⚡ **Stack-based VM** with a tightly optimized opcode dispatch loop
- 🔒 **Strict static typing** with smart `auto`/`let` inference at declaration time
- 🛡 **Deep runtime safety** — overflow guards, division-by-zero checks, type coercion protection, and more
- 🧠 **Short-circuit evaluation** natively handled inside the VM
- 📖 **Human-readable errors** — when things go wrong, you get a real explanation, not a cryptic crash

---

## 🏗️ Architecture

CVM++ processes source code through a **five-stage pipeline**, where each stage has a single, well-defined responsibility. No stage knows too much about the others.

```
Source Code (.cvm)
       │
       ▼
┌─────────────────┐
│   LEXER         │  Raw characters → Semantic tokens
│                 │  Handles keywords, literals, operators,
│                 │  identifiers, and whitespace stripping
└────────┬────────┘
         │  Token Stream
         ▼
┌─────────────────┐
│   PARSER        │  Tokens → Abstract Syntax Tree (AST)
│                 │  Recursive descent — no parser generators,
│                 │  written entirely by hand
└────────┬────────┘
         │  Raw AST
         ▼
┌─────────────────┐
│  AST VALIDATOR  │  Enforces scoping rules, declaration
│                 │  constraints, and syntactic legality
│                 │  before a single byte of code is emitted
└────────┬────────┘
         │  Validated AST
         ▼
┌─────────────────┐
│   COMPILER      │  AST → Flattened opcode sequences (Chunks)
│                 │  Resolves variables, handles jumps,
│                 │  and optimizes the output bytecode
└────────┬────────┘
         │  Bytecode (Chunks)
         ▼
┌─────────────────┐
│ VIRTUAL MACHINE │  Executes bytecode on a stack-based engine
│                 │  Manages types, handles I/O, enforces
│                 │  all runtime safety guarantees
└─────────────────┘
         │
         ▼
    Program Output
```

Every stage fails fast and loudly. If your code is malformed, you'll know exactly where and why before it ever reaches the VM.

---

## ⚡ Language Specification

CVM++ is **statically typed at declaration** with support for **runtime type inference** via `auto` and `let`. The language is intentionally minimal but expressive — everything is a deliberate design choice.

---

### Type System

The type system maps directly to hardware primitives. There's no hidden boxing, no garbage-collected heap objects masquerading as values.

| Type | Description | Bit Width |
|------|-------------|-----------|
| `int` | Signed integer | 32-bit |
| `long` | Signed integer | 64-bit |
| `float` | Floating point | 32-bit (IEEE 754) |
| `double` | Floating point | 64-bit (IEEE 754) |
| `bool` | Boolean | — |
| `char` | Single character | 8-bit |
| `auto` / `let` | Type inferred from assignment | — |
| `nil` | Safe uninitialized state | — |

> **No implicit lossy downcasts.** Assigning a `double` to an `int` without an explicit cast is a runtime error, not a silent truncation.

---

### Operators & Operations

**Mathematical**
Standard arithmetic (`+`, `-`, `*`, `/`), modulo (`%`), and exponentiation (`**`).

**Bitwise**
Low-level integer manipulation: AND (`&`), OR (`|`), NOT (`~`).

**Logical**
Boolean logic with proper short-circuit evaluation: `&&`, `||`, `!`.
The VM halts evaluation of the right-hand operand whenever the result is already determined by the left — this is enforced at the bytecode level, not just at the AST level.

**Relational**
Full comparison suite: `==`, `!=`, `<`, `>`, `<=`, `>=`.

**Assignment**
Standard (`=`) and compound (`+=`, `-=`, `*=`, `/=`, `%=`) assignment operators for concise in-place mutation.

---

### Control Flow

**Conditionals**
Standard `if` / `else if` / `else` branching. Condition expressions are fully evaluated with type checking before any branch is taken.

**Loops**
- Pre-condition `while` loops — evaluate condition, enter body, repeat.
- Counter-based `for` loops — full initializer, condition, and increment expressions.

**Loop Modifiers**
- `break` — immediately exits the enclosing loop.
- `continue` — skips the remaining body of the current iteration and re-evaluates the condition.

**I/O**
- `print` — native standard output stream.
- `input` — strictly typed standard input. If the user types a `string` when an `int` is expected, the VM halts gracefully with a descriptive error. No unchecked `scanf` behavior.

---

## 🛡 Safety & Error Handling

CVM++ doesn't just crash. It tells you *what went wrong*, *where*, and *why*. Runtime safety is not an afterthought — it's baked into every layer of the pipeline.

### Compile-Time Protections

| Check | What It Catches |
|---|---|
| **Lexer Analytics** | Malformed tokens, unterminated strings, illegal characters |
| **Parser Validation** | Unexpected token sequences, malformed expressions |
| **Scope Enforcement** | Undeclared variables, out-of-scope references |
| **Type Coercion Guards** | Implicit narrowing conversions that would silently lose precision |

### Runtime Protections

| Check | What It Catches |
|---|---|
| **Stack Underflow Guard** | Pops on an empty stack — would be a silent segfault elsewhere |
| **Bounds Violation Detection** | Out-of-range memory/stack access at the opcode level |
| **Division / Modulo by Zero** | Strict pre-check before every `/` and `%` operation |
| **Integer Overflow Guard** | Wrapping arithmetic that would silently corrupt values |
| **Float Explosion Guard** | Detects `Inf` and `NaN` results before they propagate |
| **Input Type Mismatch** | Halts when stdin input violates the declared variable's type |

> There is no concept of "undefined behavior" hiding in the VM. Every exceptional condition has an explicit check and a human-readable error message.

---

## 🚀 Getting Started

### Prerequisites

| Requirement | Version |
|---|---|
| C++ Compiler (`g++`, `clang++`, or MSVC) | C++20 support required |
| CMake | 3.10 or higher |

Verify your compiler supports C++20:
```bash
g++ --version        # GCC 10+ for full C++20
clang++ --version    # Clang 10+ for full C++20
```

---

### Building

Clone the repository and build with CMake:

```bash
git clone https://github.com/yourusername/cvmpp.git
cd cvmpp

mkdir build
cd build
cmake ..
cmake --build .
```

The compiled binary will be located at `./build/cvm`.

---

### Running CVM++

```bash
# Launch the interactive REPL (great for quick tests)
./build/cvm

# Execute a .cvm script file
./build/cvm path/to/your/script.cvm

# Execute with a step limit (protects against infinite loops)
./build/cvm --max-steps 10000 path/to/your/script.cvm
```

> **Tip:** Use `--max-steps` during development to catch accidental infinite loops early. The VM will halt and report how many steps were executed before the limit was hit.

---

### Project Structure

```
cvmpp/
├── src/
│   ├── lexer/          # Tokenization stage
│   ├── parser/         # AST construction (recursive descent)
│   ├── validator/      # AST scoping & syntax rules
│   ├── compiler/       # Bytecode emission (Chunks)
│   └── vm/             # Stack-based execution engine
├── include/            # Shared headers
├── scripts/            # Utility & test scripts
├── CMakeLists.txt
└── README.md
```

---



## 🙏 Acknowledgments

This project was heavily inspired by the learning philosophy behind crafting interpreters from the ground up. Special thanks to:

- [**Crafting Interpreters** by Robert Nystrom](https://craftinginterpreters.com/) — the gold standard for learning this stuff
- The C++ standards committee — for giving us `std::variant` and `std::optional`, which made type dispatch actually bearable


---

<div align="center">

<br>

**If this project helped you learn something, drop a ⭐.**

<br>

*Built with stubbornness, coffee, and an unreasonable amount of passion for how computers actually work.*

<br>

[![forthebadge](https://forthebadge.com/images/badges/built-with-love.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/made-with-c-plus-plus.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/works-on-my-machine.svg)](https://forthebadge.com)

</div>
