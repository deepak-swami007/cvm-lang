#include "cvm/vm.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace cvm {

// ── Helper: type-aware binary arithmetic ────────────────────────────
// If both operands are int64_t, do integer math (with overflow check).
// If either is double, promote both to double.

static Value numericAdd(const Value& left, const Value& right) {
    if (isInteger(left) && isInteger(right)) {
        int64_t a = std::get<int64_t>(left), b = std::get<int64_t>(right);
        if (addOverflows(a, b))
            throw std::runtime_error("Integer overflow in addition (" +
                std::to_string(a) + " + " + std::to_string(b) + ").");
        return Value{a + b};
    }
    return Value{toDouble(left) + toDouble(right)};
}

static Value numericSub(const Value& left, const Value& right) {
    if (isInteger(left) && isInteger(right)) {
        int64_t a = std::get<int64_t>(left), b = std::get<int64_t>(right);
        if (subOverflows(a, b))
            throw std::runtime_error("Integer overflow in subtraction (" +
                std::to_string(a) + " - " + std::to_string(b) + ").");
        return Value{a - b};
    }
    return Value{toDouble(left) - toDouble(right)};
}

static Value numericMul(const Value& left, const Value& right) {
    if (isInteger(left) && isInteger(right)) {
        int64_t a = std::get<int64_t>(left), b = std::get<int64_t>(right);
        if (mulOverflows(a, b))
            throw std::runtime_error("Integer overflow in multiplication (" +
                std::to_string(a) + " * " + std::to_string(b) + ").");
        return Value{a * b};
    }
    return Value{toDouble(left) * toDouble(right)};
}

static Value numericDiv(const Value& left, const Value& right) {
    if (isInteger(left) && isInteger(right)) {
        int64_t a = std::get<int64_t>(left), b = std::get<int64_t>(right);
        if (b == 0) throw std::runtime_error("Division by zero.");
        if (a == std::numeric_limits<int64_t>::min() && b == -1)
            throw std::runtime_error("Integer overflow in division (INT_MIN / -1).");
        return Value{a / b};
    }
    double d = toDouble(right);
    if (d == 0.0) throw std::runtime_error("Division by zero.");
    return Value{toDouble(left) / d};
}

static Value numericMod(const Value& left, const Value& right) {
    if (isInteger(left) && isInteger(right)) {
        int64_t a = std::get<int64_t>(left), b = std::get<int64_t>(right);
        if (b == 0) throw std::runtime_error("Modulo by zero.");
        return Value{a % b};
    }
    double d = toDouble(right);
    if (d == 0.0) throw std::runtime_error("Modulo by zero.");
    return Value{std::fmod(toDouble(left), d)};
}

static Value numericPow(const Value& left, const Value& right) {
    // Power always produces a double (like std::pow)
    return Value{std::pow(toDouble(left), toDouble(right))};
}

static void ensureNumber(const Value& value, const char* context) {
    if (!isNumber(value)) {
        throw std::runtime_error(
            std::string("Expected number for ") + context + ", got " + valueTypeName(value) + ".");
    }
}

// ── VM run ──────────────────────────────────────────────────────────

