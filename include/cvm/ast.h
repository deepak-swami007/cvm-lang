#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "cvm/token.h"

namespace cvm {

class Expr;
using ExprPtr = std::unique_ptr<Expr>;

class LiteralExpr {
  public:
    double value;
};

class GroupingExpr {
  public:
    ExprPtr expression;
};

class UnaryExpr {
  public:
    Token op;
    ExprPtr right;
};

class BinaryExpr {
  public:
    ExprPtr left;
    Token op;
    ExprPtr right;
};

class Expr {
  public:
    using Variant = std::variant<LiteralExpr, GroupingExpr, UnaryExpr, BinaryExpr>;

    template <typename T>
    explicit Expr(T node) : value(std::move(node)) {}

    Variant value;
};

class Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

class PrintStmt {
  public:
    ExprPtr expression;
};

class ExpressionStmt {
  public:
    ExprPtr expression;
};

class Stmt {
  public:
    using Variant = std::variant<PrintStmt, ExpressionStmt>;

    template <typename T>
    explicit Stmt(T node) : value(std::move(node)) {}

    Variant value;
};

namespace detail {

inline std::string parenthesize(std::string_view name, const std::vector<const Expr*>& expressions);

inline std::string exprToString(const Expr& expr) {
    return std::visit(
        [](const auto& node) -> std::string {
            using T = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<T, LiteralExpr>) {
                std::ostringstream out;
                out << node.value;
                return out.str();
            } else if constexpr (std::is_same_v<T, GroupingExpr>) {
                return parenthesize("group", {node.expression.get()});
            } else if constexpr (std::is_same_v<T, UnaryExpr>) {
                return parenthesize(node.op.lexeme, {node.right.get()});
            } else {
                return parenthesize(node.op.lexeme, {node.left.get(), node.right.get()});
            }
        },
        expr.value);
}

inline std::string parenthesize(std::string_view name, const std::vector<const Expr*>& expressions) {
    std::string result = "(" + std::string(name);
    for (const Expr* expression : expressions) {
        result += " ";
        result += exprToString(*expression);
    }
    result += ")";
    return result;
}

}  // namespace detail

inline std::string toString(const Expr& expr) {
    return detail::exprToString(expr);
}

inline std::string toString(const Stmt& stmt) {
    return std::visit(
        [](const auto& node) -> std::string {
            using T = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<T, PrintStmt>) {
                return "(print " + toString(*node.expression) + ")";
            } else {
                return "(expr " + toString(*node.expression) + ")";
            }
        },
        stmt.value);
}

inline std::string toString(const std::vector<StmtPtr>& program) {
    std::ostringstream out;
    for (std::size_t i = 0; i < program.size(); ++i) {
        out << toString(*program[i]);
        if (i + 1 != program.size()) {
            out << '\n';
        }
    }
    return out.str();
}

}  // namespace cvm
