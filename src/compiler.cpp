#include "cvm/compiler.h"

#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace cvm {

Chunk Compiler::compile(const std::vector<StmtPtr> &program) {
  chunk_ = {};
  declaredGlobals_.clear();
  allGlobalNames_.clear();

  collectGlobalDeclarations(program);
  for (const std::string &name : chunk_.names) {
    emitNil();
    chunk_.writeOp(OpCode::DefineGlobal);
    chunk_.writeByte(chunk_.addName(name));
  }

  for (const auto &statement : program) {
    emitStatement(*statement);
  }

  chunk_.writeOp(OpCode::Halt);
  return chunk_;
}

void Compiler::collectGlobalDeclarations(const std::vector<StmtPtr> &program) {
  for (const auto &statement : program) {
    collectGlobalDeclarations(*statement);
  }
}

void Compiler::collectGlobalDeclarations(const Stmt &stmt) {
  std::visit(
      [this](const auto &node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, VarDeclStmt>) {
          if (allGlobalNames_.contains(node.name.lexeme)) {
            throw std::runtime_error("Variable '" + node.name.lexeme +
                                     "' is already declared.");
          }

          allGlobalNames_.insert(node.name.lexeme);
          chunk_.addName(node.name.lexeme);
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
          collectGlobalDeclarations(node.statements);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          collectGlobalDeclarations(*node.thenBranch);
          if (node.elseBranch) {
            collectGlobalDeclarations(*node.elseBranch);
          }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
          collectGlobalDeclarations(*node.body);
        }
        // InputStmt does not declare new globals, nothing to collect.
      },
      stmt.value);
}

void Compiler::emitStatement(const Stmt &stmt) {
  std::visit(
      [this](const auto &node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, PrintStmt>) {
          emitExpression(*node.expression);
          chunk_.writeOp(OpCode::Print);
        } else if constexpr (std::is_same_v<T, VarDeclStmt>) {
          emitExpression(*node.initializer);
          // Emit type cast if a specific type was declared
          if (node.declType == DeclType::Int || node.declType == DeclType::Long) {
            chunk_.writeOp(OpCode::CastToInt);
          } else if (node.declType == DeclType::Double || node.declType == DeclType::Float) {
            chunk_.writeOp(OpCode::CastToDouble);
          }
          chunk_.writeOp(OpCode::SetGlobal);
          chunk_.writeByte(chunk_.addName(node.name.lexeme));
          chunk_.writeOp(OpCode::Pop);
          declaredGlobals_.insert(node.name.lexeme);
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
          for (const auto &statement : node.statements) {
            emitStatement(*statement);
          }
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          emitExpression(*node.condition);
          const std::size_t thenJump = emitJump(OpCode::JumpIfFalse);
          chunk_.writeOp(OpCode::Pop);
          emitStatement(*node.thenBranch);
          const std::size_t elseJump = emitJump(OpCode::Jump);
          patchJump(thenJump);
          chunk_.writeOp(OpCode::Pop);
          if (node.elseBranch) {
            emitStatement(*node.elseBranch);
          }
          patchJump(elseJump);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
          const std::size_t loopStart = chunk_.code.size();
          emitExpression(*node.condition);
          const std::size_t exitJump = emitJump(OpCode::JumpIfFalse);
          chunk_.writeOp(OpCode::Pop);
          emitStatement(*node.body);
          emitLoop(loopStart);
          patchJump(exitJump);
          chunk_.writeOp(OpCode::Pop);
        } else if constexpr (std::is_same_v<T, InputStmt>) {
          ensureDeclaredGlobal(node.name);
          chunk_.writeOp(OpCode::Input);
          chunk_.writeByte(chunk_.addName(node.name.lexeme));
        } else {
          emitExpression(*node.expression);
          chunk_.writeOp(OpCode::Pop);
        }
      },
      stmt.value);
}

