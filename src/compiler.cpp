#include "cvm/compiler.h"

#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace cvm {

namespace {

void emitValueLiteral(Chunk& chunk, const Value& value) {
  if (isNil(value)) {
    chunk.writeOp(OpCode::Nil);
    return;
  }

  chunk.writeOp(OpCode::Constant);
  chunk.writeByte(chunk.addConstant(value));
}

}  // namespace

Chunk Compiler::compile(const std::vector<StmtPtr> &program) {
  chunk_ = {};
  declaredGlobals_.clear();
  allGlobalNames_.clear();
  loopStack_.clear();

  collectGlobalDeclarations(program);
  for (std::size_t index = 0; index < chunk_.names.size(); ++index) {
    emitValueLiteral(chunk_, defaultValueForDeclType(chunk_.nameTypes[index]));
    chunk_.writeOp(OpCode::DefineGlobal);
    chunk_.writeByte(static_cast<std::uint8_t>(index));
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
          chunk_.addName(node.name.lexeme, node.declType);
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
          collectGlobalDeclarations(node.statements);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          collectGlobalDeclarations(*node.thenBranch);
          if (node.elseBranch) {
            collectGlobalDeclarations(*node.elseBranch);
          }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
          collectGlobalDeclarations(*node.body);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
          if (node.initializer) {
            collectGlobalDeclarations(*node.initializer);
          }
          collectGlobalDeclarations(*node.body);
        }
        // InputStmt, BreakStmt, ContinueStmt do not declare new globals.
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
          chunk_.currentLine = node.name.line;
          emitExpression(*node.initializer);
          chunk_.writeOp(OpCode::SetGlobal);
          chunk_.writeByte(chunk_.addName(node.name.lexeme, node.declType));
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

          // Push loop context
          loopStack_.push_back({loopStart, false, {}, {}});
          emitStatement(*node.body);
          LoopContext ctx = std::move(loopStack_.back());
          loopStack_.pop_back();

          emitLoop(loopStart);
          patchJump(exitJump);
          chunk_.writeOp(OpCode::Pop);

          // Patch break jumps to after the loop
          for (std::size_t breakJump : ctx.breakJumps) {
            patchJump(breakJump);
          }

        } else if constexpr (std::is_same_v<T, ForStmt>) {
          // Compile initializer (if any)
          if (node.initializer) {
            emitStatement(*node.initializer);
          }

          const std::size_t loopStart = chunk_.code.size();

          // Compile condition (or push true for infinite loop)
          if (node.condition) {
            emitExpression(*node.condition);
          } else {
            chunk_.writeOp(OpCode::True);
          }
          const std::size_t exitJump = emitJump(OpCode::JumpIfFalse);
          chunk_.writeOp(OpCode::Pop);

          // Push loop context (for-loop: continue jumps need patching)
          loopStack_.push_back({loopStart, true, {}, {}});
          emitStatement(*node.body);
          LoopContext ctx = std::move(loopStack_.back());
          loopStack_.pop_back();

          // Patch continue jumps to here (the increment)
          for (std::size_t continueJump : ctx.continueJumps) {
            patchJump(continueJump);
          }

          // Compile increment (if any)
          if (node.increment) {
            emitExpression(*node.increment);
            chunk_.writeOp(OpCode::Pop);
          }

          emitLoop(loopStart);
          patchJump(exitJump);
          chunk_.writeOp(OpCode::Pop);

          // Patch break jumps to after the loop
          for (std::size_t breakJump : ctx.breakJumps) {
            patchJump(breakJump);
          }

        } else if constexpr (std::is_same_v<T, InputStmt>) {
          chunk_.currentLine = node.name.line;
          ensureDeclaredGlobal(node.name);
          chunk_.writeOp(OpCode::Input);
          chunk_.writeByte(chunk_.addName(node.name.lexeme));

        } else if constexpr (std::is_same_v<T, BreakStmt>) {
          chunk_.currentLine = node.keyword.line;
          if (loopStack_.empty()) {
            throw std::runtime_error(
                "Cannot use 'break' outside of a loop on line " +
                std::to_string(node.keyword.line) + ".");
          }
          const std::size_t breakJump = emitJump(OpCode::Jump);
          loopStack_.back().breakJumps.push_back(breakJump);

        } else if constexpr (std::is_same_v<T, ContinueStmt>) {
          chunk_.currentLine = node.keyword.line;
          if (loopStack_.empty()) {
            throw std::runtime_error(
                "Cannot use 'continue' outside of a loop on line " +
                std::to_string(node.keyword.line) + ".");
          }
          if (loopStack_.back().isForLoop) {
            // For-loop: jump forward to increment (patched later)
            const std::size_t continueJump = emitJump(OpCode::Jump);
            loopStack_.back().continueJumps.push_back(continueJump);
          } else {
            // While-loop: jump back to condition
            emitLoop(loopStack_.back().loopStart);
          }

        } else {
          // ExpressionStmt
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
          chunk_.currentLine = node.op.line;
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
          chunk_.currentLine = node.name.line;
          ensureDeclaredGlobal(node.name);
          chunk_.writeOp(OpCode::GetGlobal);
          chunk_.writeByte(chunk_.addName(node.name.lexeme));
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          emitExpression(*node.value);
          chunk_.currentLine = node.name.line;
          ensureDeclaredGlobal(node.name);
          chunk_.writeOp(OpCode::SetGlobal);
          chunk_.writeByte(chunk_.addName(node.name.lexeme));

        } else if constexpr (std::is_same_v<T, LogicalExpr>) {
          // Short-circuit evaluation
          chunk_.currentLine = node.op.line;
          if (node.op.type == TokenType::AmpAmp) {
            // AND: if left is false, skip right
            emitExpression(*node.left);
            const std::size_t endJump = emitJump(OpCode::JumpIfFalse);
            chunk_.writeOp(OpCode::Pop);
            emitExpression(*node.right);
            patchJump(endJump);
          } else {
            // OR: if left is true, skip right
            emitExpression(*node.left);
            const std::size_t falseJump = emitJump(OpCode::JumpIfFalse);
            const std::size_t endJump = emitJump(OpCode::Jump);
            patchJump(falseJump);
            chunk_.writeOp(OpCode::Pop);
            emitExpression(*node.right);
            patchJump(endJump);
          }

        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
          emitExpression(*node.left);
          emitExpression(*node.right);
          chunk_.currentLine = node.op.line;

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
