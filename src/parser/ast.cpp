#include "parser/ast.hpp"

namespace pulse::parser {

// LiteralExpression accept method
void LiteralExpression::accept(ASTVisitor& visitor) {
    visitor.visitLiteralExpression(this);
}

// IdentifierExpression accept method
void IdentifierExpression::accept(ASTVisitor& visitor) {
    visitor.visitIdentifierExpression(this);
}

// BinaryExpression accept method
void BinaryExpression::accept(ASTVisitor& visitor) {
    visitor.visitBinaryExpression(this);
}

// UnaryExpression accept method
void UnaryExpression::accept(ASTVisitor& visitor) {
    visitor.visitUnaryExpression(this);
}

// CallExpression accept method
void CallExpression::accept(ASTVisitor& visitor) {
    visitor.visitCallExpression(this);
}

// AttributeExpression accept method
void AttributeExpression::accept(ASTVisitor& visitor) {
    visitor.visitAttributeExpression(this);
}

// SubscriptExpression accept method
void SubscriptExpression::accept(ASTVisitor& visitor) {
    visitor.visitSubscriptExpression(this);
}

// ListExpression accept method
void ListExpression::accept(ASTVisitor& visitor) {
    visitor.visitListExpression(this);
}

// DictExpression accept method
void DictExpression::accept(ASTVisitor& visitor) {
    visitor.visitDictExpression(this);
}

// TupleExpression accept method
void TupleExpression::accept(ASTVisitor& visitor) {
    visitor.visitTupleExpression(this);
}

// AssignmentStatement accept method
void AssignmentStatement::accept(ASTVisitor& visitor) {
    visitor.visitAssignmentStatement(this);
}

// ExpressionStatement accept method
void ExpressionStatement::accept(ASTVisitor& visitor) {
    visitor.visitExpressionStatement(this);
}

// ReturnStatement accept method
void ReturnStatement::accept(ASTVisitor& visitor) {
    visitor.visitReturnStatement(this);
}

// IfStatement accept method
void IfStatement::accept(ASTVisitor& visitor) {
    visitor.visitIfStatement(this);
}

// WhileStatement accept method
void WhileStatement::accept(ASTVisitor& visitor) {
    visitor.visitWhileStatement(this);
}

// ForStatement accept method
void ForStatement::accept(ASTVisitor& visitor) {
    visitor.visitForStatement(this);
}

// MatchStatement accept method
void MatchStatement::accept(ASTVisitor& visitor) {
    visitor.visitMatchStatement(this);
}

// FunctionDeclaration accept method
void FunctionDeclaration::accept(ASTVisitor& visitor) {
    visitor.visitFunctionDeclaration(this);
}

// ClassDeclaration accept method
void ClassDeclaration::accept(ASTVisitor& visitor) {
    visitor.visitClassDeclaration(this);
}

// ImportDeclaration accept method
void ImportDeclaration::accept(ASTVisitor& visitor) {
    visitor.visitImportDeclaration(this);
}

// Program accept method
void Program::accept(ASTVisitor& visitor) {
    visitor.visitProgram(this);
}

} // namespace pulse::parser 