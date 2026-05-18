# CVM++ Project Reference

## Jump To

[Build Instructions](#1-build-instructions) •
[CLI Command Reference](#2-cli-command-reference) •
[Recommended Demo Sequence](#3-recommended-demo-sequence) •
[Architecture Overview](#4-architecture-overview) •
[Supported Language Functionality](#5-supported-language-functionality) •
[Operator Precedence](#6-operator-precedence) •
[Grammar](#7-grammar) •
[What The Stage Dumps Show](#8-what-the-stage-dumps-show) •
[Error Reference](#9-error-reference)

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

## 9. Error Reference

The executable classifies user-facing failures into labels such as `LexerError`, `SyntaxError`, `DeclarationError`, `NameError`, `TypeError`, `MathError`, `InputError`, `OverflowError`, `StackError`, and `RuntimeError`.

### 9.1 Driver And Startup Errors

These are reported before lexing/parsing begins.

| Error family | When it happens | Typical trigger |
|---|---|---|
| `RuntimeError` | Input file cannot be opened | Wrong path or missing file |
| `RuntimeError` | No usable input file is provided | No `.cvm` file passed and no fallback example exists |
| `RuntimeError` | Unknown CLI option is used | Invalid flag such as an unsupported `--something` |
| `RuntimeError` | `--max-steps` is missing its value | `--max-steps` with no number after it |
| `RuntimeError` | `--max-steps` is invalid | Non-numeric value such as text |
| `RuntimeError` | `--max-steps` is too large | Value outside the supported range |
| `RuntimeError` | More than one input file is provided | Multiple positional file arguments |

### 9.2 Compilation-Time Errors

Compilation-time here includes lexing, parsing, semantic validation, and bytecode generation.

| Phase | Error family | When it happens | Typical message pattern |
|---|---|---|---|
| Lexer | `LexerError` | Illegal character appears in source | `Unexpected character 'x'` |
| Lexer | `LexerError` | Character literal is not closed | `Unterminated char literal` |
| Lexer | `LexerError` | Empty character literal is used | `Empty char literal` |
| Lexer | `LexerError` | Character literal contains too many characters | `Char literal must contain exactly one character` |
| Lexer | `LexerError` | Invalid escape sequence is used in a char literal | `Invalid escape sequence` |
| Lexer | `LexerError` | Invalid hex escape is used | `Invalid hex escape sequence` |
| Lexer | `LexerError` | Numeric token is malformed | `Invalid numeric literal` |
| Lexer | `OverflowError` | Numeric literal exceeds supported range | `Numeric literal '...' is out of range` |
| Parser | `SyntaxError` | Required token is missing | `Expected ';'`, `Expected ')'`, `Expected '}'`, `Expected variable name` |
| Parser | `SyntaxError` | Expression is malformed | `Expected a literal, identifier, or parenthesized expression` |
| Parser | `SyntaxError` | Assignment target is not assignable | `Invalid assignment target` |
| Compiler | `DeclarationError` | Same variable is declared more than once | `Variable 'x' is already declared` |
| Compiler | `NameError` | Variable is used before being declared | `Undefined variable 'x'` |
| Compiler | `SyntaxError` | `break` is used outside a loop | `Cannot use 'break' outside of a loop` |
| Compiler | `SyntaxError` | `continue` is used outside a loop | `Cannot use 'continue' outside of a loop` |
| Compiler | `RuntimeError` | Constant pool limit is exceeded during code generation | `Too many constants in one chunk` |
| Compiler | `RuntimeError` | Global name table limit is exceeded during code generation | `Too many global names in one chunk` |
| Compiler | `RuntimeError` | Jump patching exceeds bytecode limits | `Too much code to jump over` |
| Compiler | `RuntimeError` | Loop body exceeds jump encoding limits | `Loop body is too large` |
| Compiler | `RuntimeError` | Internal opcode emission reaches an impossible state | `Unsupported unary operator`, `Unsupported binary operator`, `Internal compiler error while patching jump` |

### 9.3 Runtime Errors

These occur after bytecode generation, while the VM is executing.

| Error family | When it happens | Typical message pattern |
|---|---|---|
| `NameError` | Variable is missing at runtime | `Undefined variable 'x'` |
| `DeclarationError` | VM attempts to define an already-defined global | `Variable 'x' is already defined` |
| `TypeError` | Arithmetic operator receives non-numeric operands | `Expected number for ...` |
| `TypeError` | Bitwise operator receives non-integral operands | `Expected int for ...` |
| `TypeError` | Condition is not boolean | `Expected bool for if/while condition` |
| `TypeError` | Assignment violates the declared variable type | `Cannot assign ... to int/double/bool/char/...` |
| `TypeError` | Numeric conversion helper receives a non-numeric value | `Expected numeric value, got ...` |
| `MathError` | Division by zero occurs | `Division by zero` |
| `MathError` | Modulo by zero occurs | `Modulo by zero` |
| `OverflowError` | Integer math overflows | `Integer overflow in addition/subtraction/multiplication/division/negation` |
| `OverflowError` | Float or double result becomes non-finite | `Floating-point overflow in ...` |
| `OverflowError` | Type conversion overflows | `Float value is out of int range`, `Double value is out of int range` |
| `InputError` | `input` cannot read a value | `Invalid input: could not read a value for variable ...` |
| `InputError` | User input does not match declared type | `Invalid integer input`, `Invalid numeric input`, `Invalid bool input`, `Invalid char input` |
| `InputError` | `auto` input cannot be inferred from the entered text | `Invalid input for auto input` |
| `StackError` | VM tries to pop from an empty stack | `VM stack underflow` |
| `StackError` | VM checks a loop/if condition with no stack value available | `VM stack underflow while reading if/while condition` |
| `RuntimeError` | Execution exceeds the configured instruction cap | `Execution step limit exceeded` |
| `RuntimeError` | Bytecode jump target is invalid | `Jump target is out of bounds`, `Conditional jump target is out of bounds`, `Loop target is out of bounds` |
| `RuntimeError` | Bytecode stream is malformed or truncated | `Unexpected end of bytecode while reading ...` |
| `RuntimeError` | Bytecode metadata index is invalid | `Invalid constant index`, `Invalid global name index`, `Invalid global type index` |
| `RuntimeError` | VM sees an unknown opcode | `Unknown opcode byte ...` |
| `RuntimeError` | Bytecode finishes without a halt instruction | `VM reached the end of bytecode without OP_HALT` |
| `RuntimeError` | Assignment opcode executes with no value on the stack | `Cannot assign to 'x' because the VM stack is empty` |

### 9.4 Error Reporting Style

The executable also improves error readability in a few ways:

- Parse and runtime errors try to preserve source line numbers
- The front-end prints labels such as `SyntaxError`, `TypeError`, and `MathError`
- When a line number is available, the CLI prints the source line and a caret marker
