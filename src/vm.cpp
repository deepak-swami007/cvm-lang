#include "cvm/vm.h"

#include <cctype>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace cvm {

namespace {

double ensureFiniteDouble(double value, std::string_view context) {
    if (!std::isfinite(value)) {
        throw std::runtime_error("Floating-point overflow in " + std::string(context) + ".");
    }
    return value;
}

float ensureFiniteFloat(float value, std::string_view context) {
    if (!std::isfinite(value)) {
        throw std::runtime_error("Floating-point overflow in " + std::string(context) + ".");
    }
    return value;
}

bool fitsInInt32(int64_t value) {
    return value >= std::numeric_limits<int32_t>::min() &&
           value <= std::numeric_limits<int32_t>::max();
}

bool fitsInLong(int64_t value) {
    return value >= static_cast<int64_t>(std::numeric_limits<long>::lowest()) &&
           value <= static_cast<int64_t>(std::numeric_limits<long>::max());
}

bool fitsInUnsignedChar(int64_t value) {
    return value >= 0 && value <= std::numeric_limits<unsigned char>::max();
}

enum class NumericKind {
    Int32,
    Int64,
    Float,
    Double,
};

NumericKind commonNumericKind(const Value& left, const Value& right) {
    if (isDouble(left) || isDouble(right)) return NumericKind::Double;
    if (isFloat(left) || isFloat(right)) return NumericKind::Float;
    if (isLongLong(left) || isLongLong(right)) return NumericKind::Int64;
    return NumericKind::Int32;
}

Value makeInt32Value(int64_t value, std::string_view context) {
    if (!fitsInInt32(value)) {
        throw std::runtime_error("Integer overflow in " + std::string(context) + ".");
    }
    return Value{static_cast<int32_t>(value)};
}

Value makeFloatValue(long double value, std::string_view context) {
    const float converted = static_cast<float>(value);
    if (!std::isfinite(converted)) {
        throw std::runtime_error("Floating-point overflow in " + std::string(context) + ".");
    }
    return Value{converted};
}

Value makeDoubleValue(long double value, std::string_view context) {
    return Value{ensureFiniteDouble(static_cast<double>(value), context)};
}

std::string trimCopy(std::string_view text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }

    return std::string(text.substr(start, end - start));
}

std::int64_t parseIntegerStrict(const std::string& text, const std::string& context) {
    try {
        std::size_t consumed = 0;
        const std::int64_t value = std::stoll(text, &consumed, 10);
        if (consumed != text.size()) {
            throw std::runtime_error("Invalid integer input for " + context + ": '" + text + "'.");
        }
        return value;
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Invalid integer input for " + context + ": '" + text + "'.");
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Integer input is out of range for " + context + ": '" + text + "'.");
    }
}

double parseDoubleStrict(const std::string& text, const std::string& context) {
    try {
        std::size_t consumed = 0;
        const double value = std::stod(text, &consumed);
        if (consumed != text.size()) {
            throw std::runtime_error("Invalid numeric input for " + context + ": '" + text + "'.");
        }
        return ensureFiniteDouble(value, "input");
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Invalid numeric input for " + context + ": '" + text + "'.");
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Numeric input is out of range for " + context + ": '" + text + "'.");
    }
}

char parseCharInput(std::string_view rawInput, const std::string& context) {
    const std::string trimmed = trimCopy(rawInput);
    if (trimmed.empty()) {
        throw std::runtime_error("Invalid char input for " + context + ": expected exactly one character.");
    }

    if (trimmed.size() == 1) {
        return trimmed[0];
    }

    if (trimmed.size() == 3 && trimmed.front() == '\'' && trimmed.back() == '\'') {
        return trimmed[1];
    }

    if (trimmed.size() == 4 && trimmed.front() == '\'' && trimmed[1] == '\\' && trimmed.back() == '\'') {
        switch (trimmed[2]) {
            case 'n': return '\n';
            case 't': return '\t';
            case 'r': return '\r';
            case '0': return '\0';
            case '\\': return '\\';
            case '\'': return '\'';
            case '"': return '"';
            default:
                break;
        }
    }

    throw std::runtime_error("Invalid char input for " + context + ": expected exactly one character.");
}

