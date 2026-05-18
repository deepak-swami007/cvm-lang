# CVM++ Project Reference

## Purpose Of This File

This document is the detailed companion to `README.md`. It is meant to be submission-friendly and presentation-friendly, and it covers:

- How to build and run the project
- What each command-line option does
- What functionality the language currently supports
- The grammar accepted by the parser
- How to print lexer, parser, and bytecode stages
- What limitations still exist in the current implementation

## 1. Build Instructions

Configure and build the project:

```bash
cmake -S . -B build
cmake --build build
```

Run the executable:

```bash
./build/cvm path/to/program.cvm
```

Optional sanitizer-enabled build:

```bash
cmake -S . -B build -DENABLE_SANITIZERS=ON
cmake --build build
```

## 2. CLI Command Reference

Use `--help` at any time:

```bash
./build/cvm --help
```

### Core commands

```bash
./build/cvm program.cvm
./build/cvm --max-steps 10000 program.cvm
```

### Stage inspection commands

```bash
./build/cvm --source program.cvm
./build/cvm --tokens program.cvm
./build/cvm --ast program.cvm
./build/cvm --bytecode program.cvm
./build/cvm --all-stages program.cvm
./build/cvm --all-stages --no-run program.cvm
```

### Option summary

| Option | Meaning |
|---|---|
| `--source` | Prints the source file before compilation |
| `--tokens` | Prints the lexer token stream |
| `--ast` | Prints the parser output as an AST string |
| `--bytecode` | Prints the compiled bytecode disassembly |
| `--all-stages` | Enables `--source`, `--tokens`, `--ast`, and `--bytecode` together |
| `--no-run` | Stops after compilation/debug output without running the VM |
| `--max-steps N` | Caps VM instruction count; `0` disables the limit |
| `--help` / `-h` | Shows usage help |

## 3. Recommended Demo Sequence

If you need to demonstrate compilation and execution for a report or video, this is the cleanest sequence:

```bash
./build/cvm --source --tokens sample.cvm
./build/cvm --ast --no-run sample.cvm
./build/cvm --bytecode --no-run sample.cvm
./build/cvm sample.cvm
```

This demonstrates:

1. The original source program
2. Tokenization
3. AST construction
4. Bytecode generation
5. VM execution

## 4. Architecture Overview

The project follows a classic compiler + VM pipeline:

```text
Source file
  -> Lexer
  -> Token stream
  -> Parser
  -> AST
  -> Compiler
  -> Bytecode chunk
  -> Virtual machine
  -> Program output
```

### Source files involved

| File | Role |
|---|---|
| `src/lexer.cpp` | Converts source text into tokens |
| `src/parser.cpp` | Builds the AST with recursive descent |
| `include/cvm/ast.h` | Defines expression and statement node types |
| `src/compiler.cpp` | Emits bytecode from the AST |
| `include/cvm/bytecode.h` | Defines opcodes and disassembly support |
| `src/vm.cpp` | Executes bytecode and enforces runtime checks |
| `src/main.cpp` | CLI entry point and stage-dump interface |

## 5. Supported Language Functionality

### 5.1 Types

The current implementation supports:

- `int`
- `long`
- `long long`
- `float`
- `double`
- `bool`
- `char`
- `let`
- `auto`
- `nil`

### 5.2 Variable declarations

Examples:

```c
int x = 10;
double y = 3.14;
bool ready = true;
char c = 'A';
let value = 42;
auto flag = false;
long long big = 9000000000;
```

Notes:

- `let` and `auto` are accepted as inferred declarations
- A declaration may omit the initializer, in which case a default value is assigned internally
- Variables are currently managed in a global name table

### 5.3 Literals

Supported literal categories:

- Integer literals like `1`, `25`, `9000`
- Decimal numeric literals like `3.14`, `0.5`
- Boolean literals `true` and `false`
- `nil`
- Character literals

Character literal examples:

```c
'a'
'\n'
'\t'
'\x41'
'\101'
"z"
```

Important note:

- There is no string type in the current language
- Double quotes are treated as single-character literals only when exactly one character is present

### 5.4 Comments

Single-line comments are supported:

```c
// this is a comment
```

### 5.5 Statements

The implementation supports:

- Variable declarations
- Expression statements
- `print`
- `input`
- Blocks with `{ ... }`
- `if`
- `if ... else`
- `while`
- `for`
- `break`
- `continue`

Examples:

```c
print x;
input x;

if (x > 0) {
    print x;
} else {
    print 0;
}

while (x > 0) {
    x -= 1;
}

for (int i = 0; i < 5; i += 1) {
    print i;
}
```

### 5.6 Operators

#### Arithmetic

- `+`
- `-`
- `*`
- `/`
- `%`
- `^`

In this language, `^` is used for exponentiation.

#### Comparison

- `==`
- `!=`
- `<`
- `<=`
- `>`
- `>=`

#### Logical

- `&&`
- `||`
- `!`

#### Bitwise

- `&`
- `|`
- `~`

#### Assignment

- `=`
- `+=`
- `-=`
- `*=`
- `/=`

### 5.7 Input and output

Output:

```c
print expression;
```

Input:

```c
input variableName;
```

Input is typed according to the declared variable. Invalid input is rejected with a descriptive runtime error.

## 6. Operator Precedence

From lowest precedence to highest:

