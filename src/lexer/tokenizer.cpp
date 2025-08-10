#include "lexer/token.hpp"
#include <unordered_map>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace pulse::lexer {

class Tokenizer {
private:
    std::string source;
    size_t current;
    size_t start;
    size_t line;
    size_t column;
    std::vector<size_t> indent_stack;
    
    // Keyword mapping
    static const std::unordered_map<std::string, TokenType> keywords;
    
    // Helper methods
    bool isAtEnd() const { return current >= source.length(); }
    char advance() { 
        if (isAtEnd()) return '\0';
        char c = source[current++];
        if (c == '\n') {
            line++;
            column = 0;
        } else {
            column++;
        }
        return c;
    }
    
    char peek() const { return isAtEnd() ? '\0' : source[current]; }
    char peekNext() const { return current + 1 >= source.length() ? '\0' : source[current + 1]; }
    
    bool match(char expected) {
        if (isAtEnd()) return false;
        if (source[current] != expected) return false;
        current++;
        column++;
        return true;
    }
    
    void skipWhitespace() {
        while (!isAtEnd() && std::isspace(peek()) && peek() != '\n') {
            advance();
        }
    }
    
    void skipComment() {
        while (!isAtEnd() && peek() != '\n') {
            advance();
        }
    }
    
    Token makeToken(TokenType type) {
        return Token(type, source.substr(start, current - start), line, column - (current - start));
    }
    
    Token makeToken(TokenType type, const std::string& value) {
        return Token(type, source.substr(start, current - start), value, line, column - (current - start));
    }
    
    Token makeToken(TokenType type, int64_t value) {
        return Token(type, source.substr(start, current - start), value, line, column - (current - start));
    }
    
    Token makeToken(TokenType type, double value) {
        return Token(type, source.substr(start, current - start), value, line, column - (current - start));
    }
    
    Token makeToken(TokenType type, bool value) {
        return Token(type, source.substr(start, current - start), value, line, column - (current - start));
    }
    
    // Token parsing methods
    Token string() {
        char quote = peek();
        advance(); // consume opening quote
        
        while (!isAtEnd() && peek() != quote) {
            if (peek() == '\\') {
                advance(); // consume backslash
                if (!isAtEnd()) advance(); // consume escaped character
            } else {
                advance();
            }
        }
        
        if (isAtEnd()) {
            throw std::runtime_error("Unterminated string at line " + std::to_string(line));
        }
        
        advance(); // consume closing quote
        
        std::string value = source.substr(start + 1, current - start - 2);
        return makeToken(TokenType::STRING, value);
    }
    
    Token number() {
        while (!isAtEnd() && std::isdigit(peek())) {
            advance();
        }
        
        // Look for decimal part
        if (!isAtEnd() && peek() == '.' && std::isdigit(peekNext())) {
            advance(); // consume '.'
            
            while (!isAtEnd() && std::isdigit(peek())) {
                advance();
            }
            
            double value = std::stod(source.substr(start, current - start));
            return makeToken(TokenType::FLOAT, value);
        }
        
        int64_t value = std::stoll(source.substr(start, current - start));
        return makeToken(TokenType::INTEGER, value);
    }
    
    Token identifier() {
        while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
            advance();
        }
        
        std::string text = source.substr(start, current - start);
        
        // Check if it's a keyword
        auto it = keywords.find(text);
        if (it != keywords.end()) {
            return makeToken(it->second);
        }
        
        // Check for boolean literals
        if (text == "True") return makeToken(TokenType::BOOLEAN, true);
        if (text == "False") return makeToken(TokenType::BOOLEAN, false);
        if (text == "None") return makeToken(TokenType::NONE);
        
