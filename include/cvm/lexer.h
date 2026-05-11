#pragma once

#include <string>
#include <vector>

#include "cvm/token.h"

namespace cvm {

class Lexer {
  public:
    explicit Lexer(std::string source);

    std::vector<Token> scanTokens();

  private:
    bool isAtEnd() const;
    char advance();
    bool match(char expected);
    char peek() const;
    char peekNext() const;
    void addToken(TokenType type);
    void addToken(TokenType type, double number, bool isInteger = false);
    void scanToken();
    void number();
    void identifier();

    static bool isDigit(char c);
    static bool isAlpha(char c);
    static bool isAlphaNumeric(char c);

    std::string source_;
    std::vector<Token> tokens_;
    std::size_t start_ = 0;
    std::size_t current_ = 0;
    std::size_t line_ = 1;
};

}  // namespace cvm

