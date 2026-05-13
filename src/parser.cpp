#include "cvm/parser.h"

#include <stdexcept>
#include <utility>

namespace cvm {

namespace {

ExprPtr makeDefaultInitializer(DeclType declType) {
    return std::make_unique<Expr>(LiteralExpr{defaultValueForDeclType(declType)});
}

}  // namespace

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> program;
    while (!isAtEnd()) {
        program.push_back(declaration());
    }
    return program;
}

StmtPtr Parser::declaration() {
    if (const auto declType = matchDeclarationType()) {
        return variableDeclaration(*declType);
    }
    return statement();
}

StmtPtr Parser::variableDeclaration(DeclType declType) {
    const Token& name = consume(TokenType::Identifier, "Expected variable name after declaration keyword.");
    ExprPtr initializer;
    if (match({TokenType::Equal})) {
        initializer = expression();
    } else {
        initializer = makeDefaultInitializer(declType);
    }
    consume(TokenType::Semicolon, "Expected ';' after variable declaration.");

    VarDeclStmt decl;
    decl.name = name;
    decl.initializer = std::move(initializer);
    decl.declType = declType;
    return std::make_unique<Stmt>(std::move(decl));
}

StmtPtr Parser::statement() {
    if (match({TokenType::Print})) {
        return printStatement();
    }
    if (match({TokenType::Input})) {
        return inputStatement();
    }
    if (match({TokenType::If})) {
        return ifStatement();
    }
    if (match({TokenType::While})) {
        return whileStatement();
    }
    if (match({TokenType::For})) {
        return forStatement();
    }
    if (match({TokenType::Break})) {
        Token keyword = previous();
        consume(TokenType::Semicolon, "Expected ';' after 'break'.");
        return std::make_unique<Stmt>(BreakStmt{keyword});
    }
    if (match({TokenType::Continue})) {
        Token keyword = previous();
        consume(TokenType::Semicolon, "Expected ';' after 'continue'.");
        return std::make_unique<Stmt>(ContinueStmt{keyword});
    }
    if (match({TokenType::LeftBrace})) {
        return blockStatement();
    }
    return expressionStatement();
}

StmtPtr Parser::blockStatement() {
    std::vector<StmtPtr> statements;
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        statements.push_back(declaration());
    }

    consume(TokenType::RightBrace, "Expected '}' after block.");
    return std::make_unique<Stmt>(BlockStmt{std::move(statements)});
}

StmtPtr Parser::ifStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'if'.");
    ExprPtr condition = expression();
    consume(TokenType::RightParen, "Expected ')' after if condition.");

    StmtPtr thenBranch = declaration();
    StmtPtr elseBranch = nullptr;
    if (match({TokenType::Else})) {
        elseBranch = declaration();
    }

    return std::make_unique<Stmt>(IfStmt{std::move(condition), std::move(thenBranch), std::move(elseBranch)});
}

StmtPtr Parser::whileStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'while'.");
    ExprPtr condition = expression();
    consume(TokenType::RightParen, "Expected ')' after while condition.");
    StmtPtr body = declaration();
    return std::make_unique<Stmt>(WhileStmt{std::move(condition), std::move(body)});
}

StmtPtr Parser::forStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'for'.");

    // Initializer
    StmtPtr initializer;
    if (match({TokenType::Semicolon})) {
        // No initializer
    } else if (const auto declType = matchDeclarationType()) {
        initializer = variableDeclaration(*declType);
    } else {
        initializer = expressionStatement();
    }

    // Condition
    ExprPtr condition;
    if (!check(TokenType::Semicolon)) {
        condition = expression();
    }
    consume(TokenType::Semicolon, "Expected ';' after for-loop condition.");

    // Increment
    ExprPtr increment;
    if (!check(TokenType::RightParen)) {
        increment = expression();
    }
    consume(TokenType::RightParen, "Expected ')' after for clauses.");

    // Body
    StmtPtr body = declaration();

    return std::make_unique<Stmt>(ForStmt{
        std::move(initializer), std::move(condition), std::move(increment), std::move(body)});
}

StmtPtr Parser::printStatement() {
    ExprPtr value = expression();
    consume(TokenType::Semicolon, "Expected ';' after value.");
    return std::make_unique<Stmt>(PrintStmt{std::move(value)});
}

StmtPtr Parser::inputStatement() {
    const Token& name = consume(TokenType::Identifier, "Expected variable name after 'input'.");
    consume(TokenType::Semicolon, "Expected ';' after variable name.");
    return std::make_unique<Stmt>(InputStmt{name});
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
    ExprPtr expr = logicalOr();

    if (match({TokenType::Equal})) {
        Token equals = previous();
        ExprPtr value = assignment();

        if (auto* variable = std::get_if<VariableExpr>(&expr->value)) {
            return std::make_unique<Expr>(AssignExpr{variable->name, std::move(value)});
        }

        error(equals, "Invalid assignment target.");
    }

    // Compound assignment: +=, -=, *=, /=
    if (match({TokenType::PlusEqual, TokenType::MinusEqual, TokenType::StarEqual, TokenType::SlashEqual})) {
        Token op = previous();

        // Determine the arithmetic operator token type
        TokenType arithType;
        std::string arithLexeme;
        switch (op.type) {
            case TokenType::PlusEqual:  arithType = TokenType::Plus;  arithLexeme = "+"; break;
            case TokenType::MinusEqual: arithType = TokenType::Minus; arithLexeme = "-"; break;
            case TokenType::StarEqual:  arithType = TokenType::Star;  arithLexeme = "*"; break;
            case TokenType::SlashEqual: arithType = TokenType::Slash; arithLexeme = "/"; break;
            default: error(op, "Unknown compound assignment operator."); break;
        }

        ExprPtr rhs = assignment();

        if (auto* variable = std::get_if<VariableExpr>(&expr->value)) {
            // Desugar: x += e  →  x = x + e
            Token arithToken{arithType, arithLexeme, op.line};
            auto varRead = std::make_unique<Expr>(VariableExpr{variable->name});
            auto binExpr = std::make_unique<Expr>(
                BinaryExpr{std::move(varRead), std::move(arithToken), std::move(rhs)});
            return std::make_unique<Expr>(AssignExpr{variable->name, std::move(binExpr)});
        }

        error(op, "Invalid compound assignment target.");
    }

    return expr;
}

