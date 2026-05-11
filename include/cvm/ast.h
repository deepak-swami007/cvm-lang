#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "cvm/token.h"
#include "cvm/value.h"

namespace cvm {

class Expr;
using ExprPtr = std::unique_ptr<Expr>;

class LiteralExpr {
  public:
    Value value;
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

class VariableExpr {
  public:
    Token name;
};

class AssignExpr {
  public:
    Token name;
    ExprPtr value;
};

class BinaryExpr {
  public:
    ExprPtr left;
    Token op;
    ExprPtr right;
};

class Expr {
  public:
    using Variant = std::variant<LiteralExpr, GroupingExpr, UnaryExpr, VariableExpr, AssignExpr, BinaryExpr>;

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

class VarDeclStmt {
  public:
    Token name;
    ExprPtr initializer;
};

class BlockStmt {
  public:
    std::vector<StmtPtr> statements;
};

class IfStmt {
  public:
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
};

class WhileStmt {
  public:
    ExprPtr condition;
    StmtPtr body;
};

class InputStmt {
  public:
    Token name;
};

class Stmt {
  public:
    using Variant = std::variant<PrintStmt, ExpressionStmt, VarDeclStmt, BlockStmt, IfStmt, WhileStmt, InputStmt>;

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
                return formatValue(node.value);
            } else if constexpr (std::is_same_v<T, GroupingExpr>) {
                return parenthesize("group", {node.expression.get()});
            } else if constexpr (std::is_same_v<T, UnaryExpr>) {
                return parenthesize(node.op.lexeme, {node.right.get()});
            } else if constexpr (std::is_same_v<T, VariableExpr>) {
                return node.name.lexeme;
            } else if constexpr (std::is_same_v<T, AssignExpr>) {
                return "(assign " + node.name.lexeme + " " + exprToString(*node.value) + ")";
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
            } else if constexpr (std::is_same_v<T, VarDeclStmt>) {
                return "(let " + node.name.lexeme + " " + toString(*node.initializer) + ")";
            } else if constexpr (std::is_same_v<T, BlockStmt>) {
                std::string result = "(block";
                for (const auto& statement : node.statements) {
                    result += " " + toString(*statement);
                }
                result += ")";
                return result;
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                std::string result =
                    "(if " + toString(*node.condition) + " " + toString(*node.thenBranch);
                if (node.elseBranch) {
                    result += " " + toString(*node.elseBranch);
                }
                result += ")";
                return result;
            } else if constexpr (std::is_same_v<T, WhileStmt>) {
                return "(while " + toString(*node.condition) + " " + toString(*node.body) + ")";
            } else if constexpr (std::is_same_v<T, InputStmt>) {
                return "(input " + node.name.lexeme + ")";
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
