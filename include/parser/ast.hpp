#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace pulse::parser {

// Forward declarations
class ASTNode;
class Expression;
class Statement;
class Declaration;

// Type definitions for convenience
using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;
using DeclarationPtr = std::unique_ptr<Declaration>;

// Base class for all AST nodes
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(class ASTVisitor& visitor) = 0;
};

// Expression base class
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

// Statement base class
class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

// Declaration base class
class Declaration : public ASTNode {
public:
    virtual ~Declaration() = default;
};

// Literal expressions
class LiteralExpression : public Expression {
public:
    std::variant<std::string, int64_t, double, bool, std::monostate> value;
    
    explicit LiteralExpression(const std::string& value) : value(value) {}
    explicit LiteralExpression(int64_t value) : value(value) {}
    explicit LiteralExpression(double value) : value(value) {}
    explicit LiteralExpression(bool value) : value(value) {}
    LiteralExpression() : value(std::monostate{}) {} // None
    
    void accept(ASTVisitor& visitor) override;
};

// Identifier expression
class IdentifierExpression : public Expression {
public:
    std::string name;
    
    explicit IdentifierExpression(const std::string& name) : name(name) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Binary operation expression
class BinaryExpression : public Expression {
public:
    enum class Operator {
        ADD, SUBTRACT, MULTIPLY, DIVIDE, FLOOR_DIVIDE, MODULO, POWER,
        EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
        AND, OR
    };
    
    Operator op;
    ExpressionPtr left;
    ExpressionPtr right;
    
