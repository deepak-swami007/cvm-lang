#include "cvm/lexer.h"

#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace cvm {

namespace {

const std::unordered_map<std::string_view, TokenType> kKeywords = {
    {"auto", TokenType::Let},
    {"print", TokenType::Print},
    {"input", TokenType::Input},
    {"let", TokenType::Let},
    {"nil", TokenType::Nil},
    {"true", TokenType::True},
    {"false", TokenType::False},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"while", TokenType::While},
    {"for", TokenType::For},
    {"break", TokenType::Break},
    {"continue", TokenType::Continue},
    {"int", TokenType::Int},
    {"integer", TokenType::Int},
    {"long", TokenType::Long},
    {"double", TokenType::Double},
    {"float", TokenType::Float},
    {"bool", TokenType::Bool},
    {"char", TokenType::Char},
};

char decodeEscape(char escaped, std::size_t line, std::string_view context) {
    switch (escaped) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '0': return '\0';
        case '\\': return '\\';
        case '\'': return '\'';
        default:
            throw std::runtime_error(
                "Invalid escape sequence '\\" + std::string(1, escaped) + "' in " + std::string(context) +
                " on line " + std::to_string(line) + ".");
    }
}

}  // namespace

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start_ = current_;
        scanToken();
    }

    tokens_.push_back(Token{TokenType::EndOfFile, "", line_});
    return tokens_;
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.size();
}

char Lexer::advance() {
    return source_[current_++];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) {
        return false;
    }
    ++current_;
    return true;
}

char Lexer::peek() const {
    if (isAtEnd()) {
        return '\0';
    }
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.size()) {
        return '\0';
    }
    return source_[current_ + 1];
}

namespace {

bool startsExponent(char current, char next) {
    if (current != 'e' && current != 'E') {
        return false;
    }
    if (Lexer::isDigit(next)) {
        return true;
    }
    return (next == '+' || next == '-') && false;
}

}  // namespace

void Lexer::addToken(TokenType type) {
    tokens_.push_back(Token{type, source_.substr(start_, current_ - start_), line_});
}

void Lexer::addIntegerToken(std::int64_t value) {
    tokens_.push_back(Token{TokenType::Number, source_.substr(start_, current_ - start_), line_, value});
}

void Lexer::addNumberToken(double value) {
    tokens_.push_back(Token{TokenType::Number, source_.substr(start_, current_ - start_), line_, std::nullopt, value});
}

void Lexer::scanToken() {
    const char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LeftParen); break;
        case ')': addToken(TokenType::RightParen); break;
        case '{': addToken(TokenType::LeftBrace); break;
        case '}': addToken(TokenType::RightBrace); break;
        case ',': addToken(TokenType::Comma); break;
        case '.': addToken(TokenType::Dot); break;
        case ';': addToken(TokenType::Semicolon); break;
        case '+': addToken(match('=') ? TokenType::PlusEqual : TokenType::Plus); break;
        case '-': addToken(match('=') ? TokenType::MinusEqual : TokenType::Minus); break;
        case '*': addToken(match('=') ? TokenType::StarEqual : TokenType::Star); break;
        case '%': addToken(TokenType::Percent); break;
        case '^': addToken(TokenType::Caret); break;
        case '&': addToken(match('&') ? TokenType::AmpAmp : TokenType::Ampersand); break;
        case '|': addToken(match('|') ? TokenType::PipePipe : TokenType::Pipe); break;
        case '~': addToken(TokenType::Tilde); break;
        case '!': addToken(match('=') ? TokenType::BangEqual : TokenType::Bang); break;
        case '=': addToken(match('=') ? TokenType::EqualEqual : TokenType::Equal); break;
        case '<': addToken(match('=') ? TokenType::LessEqual : TokenType::Less); break;
        case '>': addToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
        case '/':
            if (match('/')) {
                // Single-line comment
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
            } else if (match('=')) {
                addToken(TokenType::SlashEqual);
            } else {
                addToken(TokenType::Slash);
            }
            break;
        case '\'':
            scanCharacter();
            break;
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            ++line_;
            break;
        default:
            if (isDigit(c)) {
                number();
            } else if (isAlpha(c)) {
                identifier();
            } else {
                throw std::runtime_error(
                    "Unexpected character '" + std::string(1, c) + "' on line " + std::to_string(line_) + ".");
            }
            break;
    }
}

void Lexer::number() {
    while (isDigit(peek())) {
        advance();
    }

    bool isInteger = true;
    if (peek() == '.' && isDigit(peekNext())) {
        isInteger = false;
        advance();
        while (isDigit(peek())) {
            advance();
        }
    }

    const std::string lexeme = source_.substr(start_, current_ - start_);
    try {
        std::size_t consumed = 0;
        if (isInteger) {
            const std::int64_t value = std::stoll(lexeme, &consumed, 10);
            if (consumed != lexeme.size()) {
                throw std::runtime_error(
                    "Invalid numeric literal '" + lexeme + "' on line " + std::to_string(line_) + ".");
            }
            addIntegerToken(value);
            return;
        }

        const double value = std::stod(lexeme, &consumed);
        if (consumed != lexeme.size()) {
            throw std::runtime_error(
                "Invalid numeric literal '" + lexeme + "' on line " + std::to_string(line_) + ".");
        }
        addNumberToken(value);
    } catch (const std::runtime_error&) {
        throw;
    } catch (const std::invalid_argument&) {
        throw std::runtime_error(
            "Invalid numeric literal '" + lexeme + "' on line " + std::to_string(line_) + ".");
    } catch (const std::exception&) {
        throw std::runtime_error(
            "Numeric literal '" + lexeme + "' is out of range on line " + std::to_string(line_) + ".");
    }
}

void Lexer::scanCharacter() {
    const std::size_t startLine = line_;
    if (isAtEnd() || peek() == '\n') {
        throw std::runtime_error(
            "Unterminated char literal starting on line " + std::to_string(startLine) + ".");
    }
    if (peek() == '\'') {
        throw std::runtime_error("Empty char literal on line " + std::to_string(startLine) + ".");
    }

    char value = '\0';
    if (peek() == '\\') {
        advance();
        if (isAtEnd() || peek() == '\n') {
            throw std::runtime_error(
                "Unterminated char literal starting on line " + std::to_string(startLine) + ".");
        }
        value = decodeEscape(advance(), startLine, "char literal");
    } else {
        value = advance();
    }

    if (peek() != '\'') {
        if (isAtEnd() || peek() == '\n') {
            throw std::runtime_error(
                "Unterminated char literal starting on line " + std::to_string(startLine) + ".");
        }
        throw std::runtime_error(
            "Char literal must contain exactly one character on line " + std::to_string(startLine) + ".");
    }

    advance();
    tokens_.push_back(
        Token{TokenType::Character, source_.substr(start_, current_ - start_), startLine,
              std::nullopt, std::nullopt, value});
}

void Lexer::identifier() {
    while (isAlphaNumeric(peek())) {
        advance();
    }

    const auto lexeme = source_.substr(start_, current_ - start_);
    const auto it = kKeywords.find(lexeme);
    if (it != kKeywords.end()) {
        addToken(it->second);
        return;
    }

    addToken(TokenType::Identifier);
}

bool Lexer::isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool Lexer::isAlpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool Lexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

}  // namespace cvm