        return makeToken(TokenType::IDENTIFIER);
    }
    
    Token handleIndentation() {
        size_t indent_level = 0;
        size_t temp_current = current;
        
        while (temp_current < source.length() && source[temp_current] == ' ') {
            indent_level++;
            temp_current++;
        }
        
        if (indent_level % 4 != 0) {
            throw std::runtime_error("Invalid indentation at line " + std::to_string(line));
        }
        
        size_t spaces = indent_level / 4;
        
        if (indent_stack.empty() || spaces > indent_stack.back()) {
            indent_stack.push_back(spaces);
            return Token(TokenType::INDENT, "", line, column);
        } else if (spaces < indent_stack.back()) {
            std::vector<Token> dedent_tokens;
            while (!indent_stack.empty() && indent_stack.back() > spaces) {
                indent_stack.pop_back();
                dedent_tokens.push_back(Token(TokenType::DEDENT, "", line, column));
            }
            
            if (indent_stack.empty() || indent_stack.back() != spaces) {
                throw std::runtime_error("Invalid indentation at line " + std::to_string(line));
            }
            
            // Return the first dedent token, others will be handled in subsequent calls
            return dedent_tokens[0];
        }
        
        return Token(TokenType::NEWLINE, "", line, column);
    }

public:
    Tokenizer(const std::string& source) 
        : source(source), current(0), start(0), line(1), column(0) {
        indent_stack.push_back(0); // Base indentation level
    }
    
    Token nextToken() {
        skipWhitespace();
        
        if (isAtEnd()) {
            return Token(TokenType::EOF_TOKEN, "", line, column);
        }
        
        start = current;
        
        char c = advance();
        
        // Handle newlines and indentation
        if (c == '\n') {
            return handleIndentation();
        }
        
        // Handle comments
        if (c == '#') {
            skipComment();
            return makeToken(TokenType::COMMENT);
        }
        
        // Handle string literals
        if (c == '"' || c == '\'') {
            current = start; // Reset to start of string
            column = column - 1; // Adjust column
            return string();
        }
        
        // Handle numbers
        if (std::isdigit(c)) {
            current = start; // Reset to start of number
            column = column - 1; // Adjust column
            return number();
        }
        
        // Handle identifiers and keywords
        if (std::isalpha(c) || c == '_') {
            current = start; // Reset to start of identifier
            column = column - 1; // Adjust column
            return identifier();
        }
        
        // Handle operators and delimiters
        switch (c) {
            case '(': return makeToken(TokenType::LPAREN);
            case ')': return makeToken(TokenType::RPAREN);
            case '[': return makeToken(TokenType::LBRACKET);
            case ']': return makeToken(TokenType::RBRACKET);
            case '{': return makeToken(TokenType::LBRACE);
            case '}': return makeToken(TokenType::RBRACE);
            case ',': return makeToken(TokenType::COMMA);
            case '.': return makeToken(TokenType::DOT);
            case ':': return makeToken(TokenType::COLON);
            case '=': 
                if (match('=')) return makeToken(TokenType::EQUAL);
                return makeToken(TokenType::ASSIGN);
            case '!':
                if (match('=')) return makeToken(TokenType::NOT_EQUAL);
                break;
            case '<':
                if (match('=')) return makeToken(TokenType::LESS_EQUAL);
                return makeToken(TokenType::LESS);
            case '>':
                if (match('=')) return makeToken(TokenType::GREATER_EQUAL);
                return makeToken(TokenType::GREATER);
            case '+': return makeToken(TokenType::PLUS);
            case '-': return makeToken(TokenType::MINUS);
            case '*': 
                if (match('*')) return makeToken(TokenType::POWER);
                return makeToken(TokenType::MULTIPLY);
            case '/': 
                if (match('/')) return makeToken(TokenType::FLOOR_DIVIDE);
                return makeToken(TokenType::DIVIDE);
            case '%': return makeToken(TokenType::MODULO);
        }
        
        throw std::runtime_error("Unexpected character '" + std::string(1, c) + "' at line " + std::to_string(line));
    }
    
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        Token token;
        
        do {
            token = nextToken();
            tokens.push_back(token);
        } while (token.type != TokenType::EOF_TOKEN);
        
        return tokens;
    }
};

// Initialize static keyword map
const std::unordered_map<std::string, TokenType> Tokenizer::keywords = {
    {"if", TokenType::IF},
    {"elif", TokenType::ELIF},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"in", TokenType::IN},
    {"def", TokenType::DEF},
    {"class", TokenType::CLASS},
    {"return", TokenType::RETURN},
    {"import", TokenType::IMPORT},
    {"as", TokenType::AS},
    {"match", TokenType::MATCH},
    {"async", TokenType::ASYNC},
    {"await", TokenType::AWAIT},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT}
};

} // namespace pulse::lexer 