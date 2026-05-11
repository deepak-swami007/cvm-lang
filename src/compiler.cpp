#include "cvm/compiler.h"

#include <stdexcept>
#include <type_traits>

namespace cvm {

Chunk Compiler::compile(const std::vector<StmtPtr>& program) {
    chunk_ = {};
    declaredGlobals_.clear();

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
            } else if constexpr (std::is_same_v<T, VarDeclStmt>) {
                if (declaredGlobals_.contains(node.name.lexeme)) {
                    throw std::runtime_error("Variable '" + node.name.lexeme + "' is already declared.");
                }

                emitExpression(*node.initializer);
                chunk_.writeOp(OpCode::DefineGlobal);
                chunk_.writeByte(chunk_.addName(node.name.lexeme));
                declaredGlobals_.insert(node.name.lexeme);
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
            } else if constexpr (std::is_same_v<T, VariableExpr>) {
                ensureDeclaredGlobal(node.name);
                chunk_.writeOp(OpCode::GetGlobal);
                chunk_.writeByte(chunk_.addName(node.name.lexeme));
            } else if constexpr (std::is_same_v<T, AssignExpr>) {
                ensureDeclaredGlobal(node.name);
                emitExpression(*node.value);
                chunk_.writeOp(OpCode::SetGlobal);
                chunk_.writeByte(chunk_.addName(node.name.lexeme));
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

void Compiler::ensureDeclaredGlobal(const Token& name) const {
    if (!declaredGlobals_.contains(name.lexeme)) {
        throw std::runtime_error(
            "Undefined variable '" + name.lexeme + "' on line " + std::to_string(name.line) + ".");
    }
}

}  // namespace cvm
