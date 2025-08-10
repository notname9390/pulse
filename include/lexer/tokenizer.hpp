#pragma once

#include "lexer/token.hpp"
#include <vector>
#include <string>

namespace pulse::lexer {

class Tokenizer {
public:
    explicit Tokenizer(const std::string& source);
    
    // Get the next token from the source
    Token nextToken();
    
    // Tokenize the entire source into a vector of tokens
    std::vector<Token> tokenize();
    
private:
    std::string source;
    size_t current;
    size_t start;
    size_t line;
    size_t column;
    std::vector<size_t> indent_stack;
    
    // Helper methods
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    
    void skipWhitespace();
    void skipComment();
    
    // Token creation helpers
    Token makeToken(TokenType type);
    Token makeToken(TokenType type, const std::string& value);
    Token makeToken(TokenType type, int64_t value);
    Token makeToken(TokenType type, double value);
    Token makeToken(TokenType type, bool value);
    
    // Token parsing methods
    Token string();
    Token number();
    Token identifier();
    Token handleIndentation();
    
    // Static keyword mapping
    static const std::unordered_map<std::string, TokenType> keywords;
};

} // namespace pulse::lexer 