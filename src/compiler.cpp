#include "cvm/compiler.h"

#include <stdexcept>
#include <type_traits>

namespace cvm {

Chunk Compiler::compile(const std::vector<StmtPtr>& program) {
    chunk_ = {};

    for (const auto& statement : program) {
        emitStatement(*statement);
    }

    chunk_.writeOp(OpCode::Halt);
    return chunk_;
}

void Compiler::emitStatement(const Stmt& stmt) {
    std::visit(
        [this](const auto& node) {
            using T = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<T, PrintStmt>) {
                emitExpression(*node.expression);
                chunk_.writeOp(OpCode::Print);
            } else {
                emitExpression(*node.expression);
                chunk_.writeOp(OpCode::Pop);
            }
        },
        stmt.value);
}

void Compiler::emitExpression(const Expr& expr) {
    std::visit(
        [this](const auto& node) {
            using T = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<T, LiteralExpr>) {
                emitConstant(node.value);
            } else if constexpr (std::is_same_v<T, GroupingExpr>) {
                emitExpression(*node.expression);
            } else if constexpr (std::is_same_v<T, UnaryExpr>) {
                emitExpression(*node.right);
                if (node.op.type == TokenType::Minus) {
                    chunk_.writeOp(OpCode::Negate);
                    return;
                }
                throw std::runtime_error("Unsupported unary operator '" + node.op.lexeme + "'.");
            } else if constexpr (std::is_same_v<T, BinaryExpr>) {
                emitExpression(*node.left);
                emitExpression(*node.right);

                switch (node.op.type) {
                    case TokenType::Plus:
                        chunk_.writeOp(OpCode::Add);
                        return;
                    case TokenType::Minus:
                        chunk_.writeOp(OpCode::Subtract);
                        return;
                    case TokenType::Star:
                        chunk_.writeOp(OpCode::Multiply);
                        return;
                    case TokenType::Slash:
                        chunk_.writeOp(OpCode::Divide);
                        return;
                    default:
                        throw std::runtime_error("Unsupported binary operator '" + node.op.lexeme + "'.");
                }
            }
        },
        expr.value);
}

void Compiler::emitConstant(Value value) {
    chunk_.writeOp(OpCode::Constant);
    chunk_.writeByte(chunk_.addConstant(value));
}

}  // namespace cvm