| Level | Operators |
|---|---|
| Assignment | `=`, `+=`, `-=`, `*=`, `/=` |
| Logical OR | `||` |
| Logical AND | `&&` |
| Equality | `==`, `!=` |
| Comparison | `<`, `<=`, `>`, `>=` |
| Bitwise OR | `\|` |
| Bitwise AND | `&` |
| Additive | `+`, `-` |
| Multiplicative | `*`, `/`, `%` |
| Power | `^` |
| Unary | `!`, `-`, `~` |
| Primary | literals, identifiers, grouped expressions |

Notes:

- Power is parsed right-associatively
- Conditions must evaluate to `bool`; there is no truthy/falsy coercion layer

## 7. Grammar

The following grammar matches the current recursive-descent parser closely.

```ebnf
program        ::= declaration* EOF ;

declaration    ::= varDecl
                 | statement ;

varDecl        ::= type IDENTIFIER ("=" expression)? ";" ;

type           ::= "let"
                 | "auto"
                 | "int"
                 | "integer"
                 | "long"
                 | "long" "long"
                 | "double"
                 | "float"
                 | "bool"
                 | "char" ;

statement      ::= printStmt
                 | inputStmt
                 | ifStmt
                 | whileStmt
                 | forStmt
                 | breakStmt
                 | continueStmt
                 | block
                 | exprStmt ;

block          ::= "{" declaration* "}" ;

printStmt      ::= "print" expression ";" ;
inputStmt      ::= "input" IDENTIFIER ";" ;
breakStmt      ::= "break" ";" ;
continueStmt   ::= "continue" ";" ;
exprStmt       ::= expression ";" ;

ifStmt         ::= "if" "(" expression ")" declaration ("else" declaration)? ;
whileStmt      ::= "while" "(" expression ")" declaration ;

forStmt        ::= "for" "(" forInit expression? ";" expression? ")" declaration ;
forInit        ::= varDecl
                 | exprStmt
                 | ";" ;

expression     ::= assignment ;

assignment     ::= logicalOr (assignmentOp assignment)? ;
assignmentOp   ::= "=" | "+=" | "-=" | "*=" | "/=" ;

logicalOr      ::= logicalAnd ("||" logicalAnd)* ;
logicalAnd     ::= equality ("&&" equality)* ;
equality       ::= comparison (("==" | "!=") comparison)* ;
comparison     ::= bitwiseOr ((">" | ">=" | "<" | "<=") bitwiseOr)* ;
bitwiseOr      ::= bitwiseAnd ("|" bitwiseAnd)* ;
bitwiseAnd     ::= term ("&" term)* ;
term           ::= factor (("+" | "-") factor)* ;
factor         ::= power (("*" | "/" | "%") power)* ;
power          ::= unary ("^" power)? ;
unary          ::= ("!" | "-" | "~") unary
                 | primary ;

primary        ::= NUMBER
                 | CHARACTER
                 | "nil"
                 | "true"
                 | "false"
                 | IDENTIFIER
                 | "(" expression ")" ;
```

## 8. What The Stage Dumps Show

### `--tokens`

Prints lexer output in a readable form. Each line shows:

- Token kind
- Lexeme
- Literal value when available
- Source line

### `--ast`

Prints the parsed tree in a compact prefix-style textual representation.

Example style:

```text
(print (+ 1 2))
```

### `--bytecode`

Prints bytecode instruction offsets, opcode names, and operands.

Example style:

```text
0: OP_CONSTANT 0 (1)
2: OP_CONSTANT 1 (2)
4: OP_ADD
5: OP_PRINT
6: OP_HALT
```

### `--all-stages`

Prints:

1. Source
2. Tokens
3. AST
4. Bytecode
5. Program output

If you want everything except execution, add `--no-run`.

## 9. Safety And Error Handling

The project already includes useful runtime and compile-time checks, including:

- Unexpected character detection in the lexer
- Malformed literal detection
- Parse errors with line information
- Undefined variable errors
- Division by zero protection
- Modulo by zero protection
- Integer overflow checks in critical numeric operations
- Floating-point overflow checks
- Stack underflow protection in the VM
- Input validation against declared variable types
- Instruction limit enforcement via `--max-steps`

## 10. Current Limitations

To keep the documentation honest, these are important current boundaries of the implementation:

- No user-defined functions yet
- No arrays, strings, classes, or heap-allocated objects
- No interactive REPL mode
- Variables are effectively compiled as globals, even when declared inside nested blocks
- Conditions expect `bool` values rather than general truthiness
- Compound assignment currently supports `+=`, `-=`, `*=`, and `/=` only
- Numeric literals are plain integer/decimal forms; scientific notation is not part of the current lexer

## 11. Suggested Submission Checklist

For a clean submission, you can include:

- Source code
- `README.md`
- `PROJECT_REFERENCE.md`
- A short report on architecture and design
- A demo video showing source, tokens, AST, bytecode, and final VM execution

## 12. Short Viva Summary

If someone asks what the project does in one paragraph:

> CVM++ is a handwritten compiler and virtual machine built in C++20. It reads a small statically-typed language, converts source code into tokens, parses those tokens into an AST, compiles the AST into bytecode, and executes the bytecode on a custom stack-based VM. The project also includes typed input handling, arithmetic and control-flow support, runtime safety checks, and command-line options to print each internal compilation stage for demonstration and debugging.