Value inferInputValue(const std::string& rawInput) {
    const std::string trimmed = trimCopy(rawInput);
    if (trimmed.empty()) {
        throw std::runtime_error("Invalid input for auto input: expected a number, bool, or single character.");
    }

    if (trimmed == "true") {
        return Value{true};
    }
    if (trimmed == "false") {
        return Value{false};
    }

    try {
        const int64_t integerValue = parseIntegerStrict(trimmed, "auto input");
        if (fitsInInt32(integerValue)) {
            return Value{static_cast<int32_t>(integerValue)};
        }
        return Value{integerValue};
    } catch (const std::exception&) {
    }

    try {
        return Value{parseDoubleStrict(trimmed, "auto input")};
    } catch (const std::exception&) {
    }

    if (trimmed.size() == 1 ||
        (trimmed.size() >= 3 && trimmed.front() == '\'' && trimmed.back() == '\'')) {
        try {
            return Value{parseCharInput(trimmed, "auto input")};
        } catch (const std::exception&) {
        }
    }

    throw std::runtime_error("Invalid input for auto input: expected a number, bool, or single character.");
}

Value coerceValueToDeclType(const Value& value, DeclType declType, const std::string& context) {
    switch (declType) {
        case DeclType::Auto:
            return value;
        case DeclType::Int: {
            if (!isNumber(value)) {
                throw std::runtime_error(
                    "Cannot assign " + valueTypeName(value) + " to int in " + context + ".");
            }
            const int64_t converted = toInt64(value);
            if (!fitsInInt32(converted)) {
                throw std::runtime_error("Cannot assign " + valueTypeName(value) + " to int in " + context +
                                         ": value is out of int range.");
            }
            return Value{static_cast<int32_t>(converted)};
        }
        case DeclType::Long: {
            if (!isNumber(value)) {
                throw std::runtime_error(
                    "Cannot assign " + valueTypeName(value) + " to long in " + context + ".");
            }
            const int64_t converted = toInt64(value);
            if (!fitsInLong(converted)) {
                throw std::runtime_error("Cannot assign " + valueTypeName(value) + " to long in " + context +
                                         ": value is out of long range.");
            }
            if constexpr (sizeof(long) <= sizeof(int32_t)) {
                return Value{static_cast<int32_t>(converted)};
            } else {
                return Value{converted};
            }
        }
        case DeclType::LongLong:
            if (!isNumber(value)) {
                throw std::runtime_error(
                    "Cannot assign " + valueTypeName(value) + " to long long in " + context + ".");
            }
            return Value{toInt64(value)};
        case DeclType::Double:
            if (!isNumber(value)) {
                throw std::runtime_error(
                    "Cannot assign " + valueTypeName(value) + " to double in " + context + ".");
            }
            return Value{ensureFiniteDouble(toDouble(value), context)};
        case DeclType::Float:
            if (!isNumber(value)) {
                throw std::runtime_error(
                    "Cannot assign " + valueTypeName(value) + " to float in " + context + ".");
            }
            return Value{ensureFiniteFloat(static_cast<float>(toDouble(value)), context)};
        case DeclType::Bool:
            if (!isBoolean(value)) {
                throw std::runtime_error("Cannot assign " + valueTypeName(value) + " to bool in " + context + ".");
            }
            return value;
        case DeclType::Char:
            if (isChar(value)) {
                return value;
            }
            if (!isNumber(value)) {
                throw std::runtime_error("Cannot assign " + valueTypeName(value) + " to char in " + context + ".");
            }
            {
                const int64_t converted = toInt64(value);
                if (!fitsInUnsignedChar(converted)) {
                    throw std::runtime_error("Cannot assign " + valueTypeName(value) + " to char in " + context +
                                             ": value is out of char range.");
                }
                return Value{static_cast<char>(static_cast<unsigned char>(converted))};
            }
    }

    return value;
}

