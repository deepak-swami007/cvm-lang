#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <variant>

namespace cvm {

using Value = std::variant<std::monostate, int64_t, double, bool, std::string>;

inline bool isNil(const Value& value) {
    return std::holds_alternative<std::monostate>(value);
}

inline bool isInteger(const Value& value) {
    return std::holds_alternative<int64_t>(value);
}

inline bool isDouble(const Value& value) {
    return std::holds_alternative<double>(value);
}

inline bool isNumber(const Value& value) {
    return isInteger(value) || isDouble(value);
}

inline bool isBoolean(const Value& value) {
    return std::holds_alternative<bool>(value);
}

inline bool isString(const Value& value) {
    return std::holds_alternative<std::string>(value);
}

inline std::string valueTypeName(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) return "nil";
    if (std::holds_alternative<int64_t>(value)) return "int";
    if (std::holds_alternative<double>(value)) return "double";
    if (std::holds_alternative<bool>(value)) return "bool";
    if (std::holds_alternative<std::string>(value)) return "string";
    return "unknown";
}

// Get numeric value as double (works for both int and double)
inline double toDouble(const Value& value) {
    if (const auto* i = std::get_if<int64_t>(&value)) return static_cast<double>(*i);
    if (const auto* d = std::get_if<double>(&value)) return *d;
    return 0.0;
}

// Get numeric value as int64 (truncates doubles)
inline int64_t toInt64(const Value& value) {
    if (const auto* i = std::get_if<int64_t>(&value)) return *i;
    if (const auto* d = std::get_if<double>(&value)) return static_cast<int64_t>(*d);
    return 0;
}

inline std::string formatValue(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return "nil";
    }

    if (const auto* integer = std::get_if<int64_t>(&value)) {
        return std::to_string(*integer);
    }

    if (const auto* number = std::get_if<double>(&value)) {
        std::ostringstream out;
        const double rounded = std::round(*number);
        if (std::fabs(*number - rounded) < 1e-9) {
            out << static_cast<std::int64_t>(rounded);
        } else {
            out << *number;
        }
        return out.str();
    }

    if (const auto* str = std::get_if<std::string>(&value)) {
        return *str;
    }

    return std::get<bool>(value) ? "true" : "false";
}

inline bool valuesEqual(const Value& left, const Value& right) {
    // nil == nil
    if (isNil(left) && isNil(right)) return true;
    if (isNil(left) || isNil(right)) return false;

    // Both numeric — compare as doubles for cross-type equality
    if (isNumber(left) && isNumber(right)) {
        return toDouble(left) == toDouble(right);
    }

    // bool == bool
    if (isBoolean(left) && isBoolean(right)) {
        return std::get<bool>(left) == std::get<bool>(right);
    }

    // string == string
    if (isString(left) && isString(right)) {
        return std::get<std::string>(left) == std::get<std::string>(right);
    }

    return false;
}

// ── Overflow detection (C++ style) ──────────────────────────────────

inline bool addOverflows(int64_t a, int64_t b) {
    return (b > 0 && a > std::numeric_limits<int64_t>::max() - b) ||
           (b < 0 && a < std::numeric_limits<int64_t>::min() - b);
}

inline bool subOverflows(int64_t a, int64_t b) {
    return (b < 0 && a > std::numeric_limits<int64_t>::max() + b) ||
           (b > 0 && a < std::numeric_limits<int64_t>::min() + b);
}

inline bool mulOverflows(int64_t a, int64_t b) {
    if (a == 0 || b == 0) return false;
    if (a == -1) return b == std::numeric_limits<int64_t>::min();
    if (b == -1) return a == std::numeric_limits<int64_t>::min();
    if (a > 0) {
        if (b > 0) return a > std::numeric_limits<int64_t>::max() / b;
        return b < std::numeric_limits<int64_t>::min() / a;
    }
    if (b > 0) return a < std::numeric_limits<int64_t>::min() / b;
    return a < std::numeric_limits<int64_t>::max() / b;
}

}  // namespace cvm
