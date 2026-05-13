#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace cvm {

enum class TokenType {
    // Punctuation
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Comma,
    Dot,
    Semicolon,

    // Arithmetic operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,
    Ampersand,
    Pipe,
    Tilde,

    // Logical operators
    AmpAmp,
    PipePipe,

    // Comparison / equality
    Bang,
    BangEqual,
    Equal,
    EqualEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,

    // Compound assignment
    PlusEqual,
    MinusEqual,
    StarEqual,
    SlashEqual,

    // Literals
    Identifier,
    Number,
    Character,

    // Keywords
    Print,
    Input,
    Let,
    Nil,
    True,
    False,
    If,
    Else,
    While,
    For,
    Break,
    Continue,

    // Type keywords
    Int,
    Long,
    Double,
    Float,
    Bool,
    Char,

    EndOfFile,
};

class Token {
  public:
    Token() = default;
    Token(TokenType type,
          std::string lexeme,
          std::size_t line,
          std::optional<std::int64_t> integer = std::nullopt,
          std::optional<double> number = std::nullopt,
          std::optional<char> character = std::nullopt)
        : type(type),
          lexeme(std::move(lexeme)),
          line(line),
          integer(integer),
          number(number),
          character(character) {}

    TokenType type;
    std::string lexeme;
    std::size_t line;
    std::optional<std::int64_t> integer;
    std::optional<double> number;
    std::optional<char> character;
};

inline std::string escapeForDisplay(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size());

    for (char c : text) {
        switch (c) {
            case '\n': escaped += "\\n"; break;
            case '\t': escaped += "\\t"; break;
            case '\r': escaped += "\\r"; break;
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\'': escaped += "\\'"; break;
            case '\0': escaped += "\\0"; break;
            default: escaped += c; break;
        }
    }

    return escaped;
}

inline std::string_view tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::LeftParen: return "LeftParen";
        case TokenType::RightParen: return "RightParen";
        case TokenType::LeftBrace: return "LeftBrace";
        case TokenType::RightBrace: return "RightBrace";
        case TokenType::Comma: return "Comma";
        case TokenType::Dot: return "Dot";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Percent: return "Percent";
        case TokenType::Caret: return "Caret";
        case TokenType::Ampersand: return "Ampersand";
        case TokenType::Pipe: return "Pipe";
        case TokenType::Tilde: return "Tilde";
        case TokenType::AmpAmp: return "AmpAmp";
        case TokenType::PipePipe: return "PipePipe";
        case TokenType::Bang: return "Bang";
        case TokenType::BangEqual: return "BangEqual";
        case TokenType::Equal: return "Equal";
        case TokenType::EqualEqual: return "EqualEqual";
        case TokenType::Greater: return "Greater";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::Less: return "Less";
        case TokenType::LessEqual: return "LessEqual";
        case TokenType::PlusEqual: return "PlusEqual";
        case TokenType::MinusEqual: return "MinusEqual";
        case TokenType::StarEqual: return "StarEqual";
        case TokenType::SlashEqual: return "SlashEqual";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Number: return "Number";
        case TokenType::Character: return "Character";
        case TokenType::Print: return "Print";
        case TokenType::Input: return "Input";
        case TokenType::Let: return "Let";
        case TokenType::Nil: return "Nil";
        case TokenType::True: return "True";
        case TokenType::False: return "False";
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::While: return "While";
        case TokenType::For: return "For";
        case TokenType::Break: return "Break";
        case TokenType::Continue: return "Continue";
        case TokenType::Int: return "Int";
        case TokenType::Long: return "Long";
        case TokenType::Double: return "Double";
        case TokenType::Float: return "Float";
        case TokenType::Bool: return "Bool";
        case TokenType::Char: return "Char";
        case TokenType::EndOfFile: return "EOF";
    }

    return "Unknown";
}

inline std::string formatToken(const Token& token) {
    std::string result = std::string(tokenTypeName(token.type)) + " \"" + token.lexeme + "\"";
    if (token.integer.has_value()) {
        result += " value=" + std::to_string(*token.integer);
    }
    if (token.number.has_value()) {
        result += " value=" + std::to_string(*token.number);
    }
    if (token.character.has_value()) {
        result += " char='" + escapeForDisplay(std::string(1, *token.character)) + "'";
    }
    result += " line=" + std::to_string(token.line);
    return result;
}

}  // namespace cvm
