#pragma once

#include <string>
#include <variant>
#include <optional>

namespace pulse::lexer {

enum class TokenType {
    // Literals
    IDENTIFIER,
    STRING,
    INTEGER,
    FLOAT,
    BOOLEAN,
    NONE,
    
    // Operators
    PLUS,           // +
    MINUS,          // -
    MULTIPLY,       // *
    DIVIDE,         // /
    FLOOR_DIVIDE,   // //
    MODULO,         // %
    POWER,          // **
    
    // Comparison operators
    EQUAL,          // ==
    NOT_EQUAL,      // !=
    LESS,           // <
    LESS_EQUAL,     // <=
    GREATER,        // >
    GREATER_EQUAL,  // >=
    
    // Logical operators
    AND,            // and
    OR,             // or
    NOT,            // not
    
    // Assignment
    ASSIGN,         // =
    
    // Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACKET,       // [
    RBRACKET,       // ]
    LBRACE,         // {
    RBRACE,         // }
    COMMA,          // ,
    COLON,          // :
    DOT,            // .
    
    // Keywords
    IF,
    ELIF,
    ELSE,
    WHILE,
    FOR,
    IN,
    DEF,
    CLASS,
    RETURN,
    IMPORT,
    AS,
    MATCH,
    ASYNC,
    AWAIT,
    
    // Special
    INDENT,
    DEDENT,
    NEWLINE,
    EOF_TOKEN,
    
    // Comments
    COMMENT
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::variant<std::string, int64_t, double, bool, std::monostate> value;
    size_t line;
    size_t column;
    
    Token(TokenType type, const std::string& lexeme, size_t line, size_t column)
        : type(type), lexeme(lexeme), value(std::monostate{}), line(line), column(column) {}
    
    Token(TokenType type, const std::string& lexeme, const std::string& value, size_t line, size_t column)
        : type(type), lexeme(lexeme), value(value), line(line), column(column) {}
    
    Token(TokenType type, const std::string& lexeme, int64_t value, size_t line, size_t column)
        : type(type), lexeme(lexeme), value(value), line(line), column(column) {}
    
    Token(TokenType type, const std::string& lexeme, double value, size_t line, size_t column)
        : type(type), lexeme(lexeme), value(value), line(line), column(column) {}
    
    Token(TokenType type, const std::string& lexeme, bool value, size_t line, size_t column)
        : type(type), lexeme(lexeme), value(value), line(line), column(column) {}
    
    // Helper methods to get typed values
    std::optional<std::string> getString() const {
        if (std::holds_alternative<std::string>(value)) {
            return std::get<std::string>(value);
        }
        return std::nullopt;
    }
    
    std::optional<int64_t> getInteger() const {
        if (std::holds_alternative<int64_t>(value)) {
            return std::get<int64_t>(value);
        }
        return std::nullopt;
    }
    
    std::optional<double> getFloat() const {
        if (std::holds_alternative<double>(value)) {
            return std::get<double>(value);
        }
        return std::nullopt;
    }
    
    std::optional<bool> getBoolean() const {
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value);
        }
        return std::nullopt;
    }
};

} // namespace pulse::lexer 