void Compiler::emitExpression(const Expr &expr) {
  std::visit(
      [this](const auto &node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, LiteralExpr>) {
          if (isNil(node.value)) {
            emitNil();
          } else if (const auto *boolean = std::get_if<bool>(&node.value)) {
            chunk_.writeOp(*boolean ? OpCode::True : OpCode::False);
          } else {
            emitConstant(node.value);
          }
        } else if constexpr (std::is_same_v<T, GroupingExpr>) {
          emitExpression(*node.expression);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
          emitExpression(*node.right);
          if (node.op.type == TokenType::Minus) {
            chunk_.writeOp(OpCode::Negate);
            return;
          }
          if (node.op.type == TokenType::Bang) {
            chunk_.writeOp(OpCode::Not);
            return;
          }
          if (node.op.type == TokenType::Tilde) {
            chunk_.writeOp(OpCode::BitwiseNot);
            return;
          }
          throw std::runtime_error("Unsupported unary operator '" +
                                   node.op.lexeme + "'.");
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
          case TokenType::BangEqual:
            chunk_.writeOp(OpCode::Equal);
            chunk_.writeOp(OpCode::Not);
            return;
          case TokenType::EqualEqual:
            chunk_.writeOp(OpCode::Equal);
            return;
          case TokenType::Greater:
            chunk_.writeOp(OpCode::Greater);
            return;
          case TokenType::GreaterEqual:
            chunk_.writeOp(OpCode::GreaterEqual);
            return;
          case TokenType::Less:
            chunk_.writeOp(OpCode::Less);
            return;
          case TokenType::LessEqual:
            chunk_.writeOp(OpCode::LessEqual);
            return;
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
          case TokenType::Percent:
            chunk_.writeOp(OpCode::Modulo);
            return;
          case TokenType::Caret:
            chunk_.writeOp(OpCode::Power);
            return;
          case TokenType::Ampersand:
            chunk_.writeOp(OpCode::BitwiseAnd);
            return;
          case TokenType::Pipe:
            chunk_.writeOp(OpCode::BitwiseOr);
            return;
          default:
            throw std::runtime_error("Unsupported binary operator '" +
                                     node.op.lexeme + "'.");
          }
        }
      },
      expr.value);
}

void Compiler::emitConstant(Value value) {
  chunk_.writeOp(OpCode::Constant);
  chunk_.writeByte(chunk_.addConstant(value));
}

void Compiler::emitNil() { chunk_.writeOp(OpCode::Nil); }

std::size_t Compiler::emitJump(OpCode op) {
  chunk_.writeOp(op);
  chunk_.writeByte(0xff);
  chunk_.writeByte(0xff);
  return chunk_.code.size() - 2;
}

void Compiler::patchJump(std::size_t operandOffset) {
  if (chunk_.code.size() < operandOffset + 2) {
    throw std::runtime_error("Internal compiler error while patching jump.");
  }

  const std::size_t jump = chunk_.code.size() - (operandOffset + 2);
  if (jump > 0xffff) {
    throw std::runtime_error("Too much code to jump over.");
  }

  chunk_.code[operandOffset] = static_cast<std::uint8_t>((jump >> 8) & 0xff);
  chunk_.code[operandOffset + 1] = static_cast<std::uint8_t>(jump & 0xff);
}

void Compiler::emitLoop(std::size_t loopStart) {
  chunk_.writeOp(OpCode::Loop);
  const std::size_t jump = chunk_.code.size() + 2 - loopStart;
  if (jump > 0xffff) {
    throw std::runtime_error("Loop body is too large.");
  }

  chunk_.writeByte(static_cast<std::uint8_t>((jump >> 8) & 0xff));
  chunk_.writeByte(static_cast<std::uint8_t>(jump & 0xff));
}

void Compiler::ensureDeclaredGlobal(const Token &name) const {
  if (!declaredGlobals_.contains(name.lexeme)) {
    throw std::runtime_error("Undefined variable '" + name.lexeme +
                             "' on line " + std::to_string(name.line) + ".");
  }
}

} // namespace cvm