Value parseInputValue(const std::string& rawInput, DeclType declType, const std::string& context) {
    const std::string trimmed = trimCopy(rawInput);

    switch (declType) {
        case DeclType::Auto:
            return inferInputValue(rawInput);
        case DeclType::Int:
            return coerceValueToDeclType(Value{static_cast<int64_t>(parseIntegerStrict(trimmed, context))},
                                         DeclType::Int, context);
        case DeclType::Long:
            return coerceValueToDeclType(Value{static_cast<int64_t>(parseIntegerStrict(trimmed, context))},
                                         DeclType::Long, context);
        case DeclType::LongLong:
            return Value{static_cast<int64_t>(parseIntegerStrict(trimmed, context))};
        case DeclType::Double:
            return Value{parseDoubleStrict(trimmed, context)};
        case DeclType::Float:
            return coerceValueToDeclType(Value{parseDoubleStrict(trimmed, context)}, DeclType::Float, context);
        case DeclType::Bool:
            if (trimmed == "true" || trimmed == "1") {
                return Value{true};
            }
            if (trimmed == "false" || trimmed == "0") {
                return Value{false};
            }
            throw std::runtime_error("Invalid bool input for " + context + ": '" + rawInput + "'.");
        case DeclType::Char:
            return Value{parseCharInput(rawInput, context)};
    }

    return Value{std::monostate{}};
}

}  // namespace

static Value numericAdd(const Value& left, const Value& right) {
    switch (commonNumericKind(left, right)) {
        case NumericKind::Int32: {
            const int32_t a = static_cast<int32_t>(toInt64(left));
            const int32_t b = static_cast<int32_t>(toInt64(right));
            if (addOverflows(a, b)) {
                throw std::runtime_error("Integer overflow in addition (" +
                                         std::to_string(a) + " + " + std::to_string(b) + ").");
            }
            return Value{static_cast<int32_t>(a + b)};
        }
        case NumericKind::Int64: {
            const int64_t a = toInt64(left);
            const int64_t b = toInt64(right);
            if (addOverflows(a, b)) {
                throw std::runtime_error("Integer overflow in addition (" +
                                         std::to_string(a) + " + " + std::to_string(b) + ").");
            }
            return Value{a + b};
        }
        case NumericKind::Float:
            return makeFloatValue(toLongDouble(left) + toLongDouble(right), "addition");
        case NumericKind::Double:
            return makeDoubleValue(toLongDouble(left) + toLongDouble(right), "addition");
    }

    throw std::runtime_error("Internal addition error.");
}

static Value numericSub(const Value& left, const Value& right) {
    switch (commonNumericKind(left, right)) {
        case NumericKind::Int32: {
            const int32_t a = static_cast<int32_t>(toInt64(left));
            const int32_t b = static_cast<int32_t>(toInt64(right));
            if (subOverflows(a, b)) {
                throw std::runtime_error("Integer overflow in subtraction (" +
                                         std::to_string(a) + " - " + std::to_string(b) + ").");
            }
            return Value{static_cast<int32_t>(a - b)};
        }
        case NumericKind::Int64: {
            const int64_t a = toInt64(left);
            const int64_t b = toInt64(right);
            if (subOverflows(a, b)) {
                throw std::runtime_error("Integer overflow in subtraction (" +
                                         std::to_string(a) + " - " + std::to_string(b) + ").");
            }
            return Value{a - b};
        }
        case NumericKind::Float:
            return makeFloatValue(toLongDouble(left) - toLongDouble(right), "subtraction");
        case NumericKind::Double:
            return makeDoubleValue(toLongDouble(left) - toLongDouble(right), "subtraction");
    }

    throw std::runtime_error("Internal subtraction error.");
}

static Value numericMul(const Value& left, const Value& right) {
    switch (commonNumericKind(left, right)) {
        case NumericKind::Int32: {
            const int32_t a = static_cast<int32_t>(toInt64(left));
            const int32_t b = static_cast<int32_t>(toInt64(right));
            if (mulOverflows(a, b)) {
                throw std::runtime_error("Integer overflow in multiplication (" +
                                         std::to_string(a) + " * " + std::to_string(b) + ").");
            }
            return Value{static_cast<int32_t>(a * b)};
        }
        case NumericKind::Int64: {
            const int64_t a = toInt64(left);
            const int64_t b = toInt64(right);
            if (mulOverflows(a, b)) {
                throw std::runtime_error("Integer overflow in multiplication (" +
                                         std::to_string(a) + " * " + std::to_string(b) + ").");
            }
            return Value{a * b};
        }
        case NumericKind::Float:
            return makeFloatValue(toLongDouble(left) * toLongDouble(right), "multiplication");
        case NumericKind::Double:
            return makeDoubleValue(toLongDouble(left) * toLongDouble(right), "multiplication");
    }

    throw std::runtime_error("Internal multiplication error.");
}

