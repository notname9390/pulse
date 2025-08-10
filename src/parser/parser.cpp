#include "parser/parser.hpp"
#include <stdexcept>
#include <iostream>

namespace pulse::parser {

Parser::Parser(const std::vector<lexer::Token>& tokens) 
    : tokens(tokens), current(0) {}

std::unique_ptr<Program> Parser::parse() {
    try {
        return program();
    } catch (const std::runtime_error& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Program> Parser::program() {
    std::vector<std::unique_ptr<Declaration>> declarations;
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!isAtEnd()) {
        if (peek().type == lexer::TokenType::INDENT) {
            advance(); // consume indent
            continue;
        }
        
        if (peek().type == lexer::TokenType::DEDENT) {
            advance(); // consume dedent
            continue;
        }
        
        if (peek().type == lexer::TokenType::NEWLINE) {
            advance(); // consume newline
            continue;
        }
        
        if (auto decl = declaration()) {
            declarations.push_back(std::move(decl));
        } else if (auto stmt = statement()) {
            statements.push_back(std::move(stmt));
        } else {
            break;
        }
    }
    
    return std::make_unique<Program>(std::move(declarations), std::move(statements));
}

std::unique_ptr<Declaration> Parser::declaration() {
    if (match(lexer::TokenType::IMPORT)) {
        return importDeclaration();
    }
    
    if (match(lexer::TokenType::DEF)) {
        return functionDeclaration();
    }
    
    if (match(lexer::TokenType::CLASS)) {
        return classDeclaration();
    }
    
    return nullptr;
}

std::unique_ptr<Statement> Parser::statement() {
    if (match(lexer::TokenType::IF)) {
        return ifStatement();
    }
    
    if (match(lexer::TokenType::WHILE)) {
        return whileStatement();
    }
    
    if (match(lexer::TokenType::FOR)) {
        return forStatement();
    }
    
    if (match(lexer::TokenType::MATCH)) {
        return matchStatement();
    }
    
    if (match(lexer::TokenType::RETURN)) {
        return returnStatement();
    }
    
    // Check for assignment (identifier = expression)
    if (peek().type == lexer::TokenType::IDENTIFIER && 
        peek().type != lexer::TokenType::EOF_TOKEN) {
        auto temp_current = current;
        advance(); // consume identifier
        
        if (peek().type == lexer::TokenType::ASSIGN) {
            current = temp_current; // reset
            return assignmentStatement();
        }
        
        current = temp_current; // reset
    }
    
    return expressionStatement();
}

std::unique_ptr<Expression> Parser::expression() {
    return logicalOr();
}

std::unique_ptr<Expression> Parser::logicalOr() {
    auto expr = logicalAnd();
    
    while (match(lexer::TokenType::OR)) {
        auto operator_token = tokens[current - 1];
        auto right = logicalAnd();
        expr = std::make_unique<BinaryExpression>(
            BinaryExpression::Operator::OR, std::move(expr), std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::logicalAnd() {
    auto expr = equality();
    
    while (match(lexer::TokenType::AND)) {
        auto operator_token = tokens[current - 1];
        auto right = equality();
        expr = std::make_unique<BinaryExpression>(
            BinaryExpression::Operator::AND, std::move(expr), std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::equality() {
    auto expr = comparison();
    
    while (match(lexer::TokenType::EQUAL) || match(lexer::TokenType::NOT_EQUAL)) {
        auto operator_token = tokens[current - 1];
        auto right = comparison();
        
        BinaryExpression::Operator op = (operator_token.type == lexer::TokenType::EQUAL) 
            ? BinaryExpression::Operator::EQUAL 
            : BinaryExpression::Operator::NOT_EQUAL;
            
        expr = std::make_unique<BinaryExpression>(op, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::comparison() {
    auto expr = term();
    
    while (match(lexer::TokenType::LESS) || match(lexer::TokenType::LESS_EQUAL) ||
           match(lexer::TokenType::GREATER) || match(lexer::TokenType::GREATER_EQUAL)) {
        auto operator_token = tokens[current - 1];
        auto right = term();
        
        BinaryExpression::Operator op;
        switch (operator_token.type) {
            case lexer::TokenType::LESS: op = BinaryExpression::Operator::LESS; break;
            case lexer::TokenType::LESS_EQUAL: op = BinaryExpression::Operator::LESS_EQUAL; break;
            case lexer::TokenType::GREATER: op = BinaryExpression::Operator::GREATER; break;
            case lexer::TokenType::GREATER_EQUAL: op = BinaryExpression::Operator::GREATER_EQUAL; break;
            default: op = BinaryExpression::Operator::LESS; break;
        }
        
        expr = std::make_unique<BinaryExpression>(op, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::term() {
    auto expr = factor();
    
    while (match(lexer::TokenType::PLUS) || match(lexer::TokenType::MINUS)) {
        auto operator_token = tokens[current - 1];
        auto right = factor();
        
        BinaryExpression::Operator op = (operator_token.type == lexer::TokenType::PLUS) 
            ? BinaryExpression::Operator::ADD 
            : BinaryExpression::Operator::SUBTRACT;
            
        expr = std::make_unique<BinaryExpression>(op, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::factor() {
    auto expr = power();
    
    while (match(lexer::TokenType::MULTIPLY) || match(lexer::TokenType::DIVIDE) ||
           match(lexer::TokenType::FLOOR_DIVIDE) || match(lexer::TokenType::MODULO)) {
        auto operator_token = tokens[current - 1];
        auto right = power();
        
        BinaryExpression::Operator op;
        switch (operator_token.type) {
            case lexer::TokenType::MULTIPLY: op = BinaryExpression::Operator::MULTIPLY; break;
            case lexer::TokenType::DIVIDE: op = BinaryExpression::Operator::DIVIDE; break;
            case lexer::TokenType::FLOOR_DIVIDE: op = BinaryExpression::Operator::FLOOR_DIVIDE; break;
            case lexer::TokenType::MODULO: op = BinaryExpression::Operator::MODULO; break;
            default: op = BinaryExpression::Operator::MULTIPLY; break;
        }
        
        expr = std::make_unique<BinaryExpression>(op, std::move(expr), std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::power() {
    auto expr = unary();
    
    while (match(lexer::TokenType::POWER)) {
        auto right = unary();
        expr = std::make_unique<BinaryExpression>(
            BinaryExpression::Operator::POWER, std::move(expr), std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::unary() {
    if (match(lexer::TokenType::MINUS) || match(lexer::TokenType::NOT)) {
        auto operator_token = tokens[current - 1];
        auto operand = unary();
        
        UnaryExpression::Operator op = (operator_token.type == lexer::TokenType::MINUS) 
            ? UnaryExpression::Operator::MINUS 
            : UnaryExpression::Operator::NOT;
            
        return std::make_unique<UnaryExpression>(op, std::move(operand));
    }
    
    return primary();
}

std::unique_ptr<Expression> Parser::primary() {
    if (match(lexer::TokenType::FALSE)) {
        return std::make_unique<LiteralExpression>(false);
    }
    
    if (match(lexer::TokenType::TRUE)) {
        return std::make_unique<LiteralExpression>(true);
    }
    
    if (match(lexer::TokenType::NONE)) {
        return std::make_unique<LiteralExpression>();
    }
    
    if (match(lexer::TokenType::INTEGER)) {
        auto token = tokens[current - 1];
        if (auto value = token.getInteger()) {
            return std::make_unique<LiteralExpression>(*value);
        }
    }
    
    if (match(lexer::TokenType::FLOAT)) {
        auto token = tokens[current - 1];
        if (auto value = token.getFloat()) {
            return std::make_unique<LiteralExpression>(*value);
        }
    }
    
    if (match(lexer::TokenType::STRING)) {
        auto token = tokens[current - 1];
        if (auto value = token.getString()) {
            return std::make_unique<LiteralExpression>(*value);
        }
    }
    
    if (match(lexer::TokenType::IDENTIFIER)) {
        auto token = tokens[current - 1];
        return std::make_unique<IdentifierExpression>(token.lexeme);
    }
    
    if (match(lexer::TokenType::LPAREN)) {
        auto expr = expression();
        consume(lexer::TokenType::RPAREN, "Expect ')' after expression.");
        return expr;
    }
    
    if (match(lexer::TokenType::LBRACKET)) {
        return listExpression();
    }
    
    if (match(lexer::TokenType::LBRACE)) {
        return dictExpression();
    }
    
    error(peek(), "Expect expression.");
    return nullptr;
}

std::unique_ptr<Expression> Parser::call() {
    auto expr = primary();
    
    while (true) {
        if (match(lexer::TokenType::LPAREN)) {
            expr = finishCall(std::move(expr));
        } else if (match(lexer::TokenType::DOT)) {
            consume(lexer::TokenType::IDENTIFIER, "Expect property name after '.'.");
            auto name = tokens[current - 1].lexeme;
            expr = std::make_unique<AttributeExpression>(std::move(expr), name);
        } else if (match(lexer::TokenType::LBRACKET)) {
            auto index = expression();
            consume(lexer::TokenType::RBRACKET, "Expect ']' after index.");
            expr = std::make_unique<SubscriptExpression>(std::move(expr), std::move(index));
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::finishCall(std::unique_ptr<Expression> callee) {
    std::vector<std::unique_ptr<Expression>> arguments;
    
    if (!check(lexer::TokenType::RPAREN)) {
        do {
            arguments.push_back(expression());
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RPAREN, "Expect ')' after arguments.");
    
    return std::make_unique<CallExpression>(std::move(callee), std::move(arguments));
}

// Statement parsing implementations
std::unique_ptr<Statement> Parser::ifStatement() {
    consume(lexer::TokenType::COLON, "Expect ':' after if condition.");
    
    auto condition = expression();
    auto then_branch = block();
    
    std::vector<IfStatement::Branch> branches;
    branches.emplace_back(std::move(condition), std::move(then_branch));
    
    while (match(lexer::TokenType::ELIF)) {
        consume(lexer::TokenType::COLON, "Expect ':' after elif condition.");
        auto elif_condition = expression();
        auto elif_branch = block();
        branches.emplace_back(std::move(elif_condition), std::move(elif_branch));
    }
    
    std::vector<std::unique_ptr<Statement>> else_branch;
    if (match(lexer::TokenType::ELSE)) {
        consume(lexer::TokenType::COLON, "Expect ':' after else.");
        else_branch = block();
    }
    
    return std::make_unique<IfStatement>(std::move(branches), std::move(else_branch));
}

std::unique_ptr<Statement> Parser::whileStatement() {
    auto condition = expression();
    consume(lexer::TokenType::COLON, "Expect ':' after while condition.");
    auto body = block();
    
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::forStatement() {
    consume(lexer::TokenType::IDENTIFIER, "Expect variable name after 'for'.");
    auto variable = tokens[current - 1].lexeme;
    
    consume(lexer::TokenType::IN, "Expect 'in' after variable name.");
    auto iterable = expression();
    
    consume(lexer::TokenType::COLON, "Expect ':' after iterable.");
    auto body = block();
    
    return std::make_unique<ForStatement>(variable, std::move(iterable), std::move(body));
}

std::unique_ptr<Statement> Parser::matchStatement() {
    auto value = expression();
    consume(lexer::TokenType::COLON, "Expect ':' after match value.");
    
    std::vector<std::pair<ExpressionPtr, std::vector<StatementPtr>>> cases;
    
    while (!isAtEnd() && !check(lexer::TokenType::DEDENT)) {
        auto pattern = expression();
        consume(lexer::TokenType::COLON, "Expect ':' after pattern.");
        auto case_body = block();
        cases.emplace_back(std::move(pattern), std::move(case_body));
    }
    
    return std::make_unique<MatchStatement>(std::move(value), std::move(cases));
}

std::unique_ptr<Statement> Parser::returnStatement() {
    std::unique_ptr<Expression> value = nullptr;
    
    if (!check(lexer::TokenType::NEWLINE) && !check(lexer::TokenType::DEDENT)) {
        value = expression();
    }
    
    return std::make_unique<ReturnStatement>(std::move(value));
}

std::unique_ptr<Statement> Parser::assignmentStatement() {
    consume(lexer::TokenType::IDENTIFIER, "Expect variable name.");
    auto name = tokens[current - 1].lexeme;
    
    consume(lexer::TokenType::ASSIGN, "Expect '=' after variable name.");
    auto value = expression();
    
    return std::make_unique<AssignmentStatement>(name, std::move(value));
}

std::unique_ptr<Statement> Parser::expressionStatement() {
    auto expr = expression();
    return std::make_unique<ExpressionStatement>(std::move(expr));
}

// Declaration parsing implementations
std::unique_ptr<Declaration> Parser::functionDeclaration() {
    consume(lexer::TokenType::IDENTIFIER, "Expect function name.");
    auto name = tokens[current - 1].lexeme;
    
    consume(lexer::TokenType::LPAREN, "Expect '(' after function name.");
    
    std::vector<std::string> parameters;
    if (!check(lexer::TokenType::RPAREN)) {
        do {
            consume(lexer::TokenType::IDENTIFIER, "Expect parameter name.");
            parameters.push_back(tokens[current - 1].lexeme);
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RPAREN, "Expect ')' after parameters.");
    consume(lexer::TokenType::COLON, "Expect ':' after function parameters.");
    
    auto body = block();
    
    return std::make_unique<FunctionDeclaration>(name, std::move(parameters), std::move(body));
}

std::unique_ptr<Declaration> Parser::classDeclaration() {
    consume(lexer::TokenType::IDENTIFIER, "Expect class name.");
    auto name = tokens[current - 1].lexeme;
    
    std::string base_class;
    if (match(lexer::TokenType::LPAREN)) {
        consume(lexer::TokenType::IDENTIFIER, "Expect base class name.");
        base_class = tokens[current - 1].lexeme;
        consume(lexer::TokenType::RPAREN, "Expect ')' after base class.");
    }
    
    consume(lexer::TokenType::COLON, "Expect ':' after class declaration.");
    
    std::vector<std::unique_ptr<Declaration>> members;
    while (!isAtEnd() && !check(lexer::TokenType::DEDENT)) {
        if (auto member = declaration()) {
            members.push_back(std::move(member));
        } else {
            break;
        }
    }
    
    return std::make_unique<ClassDeclaration>(name, base_class, std::move(members));
}

std::unique_ptr<Declaration> Parser::importDeclaration() {
    consume(lexer::TokenType::IDENTIFIER, "Expect module name after 'import'.");
    auto module = tokens[current - 1].lexeme;
    
    std::string alias;
    if (match(lexer::TokenType::AS)) {
        consume(lexer::TokenType::IDENTIFIER, "Expect alias name after 'as'.");
        alias = tokens[current - 1].lexeme;
    }
    
    return std::make_unique<ImportDeclaration>(module, alias);
}

// Expression parsing implementations
std::unique_ptr<Expression> Parser::listExpression() {
    std::vector<std::unique_ptr<Expression>> elements;
    
    if (!check(lexer::TokenType::RBRACKET)) {
        do {
            elements.push_back(expression());
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RBRACKET, "Expect ']' after list elements.");
    
    return std::make_unique<ListExpression>(std::move(elements));
}

std::unique_ptr<Expression> Parser::dictExpression() {
    std::vector<DictExpression::KeyValue> pairs;
    
    if (!check(lexer::TokenType::RBRACE)) {
        do {
            auto key = expression();
            consume(lexer::TokenType::COLON, "Expect ':' after key.");
            auto value = expression();
            pairs.emplace_back(std::move(key), std::move(value));
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RBRACE, "Expect '}' after dictionary pairs.");
    
    return std::make_unique<DictExpression>(std::move(pairs));
}

std::unique_ptr<Expression> Parser::tupleExpression() {
    std::vector<std::unique_ptr<Expression>> elements;
    
    if (!check(lexer::TokenType::RPAREN)) {
        do {
            elements.push_back(expression());
        } while (match(lexer::TokenType::COMMA));
    }
    
    consume(lexer::TokenType::RPAREN, "Expect ')' after tuple elements.");
    
    return std::make_unique<TupleExpression>(std::move(elements));
}

std::vector<std::unique_ptr<Statement>> Parser::block() {
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!isAtEnd() && !check(lexer::TokenType::DEDENT)) {
        if (auto stmt = statement()) {
            statements.push_back(std::move(stmt));
        } else {
            break;
        }
    }
    
    return statements;
}

// Helper method implementations
bool Parser::check(lexer::TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(lexer::TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::consume(lexer::TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return;
    }
    
    error(peek(), message);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (tokens[current - 1].type == lexer::TokenType::NEWLINE) return;
        
        switch (peek().type) {
            case lexer::TokenType::CLASS:
            case lexer::TokenType::DEF:
            case lexer::TokenType::IF:
            case lexer::TokenType::WHILE:
            case lexer::TokenType::FOR:
            case lexer::TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

void Parser::error(const lexer::Token& token, const std::string& message) {
    std::string error_msg = "Error at line " + std::to_string(token.line) + 
                           ", column " + std::to_string(token.column) + ": " + message;
    throw std::runtime_error(error_msg);
}

} // namespace pulse::parser 