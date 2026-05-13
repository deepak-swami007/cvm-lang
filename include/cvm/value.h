#pragma once

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

#include "cvm/type.h"

namespace cvm {

using Value = std::variant<std::monostate, int32_t, int64_t, float, double, bool, char>;

inline bool isNil(const Value& value) {
    return std::holds_alternative<std::monostate>(value);
}

inline bool isInt(const Value& value) {
    return std::holds_alternative<int32_t>(value);
}

inline bool isLongLong(const Value& value) {
    return std::holds_alternative<int64_t>(value);
}

inline bool isFloat(const Value& value) {
    return std::holds_alternative<float>(value);
}

inline bool isDouble(const Value& value) {
    return std::holds_alternative<double>(value);
}

inline bool isBoolean(const Value& value) {
    return std::holds_alternative<bool>(value);
}

inline bool isChar(const Value& value) {
    return std::holds_alternative<char>(value);
}

inline bool isIntegral(const Value& value) {
    return isInt(value) || isLongLong(value) || isChar(value);
}

inline bool isFloating(const Value& value) {
    return isFloat(value) || isDouble(value);
}

inline bool isNumber(const Value& value) {
    return isIntegral(value) || isFloating(value);
}

inline std::string valueTypeName(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) return "nil";
    if (std::holds_alternative<int32_t>(value)) return "int";
    if (std::holds_alternative<int64_t>(value)) return "long long";
    if (std::holds_alternative<float>(value)) return "float";
    if (std::holds_alternative<double>(value)) return "double";
    if (std::holds_alternative<bool>(value)) return "bool";
    if (std::holds_alternative<char>(value)) return "char";
    return "unknown";
}

inline int64_t charCode(char value) {
    return static_cast<unsigned char>(value);
}

inline std::string charToString(char value) {
    return std::string(1, value);
}

inline long double toLongDouble(const Value& value) {
    if (const auto* integer = std::get_if<int32_t>(&value)) return static_cast<long double>(*integer);
    if (const auto* integer = std::get_if<int64_t>(&value)) return static_cast<long double>(*integer);
    if (const auto* number = std::get_if<float>(&value)) return static_cast<long double>(*number);
    if (const auto* number = std::get_if<double>(&value)) return static_cast<long double>(*number);
    if (const auto* character = std::get_if<char>(&value)) return static_cast<long double>(charCode(*character));
    throw std::runtime_error("Expected numeric value, got " + valueTypeName(value) + ".");
}

inline double toDouble(const Value& value) {
    return static_cast<double>(toLongDouble(value));
}

inline int64_t toInt64(const Value& value) {
    if (const auto* integer = std::get_if<int32_t>(&value)) return static_cast<int64_t>(*integer);
    if (const auto* integer = std::get_if<int64_t>(&value)) return *integer;
    if (const auto* character = std::get_if<char>(&value)) return charCode(*character);
    if (const auto* number = std::get_if<float>(&value)) {
        if (!std::isfinite(*number)) {
            throw std::runtime_error("Cannot convert non-finite float to int.");
        }
        if (*number < static_cast<float>(std::numeric_limits<int64_t>::min()) ||
            *number > static_cast<float>(std::numeric_limits<int64_t>::max())) {
            throw std::runtime_error("Float value is out of int range.");
        }
        return static_cast<int64_t>(*number);
    }
    if (const auto* number = std::get_if<double>(&value)) {
        if (!std::isfinite(*number)) {
            throw std::runtime_error("Cannot convert non-finite double to int.");
        }
        if (*number < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
            *number > static_cast<double>(std::numeric_limits<int64_t>::max())) {
            throw std::runtime_error("Double value is out of int range.");
        }
        return static_cast<int64_t>(*number);
    }
    throw std::runtime_error("Expected numeric value, got " + valueTypeName(value) + ".");
}

inline Value defaultValueForDeclType(DeclType type) {
    switch (type) {
        case DeclType::Auto: return Value{std::monostate{}};
        case DeclType::Int: return Value{int32_t{0}};
        case DeclType::Long:
            if constexpr (sizeof(long) <= sizeof(int32_t)) {
                return Value{int32_t{0}};
            } else {
                return Value{int64_t{0}};
            }
        case DeclType::LongLong: return Value{int64_t{0}};
        case DeclType::Float: return Value{float{0.0f}};
        case DeclType::Double: return Value{0.0};
        case DeclType::Bool: return Value{false};
        case DeclType::Char: return Value{'\0'};
    }

    return Value{std::monostate{}};
}

template <typename Float>
inline std::string formatFloating(Float value) {
    std::ostringstream out;
    const long double wide = static_cast<long double>(value);
    const long double rounded = std::round(wide);
    const long double epsilon =
        std::is_same_v<Float, float> ? static_cast<long double>(1e-6f) : static_cast<long double>(1e-12);

    if (std::fabs(wide - rounded) < epsilon &&
        rounded >= static_cast<long double>(std::numeric_limits<int64_t>::min()) &&
        rounded <= static_cast<long double>(std::numeric_limits<int64_t>::max())) {
        out << static_cast<int64_t>(rounded);
    } else {
        out << std::setprecision(std::numeric_limits<Float>::max_digits10) << value;
    }

    return out.str();
}

inline std::string formatValue(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return "nil";
    }

    if (const auto* integer = std::get_if<int32_t>(&value)) {
        return std::to_string(*integer);
    }

    if (const auto* integer = std::get_if<int64_t>(&value)) {
        return std::to_string(*integer);
    }

    if (const auto* number = std::get_if<float>(&value)) {
        return formatFloating(*number);
    }

    if (const auto* number = std::get_if<double>(&value)) {
        return formatFloating(*number);
    }

    if (const auto* character = std::get_if<char>(&value)) {
        return charToString(*character);
    }

    return std::get<bool>(value) ? "true" : "false";
}

inline bool valuesEqual(const Value& left, const Value& right) {
    if (isNil(left) && isNil(right)) return true;
    if (isNil(left) || isNil(right)) return false;

    if (isNumber(left) && isNumber(right)) {
        return toLongDouble(left) == toLongDouble(right);
    }

    if (isBoolean(left) && isBoolean(right)) {
        return std::get<bool>(left) == std::get<bool>(right);
    }

    return false;
}

template <typename Int>
inline bool addOverflows(Int a, Int b) {
    return (b > 0 && a > std::numeric_limits<Int>::max() - b) ||
           (b < 0 && a < std::numeric_limits<Int>::min() - b);
}

template <typename Int>
inline bool subOverflows(Int a, Int b) {
    return (b < 0 && a > std::numeric_limits<Int>::max() + b) ||
           (b > 0 && a < std::numeric_limits<Int>::min() + b);
}

template <typename Int>
inline bool mulOverflows(Int a, Int b) {
    if (a == 0 || b == 0) return false;
    if (a == -1) return b == std::numeric_limits<Int>::min();
    if (b == -1) return a == std::numeric_limits<Int>::min();
    if (a > 0) {
        if (b > 0) return a > std::numeric_limits<Int>::max() / b;
        return b < std::numeric_limits<Int>::min() / a;
    }
    if (b > 0) return a < std::numeric_limits<Int>::min() / b;
    return a < std::numeric_limits<Int>::max() / b;
}

}  // namespace cvm