static Value numericDiv(const Value& left, const Value& right) {
    switch (commonNumericKind(left, right)) {
        case NumericKind::Int32: {
            const int32_t a = static_cast<int32_t>(toInt64(left));
            const int32_t b = static_cast<int32_t>(toInt64(right));
            if (b == 0) throw std::runtime_error("Division by zero.");
            if (a == std::numeric_limits<int32_t>::min() && b == -1) {
                throw std::runtime_error("Integer overflow in division (INT_MIN / -1).");
            }
            return Value{static_cast<int32_t>(a / b)};
        }
        case NumericKind::Int64: {
            const int64_t a = toInt64(left);
            const int64_t b = toInt64(right);
            if (b == 0) throw std::runtime_error("Division by zero.");
            if (a == std::numeric_limits<int64_t>::min() && b == -1) {
                throw std::runtime_error("Integer overflow in division (INT_MIN / -1).");
            }
            return Value{a / b};
        }
        case NumericKind::Float: {
            const float divisor = static_cast<float>(toDouble(right));
            if (divisor == 0.0f) throw std::runtime_error("Division by zero.");
            return makeFloatValue(toLongDouble(left) / toLongDouble(right), "division");
        }
        case NumericKind::Double: {
            const double divisor = toDouble(right);
            if (divisor == 0.0) throw std::runtime_error("Division by zero.");
            return makeDoubleValue(toLongDouble(left) / toLongDouble(right), "division");
        }
    }

    throw std::runtime_error("Internal division error.");
}

static Value numericMod(const Value& left, const Value& right) {
    switch (commonNumericKind(left, right)) {
        case NumericKind::Int32: {
            const int32_t a = static_cast<int32_t>(toInt64(left));
            const int32_t b = static_cast<int32_t>(toInt64(right));
            if (b == 0) throw std::runtime_error("Modulo by zero.");
            return Value{static_cast<int32_t>(a % b)};
        }
        case NumericKind::Int64: {
            const int64_t a = toInt64(left);
            const int64_t b = toInt64(right);
            if (b == 0) throw std::runtime_error("Modulo by zero.");
            return Value{a % b};
        }
        case NumericKind::Float: {
            const float divisor = static_cast<float>(toDouble(right));
            if (divisor == 0.0f) throw std::runtime_error("Modulo by zero.");
            return makeFloatValue(std::fmod(static_cast<float>(toDouble(left)), divisor), "modulo");
        }
        case NumericKind::Double: {
            const double divisor = toDouble(right);
            if (divisor == 0.0) throw std::runtime_error("Modulo by zero.");
            return makeDoubleValue(std::fmod(toDouble(left), divisor), "modulo");
        }
    }

    throw std::runtime_error("Internal modulo error.");
}

static Value numericPow(const Value& left, const Value& right) {
    return makeDoubleValue(std::pow(toDouble(left), toDouble(right)), "power");
}

static void ensureNumber(const Value& value, const char* context) {
    if (!isNumber(value)) {
        throw std::runtime_error(
            std::string("Expected number for ") + context + ", got " + valueTypeName(value) + ".");
    }
}