ExprPtr Parser::logicalOr() {
    ExprPtr expr = logicalAnd();

    while (match({TokenType::PipePipe})) {
        Token op = previous();
        ExprPtr right = logicalAnd();
        expr = std::make_unique<Expr>(LogicalExpr{std::move(expr), std::move(op), std::move(right)});
    }

    return expr;
}

ExprPtr Parser::logicalAnd() {
    ExprPtr expr = equality();

    while (match({TokenType::AmpAmp})) {
        Token op = previous();
        ExprPtr right = equality();
        expr = std::make_unique<Expr>(LogicalExpr{std::move(expr), std::move(op), std::move(right)});
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
    ExprPtr expr = bitwiseOr();

    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        Token op = previous();
        ExprPtr right = bitwiseOr();
        expr = std::make_unique<Expr>(BinaryExpr{std::move(expr), std::move(op), std::move(right)});
    }

    return expr;
}

ExprPtr Parser::bitwiseOr() {
    ExprPtr expr = bitwiseAnd();

    while (match({TokenType::Pipe})) {
        Token op = previous();
        ExprPtr right = bitwiseAnd();
        expr = std::make_unique<Expr>(BinaryExpr{std::move(expr), std::move(op), std::move(right)});
    }

    return expr;
}

ExprPtr Parser::bitwiseAnd() {
    ExprPtr expr = term();

    while (match({TokenType::Ampersand})) {
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
    ExprPtr expr = power();

    while (match({TokenType::Star, TokenType::Slash, TokenType::Percent})) {
        Token op = previous();
        ExprPtr right = power();
        expr = std::make_unique<Expr>(BinaryExpr{std::move(expr), std::move(op), std::move(right)});
    }

    return expr;
}

ExprPtr Parser::power() {
    ExprPtr expr = unary();

    // Right-associative: 2^3^2 = 2^(3^2)
    if (match({TokenType::Caret})) {
        Token op = previous();
        ExprPtr right = power();
        expr = std::make_unique<Expr>(BinaryExpr{std::move(expr), std::move(op), std::move(right)});
    }

    return expr;
}

ExprPtr Parser::unary() {
    if (match({TokenType::Bang, TokenType::Minus, TokenType::Tilde})) {
        Token op = previous();
        ExprPtr right = unary();
        return std::make_unique<Expr>(UnaryExpr{std::move(op), std::move(right)});
    }

    return primary();
}

ExprPtr Parser::primary() {
    if (match({TokenType::Number})) {
        const Token& tok = previous();
        if (tok.integer.has_value()) {
            return std::make_unique<Expr>(LiteralExpr{Value{*tok.integer}});
        }
        return std::make_unique<Expr>(LiteralExpr{Value{*tok.number}});
    }

    if (match({TokenType::Character})) {
        const Token& tok = previous();
        return std::make_unique<Expr>(LiteralExpr{Value{*tok.character}});
    }

    if (match({TokenType::Nil})) {
        return std::make_unique<Expr>(LiteralExpr{Value{std::monostate{}}});
    }

    if (match({TokenType::True})) {
        return std::make_unique<Expr>(LiteralExpr{Value{true}});
    }

    if (match({TokenType::False})) {
        return std::make_unique<Expr>(LiteralExpr{Value{false}});
    }

    if (match({TokenType::Identifier})) {
        return std::make_unique<Expr>(VariableExpr{previous()});
    }

    if (match({TokenType::LeftParen})) {
        ExprPtr expr = expression();
        consume(TokenType::RightParen, "Expected ')' after expression.");
        return std::make_unique<Expr>(GroupingExpr{std::move(expr)});
    }

    error(peek(), "Expected a literal, identifier, or parenthesized expression.");
}

std::optional<DeclType> Parser::matchDeclarationType() {
    if (match({TokenType::Let})) {
        return DeclType::Auto;
    }
    if (match({TokenType::Int})) {
        return DeclType::Int;
    }
    if (match({TokenType::Long})) {
        // Support "long long" as two consecutive keywords
        match({TokenType::Long});
        return DeclType::Long;
    }
    if (match({TokenType::Double})) {
        return DeclType::Double;
    }
    if (match({TokenType::Float})) {
        return DeclType::Float;
    }
    if (match({TokenType::Bool})) {
        return DeclType::Bool;
    }
    if (match({TokenType::Char})) {
        return DeclType::Char;
    }

    return std::nullopt;
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
