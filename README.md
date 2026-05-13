# CVM++ — A Custom Virtual Machine Language

CVM++ is a bytecode-compiled programming language with its own virtual machine, built from scratch in C++20. It features a complete pipeline: **Lexer → Parser → AST → Compiler → VM**.

## Features

- **Typed variables** — `int`, `long`, `double`, `float`, `bool`, `char`
- **Auto variables** — `auto` (with legacy `let` still accepted for compatibility)
- **Arithmetic** — `+`, `-`, `*`, `/` on numbers
- **Comparisons** — `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Booleans & Nil** — `true`, `false`, `nil`
- **Characters** — single-quoted literals like `'A'` and `'\n'`
- **Unary operators** — `-` (negate), `!` (logical not)
- **Control flow** — `if` / `else`, `while` loops
- **Print** — `print <expression>;`
- **Input** — `input <variable>;` (parses primitive values by declared type, or infers for `auto`)
- **Comments** — `//` single-line comments

## Language Syntax

```
// Variable declaration
int x = 10;
long total = 0;
char initial = 'C';
bool enabled = true;
auto inferred = 5.5;

// Input from user
input x;
input initial;

// Print output
print x;
print initial;

// Arithmetic
int result = (x + 5) * 2 - 1;

// Conditionals
if (x == 0) {
    print 10;
} else {
    print 20;
}

// Loops
while (x < 100) {
    x = x + 1;
}
```

## Build & Run

### Prerequisites

- C++20 compatible compiler (`clang++` or `g++`)
- `make`

### Build

```bash
make
```

### Run

```bash
# Run the default example
./build/cvm

# Run a specific script
./build/cvm examples/example.cvm

# Limit VM execution steps (prevents infinite loops)
./build/cvm --max-steps 10000 examples/example.cvm
```

## Error Reporting

CVM++ provides clear error messages with line numbers and error types:

| Error Type | Example |
|---|---|
| `SyntaxError` | Missing semicolons, unexpected tokens |
| `LexerError` | Unexpected characters |
| `NameError` | Undefined variables |
| `TypeError` | Wrong operand types |
| `MathError` | Division by zero |
| `DeclarationError` | Duplicate variable declarations |
| `InputError` | Invalid primitive input |
| `StackError` | VM stack underflow |
| `OverflowError` | Integer or floating-point overflow |

Example error output:
```
[Line 5] NameError: Undefined variable 'z' on line 5.
```

## Project Structure

```
CVM++/
├── include/cvm/
│   ├── token.h        # Token types and formatting
│   ├── lexer.h        # Lexer (tokenizer) interface
│   ├── ast.h          # AST node definitions
│   ├── parser.h       # Parser interface
│   ├── value.h        # Runtime value types (number, bool, char, nil)
│   ├── bytecode.h     # Opcodes, Chunk, and disassembler
│   ├── compiler.h     # Compiler interface
│   └── vm.h           # Virtual machine interface
├── src/
│   ├── main.cpp       # Entry point and CLI
│   ├── lexer.cpp      # Lexer implementation
│   ├── parser.cpp     # Recursive descent parser
│   ├── compiler.cpp   # AST → bytecode compiler
│   └── vm.cpp         # Stack-based VM executor
├── examples/
│   └── example.cvm    # Example CVM script
└── Makefile
```

## Architecture

```
Source Code (.cvm)
       │
       ▼
    [Lexer]         → Tokens
       │
       ▼
    [Parser]        → AST (Abstract Syntax Tree)
       │
       ▼
    [Compiler]      → Bytecode (Chunk with opcodes)
       │
       ▼
    [VM]            → Execution output
```

The VM is stack-based, using opcodes like `OP_CONSTANT`, `OP_ADD`, `OP_PRINT`, `OP_INPUT`, `OP_JUMP`, etc.