void VirtualMachine::run(const Chunk& chunk, std::ostream& out, const VMOptions& options) {
    stack_.clear();
    globals_.clear();
    std::size_t ip = 0;
    std::size_t executedInstructions = 0;

    while (ip < chunk.code.size()) {
        if (options.maxInstructions != 0 && executedInstructions >= options.maxInstructions) {
            throw std::runtime_error(
                "Execution step limit exceeded after " + std::to_string(options.maxInstructions) +
                " instructions. Possible infinite loop. Increase --max-steps or use 0 to disable the limit.");
        }

        const auto instruction = static_cast<OpCode>(readByte(chunk, ip, "opcode"));
        ++executedInstructions;

        switch (instruction) {
            case OpCode::Nil:
                push(Value{std::monostate{}});
                break;
            case OpCode::True:
                push(Value{true});
                break;
            case OpCode::False:
                push(Value{false});
                break;
            case OpCode::Constant:
                push(readConstant(chunk, ip));
                break;
            case OpCode::DefineGlobal: {
                const std::string& name = readGlobalName(chunk, ip);
                if (globals_.contains(name)) {
                    throw std::runtime_error("Variable '" + name + "' is already defined.");
                }
                globals_.emplace(name, pop());
                break;
            }
            case OpCode::GetGlobal: {
                const std::string& name = readGlobalName(chunk, ip);
                const auto it = globals_.find(name);
                if (it == globals_.end()) {
                    throw std::runtime_error("Undefined variable '" + name + "'.");
                }
                push(it->second);
                break;
            }
            case OpCode::SetGlobal: {
                const std::string& name = readGlobalName(chunk, ip);
                const auto it = globals_.find(name);
                if (it == globals_.end()) {
                    throw std::runtime_error("Undefined variable '" + name + "'.");
                }
                if (stack_.empty()) {
                    throw std::runtime_error("Cannot assign to '" + name + "' because the VM stack is empty.");
                }
                it->second = stack_.back();
                break;
            }
            case OpCode::Equal: {
                const Value right = pop();
                const Value left = pop();
                push(Value{valuesEqual(left, right)});
                break;
            }
            case OpCode::Greater: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '>'");
                ensureNumber(right, "right operand of '>'");
                push(Value{toDouble(left) > toDouble(right)});
                break;
            }
            case OpCode::GreaterEqual: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '>='");
                ensureNumber(right, "right operand of '>='");
                push(Value{toDouble(left) >= toDouble(right)});
                break;
            }
            case OpCode::Less: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '<'");
                ensureNumber(right, "right operand of '<'");
                push(Value{toDouble(left) < toDouble(right)});
                break;
            }
            case OpCode::LessEqual: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '<='");
                ensureNumber(right, "right operand of '<='");
                push(Value{toDouble(left) <= toDouble(right)});
                break;
            }
            case OpCode::Not:
                push(Value{!expectBoolean(pop(), "operand of '!'")});
                break;

            // ── Arithmetic ──────────────────────────────────────
            case OpCode::Add: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '+'");
                ensureNumber(right, "right operand of '+'");
                push(numericAdd(left, right));
                break;
            }
            case OpCode::Subtract: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '-'");
                ensureNumber(right, "right operand of '-'");
                push(numericSub(left, right));
                break;
            }
            case OpCode::Multiply: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '*'");
                ensureNumber(right, "right operand of '*'");
                push(numericMul(left, right));
                break;
            }
            case OpCode::Divide: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '/'");
                ensureNumber(right, "right operand of '/'");
                push(numericDiv(left, right));
                break;
            }
            case OpCode::Modulo: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '%'");
                ensureNumber(right, "right operand of '%'");
                push(numericMod(left, right));
                break;
            }
            case OpCode::Power: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '^'");
                ensureNumber(right, "right operand of '^'");
                push(numericPow(left, right));
                break;
            }

            // ── Bitwise (integers only) ─────────────────────────
            case OpCode::BitwiseAnd: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '&'");
                ensureNumber(right, "right operand of '&'");
                push(Value{toInt64(left) & toInt64(right)});
                break;
            }
            case OpCode::BitwiseOr: {
                const Value right = pop();
                const Value left = pop();
                ensureNumber(left, "left operand of '|'");
                ensureNumber(right, "right operand of '|'");
                push(Value{toInt64(left) | toInt64(right)});
                break;
            }
            case OpCode::BitwiseNot: {
                const Value operand = pop();
                ensureNumber(operand, "operand of '~'");
                push(Value{~toInt64(operand)});
                break;
            }

            case OpCode::Negate: {
                const Value operand = pop();
                ensureNumber(operand, "operand of unary '-'");
                if (isInteger(operand)) {
                    int64_t v = std::get<int64_t>(operand);
                    if (v == std::numeric_limits<int64_t>::min())
                        throw std::runtime_error("Integer overflow in negation.");
                    push(Value{-v});
                } else {
                    push(Value{-std::get<double>(operand)});
                }
                break;
            }

            // ── Type casts ──────────────────────────────────────
            case OpCode::CastToInt: {
                const Value operand = pop();
                push(Value{toInt64(operand)});
                break;
            }
            case OpCode::CastToDouble: {
                const Value operand = pop();
                push(Value{toDouble(operand)});
                break;
            }

            // ── Control flow ────────────────────────────────────
            case OpCode::Jump: {
                const std::uint16_t offset = readShort(chunk, ip, "jump operand");
                if (ip + offset > chunk.code.size()) {
                    throw std::runtime_error("Jump target is out of bounds.");
                }
                ip += offset;
                break;
            }
            case OpCode::JumpIfFalse: {
                const std::uint16_t offset = readShort(chunk, ip, "conditional jump operand");
                if (stack_.empty()) {
                    throw std::runtime_error("VM stack underflow while reading if/while condition.");
                }
                const bool condition = expectBoolean(stack_.back(), "if/while condition");
                if (!condition) {
                    if (ip + offset > chunk.code.size()) {
                        throw std::runtime_error("Conditional jump target is out of bounds.");
                    }
                    ip += offset;
                }
                break;
            }
            case OpCode::Loop: {
                const std::uint16_t offset = readShort(chunk, ip, "loop operand");
                if (offset > ip) {
                    throw std::runtime_error("Loop target is out of bounds.");
                }
                ip -= offset;
                break;
            }

            // ── I/O ─────────────────────────────────────────────
            case OpCode::Print:
                out << formatValue(pop()) << '\n';
                break;
            case OpCode::Input: {
                const std::string& name = readGlobalName(chunk, ip);
                const auto it = globals_.find(name);
                if (it == globals_.end()) {
                    throw std::runtime_error("Undefined variable '" + name + "' in input statement.");
                }
                double inputValue = 0;
                std::cin >> inputValue;
                if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    throw std::runtime_error("Invalid input: expected a number for variable '" + name + "'.");
                }
                // Store as integer if it's a whole number
                if (inputValue == std::floor(inputValue) &&
                    inputValue >= static_cast<double>(std::numeric_limits<int64_t>::min()) &&
                    inputValue <= static_cast<double>(std::numeric_limits<int64_t>::max())) {
                    it->second = Value{static_cast<int64_t>(inputValue)};
                } else {
                    it->second = Value{inputValue};
                }
                break;
            }

            case OpCode::Pop:
                pop();
                break;
            case OpCode::Halt:
                return;
            default:
                throw std::runtime_error("Unknown opcode byte " + std::to_string(static_cast<int>(instruction)) + ".");
        }
    }

    throw std::runtime_error("VM reached the end of bytecode without OP_HALT.");
}