static void ensureIntegral(const Value& value, const char* context) {
    if (!isIntegral(value)) {
        throw std::runtime_error(
            std::string("Expected int for ") + context + ", got " + valueTypeName(value) + ".");
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
                const std::uint8_t nameIndex = readGlobalIndex(chunk, ip);
                const std::string& name = readGlobalName(chunk, nameIndex);
                if (globals_.contains(name)) {
                    throw std::runtime_error("Variable '" + name + "' is already defined.");
                }
                globals_.emplace(
                    name,
                    coerceValueToDeclType(
                        pop(), readGlobalType(chunk, nameIndex), "declaration of variable '" + name + "'"));
                break;
            }
            case OpCode::GetGlobal: {
                const std::uint8_t nameIndex = readGlobalIndex(chunk, ip);
                const std::string& name = readGlobalName(chunk, nameIndex);
                const auto it = globals_.find(name);
                if (it == globals_.end()) {
                    throw std::runtime_error("Undefined variable '" + name + "'.");
                }
                push(it->second);
                break;
            }
            case OpCode::SetGlobal: {
                const std::uint8_t nameIndex = readGlobalIndex(chunk, ip);
                const std::string& name = readGlobalName(chunk, nameIndex);
                const auto it = globals_.find(name);
                if (it == globals_.end()) {
                    throw std::runtime_error("Undefined variable '" + name + "'.");
                }
                if (stack_.empty()) {
                    throw std::runtime_error("Cannot assign to '" + name + "' because the VM stack is empty.");
                }
                it->second =
                    coerceValueToDeclType(stack_.back(), readGlobalType(chunk, nameIndex),
                                          "assignment to variable '" + name + "'");
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
                ensureIntegral(left, "left operand of '&'");
                ensureIntegral(right, "right operand of '&'");
                push(Value{toInt64(left) & toInt64(right)});
                break;
            }
            case OpCode::BitwiseOr: {
                const Value right = pop();
                const Value left = pop();
                ensureIntegral(left, "left operand of '|'");
                ensureIntegral(right, "right operand of '|'");
                push(Value{toInt64(left) | toInt64(right)});
                break;
            }
            case OpCode::BitwiseNot: {
                const Value operand = pop();
                ensureIntegral(operand, "operand of '~'");
                push(Value{~toInt64(operand)});
                break;
            }

            case OpCode::Negate: {
                const Value operand = pop();
                ensureNumber(operand, "operand of unary '-'");
                if (isInt(operand)) {
                    const int32_t value = std::get<int32_t>(operand);
                    if (value == std::numeric_limits<int32_t>::min()) {
                        throw std::runtime_error("Integer overflow in negation.");
                    }
                    push(Value{static_cast<int32_t>(-value)});
                } else if (isLongLong(operand)) {
                    const int64_t value = std::get<int64_t>(operand);
                    if (value == std::numeric_limits<int64_t>::min()) {
                        throw std::runtime_error("Integer overflow in negation.");
                    }
                    push(Value{-value});
                } else if (isChar(operand)) {
                    push(makeInt32Value(-toInt64(operand), "negation"));
                } else if (isFloat(operand)) {
                    push(Value{ensureFiniteFloat(-std::get<float>(operand), "negation")});
                } else {
                    push(Value{ensureFiniteDouble(-std::get<double>(operand), "negation")});
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
                push(Value{ensureFiniteDouble(toDouble(operand), "cast")});
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
                const std::uint8_t nameIndex = readGlobalIndex(chunk, ip);
                const std::string& name = readGlobalName(chunk, nameIndex);
                const auto it = globals_.find(name);
                if (it == globals_.end()) {
                    throw std::runtime_error("Undefined variable '" + name + "' in input statement.");
                }
                std::string rawInput;
                if (!std::getline(std::cin >> std::ws, rawInput)) {
                    throw std::runtime_error("Invalid input: could not read a value for variable '" + name + "'.");
                }
                it->second = parseInputValue(rawInput, readGlobalType(chunk, nameIndex), "variable '" + name + "'");
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

std::uint8_t VirtualMachine::readGlobalIndex(const Chunk& chunk, std::size_t& ip) const {
    const std::uint8_t nameIndex = readByte(chunk, ip, "global operand");
    if (nameIndex >= chunk.names.size()) {
        throw std::runtime_error("Invalid global name index " + std::to_string(nameIndex) + ".");
    }
    return nameIndex;
}

const std::string& VirtualMachine::readGlobalName(const Chunk& chunk, std::uint8_t nameIndex) const {
    if (nameIndex >= chunk.names.size()) {
        throw std::runtime_error("Invalid global name index " + std::to_string(nameIndex) + ".");
    }
    return chunk.names[nameIndex];
}

DeclType VirtualMachine::readGlobalType(const Chunk& chunk, std::uint8_t nameIndex) const {
    if (nameIndex >= chunk.nameTypes.size()) {
        throw std::runtime_error("Invalid global type index " + std::to_string(nameIndex) + ".");
    }
    return chunk.nameTypes[nameIndex];
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
