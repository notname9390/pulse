#pragma once

#include "parser/ast.hpp"
#include "lexer/token.hpp"
#include <vector>
#include <memory>

namespace pulse::parser {

class Parser {
public:
    explicit Parser(const std::vector<lexer::Token>& tokens);
    
    // Parse the entire program
    std::unique_ptr<Program> parse();
    
private:
    std::vector<lexer::Token> tokens;
    size_t current;
    
    // Helper methods
    bool isAtEnd() const { return current >= tokens.size(); }
    lexer::Token peek() const { return isAtEnd() ? tokens.back() : tokens[current]; }
    lexer::Token advance() { return isAtEnd() ? tokens.back() : tokens[current++]; }
    bool check(lexer::TokenType type) const;
    bool match(lexer::TokenType type);
    void consume(lexer::TokenType type, const std::string& message);
    
    // Parsing methods
    std::unique_ptr<Program> program();
    std::unique_ptr<Declaration> declaration();
    std::unique_ptr<Statement> statement();
    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> logicalOr();
    std::unique_ptr<Expression> logicalAnd();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> power();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> primary();
    std::unique_ptr<Expression> call();
    std::unique_ptr<Expression> finishCall(std::unique_ptr<Expression> callee);
    
    // Statement parsing
    std::unique_ptr<Statement> ifStatement();
    std::unique_ptr<Statement> whileStatement();
    std::unique_ptr<Statement> forStatement();
    std::unique_ptr<Statement> matchStatement();
    std::unique_ptr<Statement> returnStatement();
    std::unique_ptr<Statement> assignmentStatement();
    std::unique_ptr<Statement> expressionStatement();
    
    // Declaration parsing
    std::unique_ptr<Declaration> functionDeclaration();
    std::unique_ptr<Declaration> classDeclaration();
    std::unique_ptr<Declaration> importDeclaration();
    
    // Expression parsing
    std::unique_ptr<Expression> listExpression();
    std::unique_ptr<Expression> dictExpression();
    std::unique_ptr<Expression> tupleExpression();
    
    // Block parsing
    std::vector<std::unique_ptr<Statement>> block();
    
    // Error handling
    void synchronize();
    void error(const lexer::Token& token, const std::string& message);
};

} // namespace pulse::parser 