    BinaryExpression(Operator op, ExpressionPtr left, ExpressionPtr right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Unary operation expression
class UnaryExpression : public Expression {
public:
    enum class Operator {
        PLUS, MINUS, NOT
    };
    
    Operator op;
    ExpressionPtr operand;
    
    UnaryExpression(Operator op, ExpressionPtr operand)
        : op(op), operand(std::move(operand)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Function call expression
class CallExpression : public Expression {
public:
    ExpressionPtr callee;
    std::vector<ExpressionPtr> arguments;
    
    CallExpression(ExpressionPtr callee, std::vector<ExpressionPtr> arguments)
        : callee(std::move(callee)), arguments(std::move(arguments)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Attribute access expression
class AttributeExpression : public Expression {
public:
    ExpressionPtr object;
    std::string attribute;
    
    AttributeExpression(ExpressionPtr object, const std::string& attribute)
        : object(std::move(object)), attribute(attribute) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Subscript expression
class SubscriptExpression : public Expression {
public:
    ExpressionPtr object;
    ExpressionPtr index;
    
    SubscriptExpression(ExpressionPtr object, ExpressionPtr index)
        : object(std::move(object)), index(std::move(index)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// List expression
class ListExpression : public Expression {
public:
    std::vector<ExpressionPtr> elements;
    
    explicit ListExpression(std::vector<ExpressionPtr> elements)
        : elements(std::move(elements)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Dictionary expression
class DictExpression : public Expression {
public:
    struct KeyValue {
        ExpressionPtr key;
        ExpressionPtr value;
        
        KeyValue(ExpressionPtr key, ExpressionPtr value)
            : key(std::move(key)), value(std::move(value)) {}
    };
    
    std::vector<KeyValue> pairs;
    
    explicit DictExpression(std::vector<KeyValue> pairs)
        : pairs(std::move(pairs)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Tuple expression
class TupleExpression : public Expression {
public:
    std::vector<ExpressionPtr> elements;
    
    explicit TupleExpression(std::vector<ExpressionPtr> elements)
        : elements(std::move(elements)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Assignment statement
class AssignmentStatement : public Statement {
public:
    std::string name;
    ExpressionPtr value;
    
    AssignmentStatement(const std::string& name, ExpressionPtr value)
        : name(name), value(std::move(value)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Expression statement
class ExpressionStatement : public Statement {
public:
    ExpressionPtr expression;
    
    explicit ExpressionStatement(ExpressionPtr expression)
        : expression(std::move(expression)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Return statement
class ReturnStatement : public Statement {
public:
    ExpressionPtr value;
    
    explicit ReturnStatement(ExpressionPtr value = nullptr)
        : value(std::move(value)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// If statement
class IfStatement : public Statement {
public:
    struct Branch {
        ExpressionPtr condition;
        std::vector<StatementPtr> body;
        
        Branch(ExpressionPtr condition, std::vector<StatementPtr> body)
            : condition(std::move(condition)), body(std::move(body)) {}
    };
    
    std::vector<Branch> branches;
    std::vector<StatementPtr> else_body;
    
    IfStatement(std::vector<Branch> branches, std::vector<StatementPtr> else_body = {})
        : branches(std::move(branches)), else_body(std::move(else_body)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// While statement
class WhileStatement : public Statement {
public:
    ExpressionPtr condition;
    std::vector<StatementPtr> body;
    
    WhileStatement(ExpressionPtr condition, std::vector<StatementPtr> body)
        : condition(std::move(condition)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// For statement
class ForStatement : public Statement {
public:
    std::string variable;
    ExpressionPtr iterable;
    std::vector<StatementPtr> body;
    
    ForStatement(const std::string& variable, ExpressionPtr iterable, std::vector<StatementPtr> body)
        : variable(variable), iterable(std::move(iterable)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Match statement
class MatchStatement : public Statement {
public:
    ExpressionPtr value;
    std::vector<std::pair<ExpressionPtr, std::vector<StatementPtr>>> cases;
    
    MatchStatement(ExpressionPtr value, std::vector<std::pair<ExpressionPtr, std::vector<StatementPtr>>> cases)
        : value(std::move(value)), cases(std::move(cases)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Function definition
class FunctionDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<StatementPtr> body;
    bool is_async;
    
    FunctionDeclaration(const std::string& name, std::vector<std::string> parameters,
                       std::vector<StatementPtr> body, bool is_async = false)
        : name(name), parameters(std::move(parameters)), body(std::move(body)), is_async(is_async) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Class definition
class ClassDeclaration : public Declaration {
public:
    std::string name;
    std::string base_class;
    std::vector<DeclarationPtr> members;
    
    ClassDeclaration(const std::string& name, const std::string& base_class = "",
                     std::vector<DeclarationPtr> members = {})
        : name(name), base_class(base_class), members(std::move(members)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Import statement
class ImportDeclaration : public Declaration {
public:
    std::string module;
    std::string alias;
    
    ImportDeclaration(const std::string& module, const std::string& alias = "")
        : module(module), alias(alias) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Program root
class Program : public ASTNode {
public:
    std::vector<DeclarationPtr> declarations;
    std::vector<StatementPtr> statements;
    
    Program(std::vector<DeclarationPtr> declarations = {}, std::vector<StatementPtr> statements = {})
        : declarations(std::move(declarations)), statements(std::move(statements)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Visitor pattern for AST traversal
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Expression visitors
    virtual void visitLiteralExpression(LiteralExpression* expr) = 0;
    virtual void visitIdentifierExpression(IdentifierExpression* expr) = 0;
    virtual void visitBinaryExpression(BinaryExpression* expr) = 0;
    virtual void visitUnaryExpression(UnaryExpression* expr) = 0;
    virtual void visitCallExpression(CallExpression* expr) = 0;
    virtual void visitAttributeExpression(AttributeExpression* expr) = 0;
    virtual void visitSubscriptExpression(SubscriptExpression* expr) = 0;
    virtual void visitListExpression(ListExpression* expr) = 0;
    virtual void visitDictExpression(DictExpression* expr) = 0;
    virtual void visitTupleExpression(TupleExpression* expr) = 0;
    
    // Statement visitors
    virtual void visitAssignmentStatement(AssignmentStatement* stmt) = 0;
    virtual void visitExpressionStatement(ExpressionStatement* stmt) = 0;
    virtual void visitReturnStatement(ReturnStatement* stmt) = 0;
    virtual void visitIfStatement(IfStatement* stmt) = 0;
    virtual void visitWhileStatement(WhileStatement* stmt) = 0;
    virtual void visitForStatement(ForStatement* stmt) = 0;
    virtual void visitMatchStatement(MatchStatement* stmt) = 0;
    
    // Declaration visitors
    virtual void visitFunctionDeclaration(FunctionDeclaration* decl) = 0;
    virtual void visitClassDeclaration(ClassDeclaration* decl) = 0;
    virtual void visitImportDeclaration(ImportDeclaration* decl) = 0;
    
    // Program visitor
    virtual void visitProgram(Program* program) = 0;
};

} // namespace pulse::parser 