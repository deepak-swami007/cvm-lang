#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace cvm {

enum class TokenType {
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Comma,
    Dot,
    Semicolon,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,
    Ampersand,
    Pipe,
    Tilde,
    Bang,
    BangEqual,
    Equal,
    EqualEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Identifier,
    Number,
    Print,
    Input,
    Let,
    Nil,
    True,
    False,
    If,
    Else,
    While,
    // Type keywords
    Int,
    Long,
    Double,
    Float,
    EndOfFile,
};

class Token {
  public:
    TokenType type;
    std::string lexeme;
    std::size_t line;
    std::optional<double> number;
    bool isIntegerLiteral = false;
};

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
        case TokenType::Bang: return "Bang";
        case TokenType::BangEqual: return "BangEqual";
        case TokenType::Equal: return "Equal";
        case TokenType::EqualEqual: return "EqualEqual";
        case TokenType::Greater: return "Greater";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::Less: return "Less";
        case TokenType::LessEqual: return "LessEqual";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Number: return "Number";
        case TokenType::Print: return "Print";
        case TokenType::Input: return "Input";
        case TokenType::Let: return "Let";
        case TokenType::Nil: return "Nil";
        case TokenType::True: return "True";
        case TokenType::False: return "False";
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::While: return "While";
        case TokenType::Int: return "Int";
        case TokenType::Long: return "Long";
        case TokenType::Double: return "Double";
        case TokenType::Float: return "Float";
        case TokenType::EndOfFile: return "EOF";
    }

    return "Unknown";
}

inline std::string formatToken(const Token& token) {
    std::string result = std::string(tokenTypeName(token.type)) + " \"" + token.lexeme + "\"";
    if (token.number.has_value()) {
        result += " value=" + std::to_string(*token.number);
    }
    result += " line=" + std::to_string(token.line);
    return result;
}

}  // namespace cvm
