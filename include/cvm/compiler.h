#pragma once

#include <string>
#include <unordered_set>
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
    void ensureDeclaredGlobal(const Token& name) const;

    Chunk chunk_;
    std::unordered_set<std::string> declaredGlobals_;
};

}  // namespace cvm
