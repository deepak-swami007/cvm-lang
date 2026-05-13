# CVM++ — A Custom Virtual Machine Language

CVM++ is a fast, bytecode-compiled programming language with its own virtual machine, built completely from scratch in C++20. It implements a fully functioning compiler pipeline: **Lexer → Parser → AST → Compiler → VM**.

## Features & Language Specification

### 1. Data Types
CVM++ features strict type checking with robust bounds handling for primitive types:
- `int` — 32-bit signed integer
- `long` — 64-bit signed integer
- `float` — 32-bit floating point
- `double` — 64-bit floating point
- `bool` — Boolean logic (`true`, `false`)
- `char` — Single-byte characters (`'A'`, `'\n'`)
- `auto` — Type inference (figures out type based on value)
- `nil` — Uninitialized or null state

> **Note:** The `let` keyword is fully supported as an alias for `auto` for legacy compatibility.

### 2. Variables and Assignments
Variables are block-scoped and require an initial type declaration.

```cvm
int count = 10;
long big_number = 9999999999;
float pi = 3.14;
double precise_pi = 3.1415926535;
bool flag = true;
char letter = 'C';
auto dynamic_val = 100;
```

### 3. Operators
CVM++ supports a wide array of mathematical, relational, and bitwise operations:
- **Arithmetic:** `+`, `-`, `*`, `/`, `%` (modulo), `^` (power)
- **Bitwise (Integers only):** `&` (AND), `|` (OR), `~` (NOT)
- **Unary:** `-` (negation), `!` (logical NOT)
- **Relational:** `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Logical (Short-circuiting):** `&&` (AND), `||` (OR)
- **Compound Assignment:** `+=`, `-=`, `*=`, `/=`

### 4. Control Flow
Standard programming constructs are natively supported:

```cvm
// If-Else Conditionals
if (count == 10 && flag) {
    print "Matched!";
} else {
    print "Not matched.";
}

// While Loops
while (count > 0) {
    count -= 1;
    if (count == 5) continue; // Skip to next iteration
}

// For Loops
for (int i = 0; i < 10; i += 1) {
    if (i == 8) break; // Exit loop early
    print i;
}
```

### 5. Input and Output (I/O)
- `print <expression>;` — Evaluates the expression and writes it to standard output.
- `input <variable>;` — Prompts the user via standard input. Input is automatically validated against the variable's declared type.

```cvm
int age = 0;
print "Enter your age: ";
input age; // Ensures the user enters a valid integer
```

### 6. Comments
Single line comments are supported using `//`.

---

## Safety & Error Handling

CVM++ has robust runtime and compile-time error reporting, pinpointing exact line numbers.

| Error Type | Description |
|---|---|
| `SyntaxError` / `LexerError` | Invalid syntax, missing semicolons, or unrecognized characters. |
| `NameError` | Attempting to access an undefined variable. |
| `TypeError` | Type mismatches, such as assigning a `bool` to an `int` without casting. |
| `MathError` | Illegal math operations, such as modulo or division by zero. |
| `OverflowError` | Exceeding numeric limits (e.g., `INT_MAX + 1` or `INT_MIN / -1`). |
| `DeclarationError` | Trying to redeclare an existing variable in the same scope. |
| `InputError` | User provides invalid console input for a statically typed variable. |

**Example Error Output:**
```
[Line 14] TypeError: Cannot assign bool to int in assignment to variable 'x'.
```

---

## Build & Run

### Prerequisites
- C++20 compatible compiler (`clang++`, `g++`, or `MSVC`)
- `CMake` (version 3.10 or higher)

### Build
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Run
From the root directory of the project:

```bash
# Run the REPL or default example
./build/cvm

# Run a specific script
./build/cvm examples/example.cvm

# Limit VM execution steps (prevents infinite loops)
./build/cvm --max-steps 10000 examples/example.cvm
```

## Architecture

CVM++ processes code in multiple phases. The VM is stack-based, executing specific custom opcodes.

```text
Source Code (.cvm)
       │
       ▼
    [Lexer]         → Tokens (Keywords, Identifiers, Operators)
       │
       ▼
    [Parser]        → AST (Abstract Syntax Tree)
       │
       ▼
    [Compiler]      → Bytecode (Chunk with custom Opcodes)
       │
       ▼
    [VM]            → Execution Output
```