std::uint8_t VirtualMachine::readByte(const Chunk& chunk, std::size_t& ip, const char* context) const {
    if (ip >= chunk.code.size()) {
        throw std::runtime_error("Unexpected end of bytecode while reading " + std::string(context) + ".");
    }
    return chunk.code[ip++];
}

std::uint16_t VirtualMachine::readShort(const Chunk& chunk, std::size_t& ip, const char* context) const {
    const std::uint8_t high = readByte(chunk, ip, context);
    const std::uint8_t low = readByte(chunk, ip, context);
    return static_cast<std::uint16_t>((high << 8) | low);
}

Value VirtualMachine::readConstant(const Chunk& chunk, std::size_t& ip) const {
    const std::uint8_t constantIndex = readByte(chunk, ip, "constant operand");
    if (constantIndex >= chunk.constants.size()) {
        throw std::runtime_error("Invalid constant index " + std::to_string(constantIndex) + ".");
    }
    return chunk.constants[constantIndex];
}

const std::string& VirtualMachine::readGlobalName(const Chunk& chunk, std::size_t& ip) const {
    const std::uint8_t nameIndex = readByte(chunk, ip, "global operand");
    if (nameIndex >= chunk.names.size()) {
        throw std::runtime_error("Invalid global name index " + std::to_string(nameIndex) + ".");
    }
    return chunk.names[nameIndex];
}

bool VirtualMachine::expectBoolean(const Value& value, const char* context) const {
    if (const auto* boolean = std::get_if<bool>(&value)) {
        return *boolean;
    }
    throw std::runtime_error(
        std::string("Expected bool for ") + context + ", got " + valueTypeName(value) + ".");
}

void VirtualMachine::push(Value value) {
    stack_.push_back(value);
}

Value VirtualMachine::pop() {
    if (stack_.empty()) {
        throw std::runtime_error("VM stack underflow.");
    }
    const Value value = stack_.back();
    stack_.pop_back();
    return value;
}

}  // namespace cvm
