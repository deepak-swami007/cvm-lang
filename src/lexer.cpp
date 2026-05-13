#include "cvm/lexer.h"

#include <cctype>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace cvm {

namespace {

const std::unordered_map<std::string_view, TokenType> kKeywords = {
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
    {"long", TokenType::Long},
    {"double", TokenType::Double},
    {"float", TokenType::Float},
};

}  // namespace

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start_ = current_;
        scanToken();
    }

    tokens_.push_back(Token{TokenType::EndOfFile, "", line_, std::nullopt, false, ""});
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

void Lexer::addToken(TokenType type) {
    tokens_.push_back(Token{type, source_.substr(start_, current_ - start_), line_, std::nullopt, false, ""});
}

void Lexer::addToken(TokenType type, double numberValue, bool isInteger) {
    tokens_.push_back(Token{type, source_.substr(start_, current_ - start_), line_, numberValue, isInteger, ""});
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
        case '"':
            scanString();
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

    const auto lexeme = source_.substr(start_, current_ - start_);
    try {
        addToken(TokenType::Number, std::stod(lexeme), isInteger);
    } catch (const std::invalid_argument&) {
        throw std::runtime_error(
            "Invalid numeric literal '" + lexeme + "' on line " + std::to_string(line_) + ".");
    } catch (const std::out_of_range&) {
        throw std::runtime_error(
            "Numeric literal '" + lexeme + "' is out of range on line " + std::to_string(line_) + ".");
    }
}

void Lexer::scanString() {
    const std::size_t startLine = line_;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') ++line_;
        advance();
    }

    if (isAtEnd()) {
        throw std::runtime_error(
            "Unterminated string starting on line " + std::to_string(startLine) + ".");
    }

    advance();  // closing "

    // Extract raw content (without surrounding quotes)
    const std::string raw = source_.substr(start_ + 1, current_ - start_ - 2);

    // Process escape sequences
    std::string processed;
    processed.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            ++i;
            switch (raw[i]) {
                case 'n':  processed += '\n'; break;
                case 't':  processed += '\t'; break;
                case '\\': processed += '\\'; break;
                case '"':  processed += '"';  break;
                default:   processed += '\\'; processed += raw[i]; break;
            }
        } else {
            processed += raw[i];
        }
    }

    const std::string lexeme = source_.substr(start_, current_ - start_);
    tokens_.push_back(Token{TokenType::String, lexeme, startLine, std::nullopt, false, processed});
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
