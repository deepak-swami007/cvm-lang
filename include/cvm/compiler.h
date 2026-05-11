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
    void collectGlobalDeclarations(const std::vector<StmtPtr>& program);
    void collectGlobalDeclarations(const Stmt& stmt);
    void emitStatement(const Stmt& stmt);
    void emitExpression(const Expr& expr);
    void emitConstant(Value value);
    void emitNil();
    std::size_t emitJump(OpCode op);
    void patchJump(std::size_t operandOffset);
    void emitLoop(std::size_t loopStart);
    void ensureDeclaredGlobal(const Token& name) const;

    Chunk chunk_;
    std::unordered_set<std::string> declaredGlobals_;
    std::unordered_set<std::string> allGlobalNames_;
};

}  // namespace cvm
