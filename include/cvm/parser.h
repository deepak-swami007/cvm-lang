#pragma once

#include <initializer_list>
#include <string_view>
#include <vector>

#include "cvm/ast.h"
#include "cvm/token.h"

namespace cvm {

class Parser {
  public:
    explicit Parser(std::vector<Token> tokens);

    std::vector<StmtPtr> parse();

  private:
    StmtPtr declaration();
    StmtPtr variableDeclaration();
    StmtPtr statement();
    StmtPtr blockStatement();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr printStatement();
    StmtPtr expressionStatement();

    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr primary();

    bool match(std::initializer_list<TokenType> types);
    bool check(TokenType type) const;
    const Token& advance();
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& consume(TokenType type, std::string_view message);
    [[noreturn]] void error(const Token& token, std::string_view message) const;

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
};

}  // namespace cvm
