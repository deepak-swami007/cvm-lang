#pragma once

#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <variant>

namespace cvm {

using Value = std::variant<std::monostate, double, bool>;

inline bool isNil(const Value& value) {
    return std::holds_alternative<std::monostate>(value);
}

inline bool isNumber(const Value& value) {
    return std::holds_alternative<double>(value);
}

inline bool isBoolean(const Value& value) {
    return std::holds_alternative<bool>(value);
}

inline std::string valueTypeName(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return "nil";
    }
    if (std::holds_alternative<double>(value)) {
        return "number";
    }
    return "bool";
}

inline std::string formatValue(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return "nil";
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

    return std::get<bool>(value) ? "true" : "false";
}

inline bool valuesEqual(const Value& left, const Value& right) {
    if (left.index() != right.index()) {
        return false;
    }

    if (std::holds_alternative<std::monostate>(left)) {
        return true;
    }

    if (const auto* leftNumber = std::get_if<double>(&left)) {
        return *leftNumber == std::get<double>(right);
    }

    return std::get<bool>(left) == std::get<bool>(right);
}

}  // namespace cvm
