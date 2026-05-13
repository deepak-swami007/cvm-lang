#pragma once

#include <string_view>

namespace cvm {

enum class DeclType { Auto, Int, Long, LongLong, Double, Float, Bool, Char };

inline std::string_view declTypeName(DeclType type) {
    switch (type) {
        case DeclType::Auto: return "auto";
        case DeclType::Int: return "int";
        case DeclType::Long: return "long";
        case DeclType::LongLong: return "long long";
        case DeclType::Double: return "double";
        case DeclType::Float: return "float";
        case DeclType::Bool: return "bool";
        case DeclType::Char: return "char";
    }

    return "unknown";
}

}  // namespace cvm
