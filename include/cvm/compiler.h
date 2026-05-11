#pragma once

#include <vector>

#include "cvm/ast.h"
#include "cvm/bytecode.h"

namespace cvm {

class Compiler {
  public:
    Chunk compile(const std::vector<StmtPtr>& program);

  private:
    void emitStatement(const Stmt& stmt);
    void emitExpression(const Expr& expr);
    void emitConstant(Value value);

    Chunk chunk_;
};

}  // namespace cvm

