#pragma once

#include <memory>
#include <string>
#include <vector>
#include "parser/ast.hpp"

// Forward declarations
namespace llvm {
    class LLVMContext;
    class Module;
    class IRBuilder;
    class Function;
    class BasicBlock;
    class Value;
    class Type;
}

namespace pulse::compiler {

class Compiler {
public:
    Compiler();
    ~Compiler();
    
    // Compile AST to LLVM IR
    bool compile(pulse::parser::Program* program, const std::string& outputFile = "");
    
    // Get generated LLVM IR as string
    std::string getIRString() const;
    
    // Get generated LLVM module
    llvm::Module* getModule() const;
    
    // Get LLVM context
    llvm::LLVMContext* getContext() const;

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    // Compilation state
    llvm::Function* currentFunction;
    llvm::BasicBlock* currentBlock;
    
    // Symbol table for variables
    std::map<std::string, llvm::Value*> variables;
    
    // Helper methods
    llvm::Value* compileExpression(pulse::parser::Expression* expr);
    llvm::Value* compileStatement(pulse::parser::Statement* stmt);
    llvm::Value* compileDeclaration(pulse::parser::Declaration* decl);
    
    // Expression compilation
    llvm::Value* compileLiteralExpression(pulse::parser::LiteralExpression* expr);
    llvm::Value* compileIdentifierExpression(pulse::parser::IdentifierExpression* expr);
    llvm::Value* compileBinaryExpression(pulse::parser::BinaryExpression* expr);
    llvm::Value* compileUnaryExpression(pulse::parser::UnaryExpression* expr);
    llvm::Value* compileCallExpression(pulse::parser::CallExpression* expr);
    
    // Statement compilation
    void compileAssignmentStatement(pulse::parser::AssignmentStatement* stmt);
    void compileExpressionStatement(pulse::parser::ExpressionStatement* stmt);
    void compileReturnStatement(pulse::parser::ReturnStatement* stmt);
    void compileIfStatement(pulse::parser::IfStatement* stmt);
    void compileWhileStatement(pulse::parser::WhileStatement* stmt);
    void compileForStatement(pulse::parser::ForStatement* stmt);
    
    // Declaration compilation
    void compileFunctionDeclaration(pulse::parser::FunctionDeclaration* decl);
    void compileClassDeclaration(pulse::parser::ClassDeclaration* decl);
    
    // Type helpers
    llvm::Type* getLLVMType(const std::string& typeName);
    llvm::Type* getLLVMType(pulse::parser::Expression* expr);
    
    // Utility methods
    void createMainFunction();
    void setupStandardLibrary();
    llvm::Function* getOrCreateFunction(const std::string& name, llvm::Type* returnType, 
                                       const std::vector<llvm::Type*>& paramTypes);
};

} // namespace pulse::compiler 