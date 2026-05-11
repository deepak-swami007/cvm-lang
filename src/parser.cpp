#include "cvm/parser.h"

#include <stdexcept>
#include <utility>

namespace cvm {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> program;
    while (!isAtEnd()) {
        program.push_back(declaration());
    }
    return program;
}

StmtPtr Parser::declaration() {
    if (match({TokenType::Let})) {
        return variableDeclaration();
    }
    return statement();
}

StmtPtr Parser::variableDeclaration() {
    const Token& name = consume(TokenType::Identifier, "Expected variable name after 'let'.");
    consume(TokenType::Equal, "Expected '=' after variable name.");
    ExprPtr initializer = expression();
    consume(TokenType::Semicolon, "Expected ';' after variable declaration.");
    return std::make_unique<Stmt>(VarDeclStmt{name, std::move(initializer)});
}

StmtPtr Parser::statement() {
    if (match({TokenType::Print})) {
        return printStatement();
    }
    return expressionStatement();
}

StmtPtr Parser::printStatement() {
    ExprPtr value = expression();
    consume(TokenType::Semicolon, "Expected ';' after value.");
    return std::make_unique<Stmt>(PrintStmt{std::move(value)});
}

StmtPtr Parser::expressionStatement() {
    ExprPtr value = expression();
    consume(TokenType::Semicolon, "Expected ';' after expression.");
    return std::make_unique<Stmt>(ExpressionStmt{std::move(value)});
}

ExprPtr Parser::expression() {
    return assignment();
}

ExprPtr Parser::assignment() {
    ExprPtr expr = equality();

    if (match({TokenType::Equal})) {
        Token equals = previous();
        ExprPtr value = assignment();

        if (auto* variable = std::get_if<VariableExpr>(&expr->value)) {
            return std::make_unique<Expr>(AssignExpr{variable->name, std::move(value)});
        }

        error(equals, "Invalid assignment target.");
    }

    return expr;
}

ExprPtr Parser::equality() {
    ExprPtr expr = comparison();

    while (match({TokenType::BangEqual, TokenType::EqualEqual})) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = std::make_unique<Expr>(BinaryExpr{std::move(expr), std::move(op), std::move(right)});
    }

    return expr;
}

ExprPtr Parser::comparison() {
    ExprPtr expr = term();

    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        Token op = previous();
        ExprPtr right = term();
        expr = std::make_unique<Expr>(BinaryExpr{std::move(expr), std::move(op), std::move(right)});
    }

    return expr;
}

ExprPtr Parser::term() {
    ExprPtr expr = factor();

    while (match({TokenType::Plus, TokenType::Minus})) {
        Token op = previous();
        ExprPtr right = factor();
        expr = std::make_unique<Expr>(BinaryExpr{std::move(expr), std::move(op), std::move(right)});
    }

    return expr;
}

ExprPtr Parser::factor() {
    ExprPtr expr = unary();

    while (match({TokenType::Star, TokenType::Slash})) {
        Token op = previous();
        ExprPtr right = unary();
        expr = std::make_unique<Expr>(BinaryExpr{std::move(expr), std::move(op), std::move(right)});
    }

    return expr;
}

ExprPtr Parser::unary() {
    if (match({TokenType::Minus})) {
        Token op = previous();
        ExprPtr right = unary();
        return std::make_unique<Expr>(UnaryExpr{std::move(op), std::move(right)});
    }

    return primary();
}

ExprPtr Parser::primary() {
    if (match({TokenType::Number})) {
        return std::make_unique<Expr>(LiteralExpr{*previous().number});
    }

    if (match({TokenType::Identifier})) {
        return std::make_unique<Expr>(VariableExpr{previous()});
    }

    if (match({TokenType::LeftParen})) {
        ExprPtr expr = expression();
        consume(TokenType::RightParen, "Expected ')' after expression.");
        return std::make_unique<Expr>(GroupingExpr{std::move(expr)});
    }

    error(peek(), "Expected a number, identifier, or parenthesized expression.");
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) {
        return type == TokenType::EndOfFile;
    }
    return peek().type == type;
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        ++current_;
    }
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EndOfFile;
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

const Token& Parser::consume(TokenType type, std::string_view message) {
    if (check(type)) {
        return advance();
    }
    error(peek(), message);
}

[[noreturn]] void Parser::error(const Token& token, std::string_view message) const {
    throw std::runtime_error(
        "Parse error at line " + std::to_string(token.line) + " near '" + token.lexeme + "': " + std::string(message));
}

}  // namespace cvm
