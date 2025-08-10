#include "compiler/compiler.hpp"
#include "compiler/llvm_backend.hpp"
#include <iostream>
#include <stdexcept>

namespace pulse::compiler {

Compiler::Compiler() 
    : context(std::make_unique<llvm::LLVMContext>()),
      module(std::make_unique<llvm::Module>("pulse_module", *context)),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)),
      currentFunction(nullptr),
      currentBlock(nullptr) {
}

Compiler::~Compiler() = default;

bool Compiler::compile(pulse::parser::Program* program, const std::string& outputFile) {
    try {
        if (!program) {
            throw std::runtime_error("No program to compile");
        }
        
        // Setup standard library
        setupStandardLibrary();
        
        // Create main function
        createMainFunction();
        
        // Compile all declarations
        for (const auto& decl : program->declarations) {
            compileDeclaration(decl.get());
        }
        
        // Compile all statements in main
        for (const auto& stmt : program->statements) {
            compileStatement(stmt.get());
        }
        
        // Add return statement to main if not present
        if (!currentBlock->getTerminator()) {
            builder->CreateRet(builder->getInt32(0));
        }
        
        // Verify module
        std::string error;
        if (llvm::verifyModule(*module, &error)) {
            throw std::runtime_error("Module verification failed: " + error);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
        return false;
    }
}

std::string Compiler::getIRString() const {
    std::string irString;
    llvm::raw_string_ostream stream(irString);
    module->print(stream, nullptr);
    return irString;
}

llvm::Module* Compiler::getModule() const {
    return module.get();
}

llvm::LLVMContext* Compiler::getContext() const {
    return context.get();
}

llvm::Value* Compiler::compileExpression(pulse::parser::Expression* expr) {
    if (!expr) return nullptr;
    
    if (auto literal = dynamic_cast<pulse::parser::LiteralExpression*>(expr)) {
        return compileLiteralExpression(literal);
    } else if (auto id = dynamic_cast<pulse::parser::IdentifierExpression*>(expr)) {
        return compileIdentifierExpression(id);
    } else if (auto binary = dynamic_cast<pulse::parser::BinaryExpression*>(expr)) {
        return compileBinaryExpression(binary);
    } else if (auto unary = dynamic_cast<pulse::parser::UnaryExpression*>(expr)) {
        return compileUnaryExpression(unary);
    } else if (auto call = dynamic_cast<pulse::parser::CallExpression*>(expr)) {
        return compileCallExpression(call);
    }
    
    return nullptr;
}

llvm::Value* Compiler::compileLiteralExpression(pulse::parser::LiteralExpression* expr) {
    if (std::holds_alternative<int64_t>(expr->value)) {
        return builder->getInt64(std::get<int64_t>(expr->value));
    } else if (std::holds_alternative<double>(expr->value)) {
        return llvm::ConstantFP::get(builder->getDoubleTy(), std::get<double>(expr->value));
    } else if (std::holds_alternative<bool>(expr->value)) {
        return builder->getInt1(std::get<bool>(expr->value));
    } else if (std::holds_alternative<std::string>(expr->value)) {
        return builder->CreateGlobalStringPtr(std::get<std::string>(expr->value));
    }
    
    return builder->getInt32(0); // None/null
}

llvm::Value* Compiler::compileIdentifierExpression(pulse::parser::IdentifierExpression* expr) {
    auto it = variables.find(expr->name);
    if (it != variables.end()) {
        return builder->CreateLoad(builder->getInt64Ty(), it->second);
    }
    
    // Return default value for undefined variables
    return builder->getInt64(0);
}

llvm::Value* Compiler::compileBinaryExpression(pulse::parser::BinaryExpression* expr) {
    auto left = compileExpression(expr->left.get());
    auto right = compileExpression(expr->right.get());
    
    if (!left || !right) return nullptr;
    
    switch (expr->op) {
        case pulse::parser::BinaryExpression::Operator::ADD:
            return builder->CreateAdd(left, right);
        case pulse::parser::BinaryExpression::Operator::SUBTRACT:
            return builder->CreateSub(left, right);
        case pulse::parser::BinaryExpression::Operator::MULTIPLY:
            return builder->CreateMul(left, right);
        case pulse::parser::BinaryExpression::Operator::DIVIDE:
            return builder->CreateSDiv(left, right);
        case pulse::parser::BinaryExpression::Operator::MODULO:
            return builder->CreateSRem(left, right);
        case pulse::parser::BinaryExpression::Operator::EQUAL:
            return builder->CreateICmpEQ(left, right);
        case pulse::parser::BinaryExpression::Operator::NOT_EQUAL:
            return builder->CreateICmpNE(left, right);
        case pulse::parser::BinaryExpression::Operator::LESS:
            return builder->CreateICmpSLT(left, right);
        case pulse::parser::BinaryExpression::Operator::LESS_EQUAL:
            return builder->CreateICmpSLE(left, right);
        case pulse::parser::BinaryExpression::Operator::GREATER:
            return builder->CreateICmpSGT(left, right);
        case pulse::parser::BinaryExpression::Operator::GREATER_EQUAL:
            return builder->CreateICmpSGE(left, right);
        default:
            return nullptr;
    }
}

llvm::Value* Compiler::compileUnaryExpression(pulse::parser::UnaryExpression* expr) {
    auto operand = compileExpression(expr->operand.get());
    if (!operand) return nullptr;
    
    switch (expr->op) {
        case pulse::parser::UnaryExpression::Operator::PLUS:
            return operand;
        case pulse::parser::UnaryExpression::Operator::MINUS:
            return builder->CreateNeg(operand);
        case pulse::parser::UnaryExpression::Operator::NOT:
            return builder->CreateNot(operand);
        default:
            return nullptr;
    }
}

llvm::Value* Compiler::compileCallExpression(pulse::parser::CallExpression* expr) {
    auto callee = compileExpression(expr->callee.get());
    if (!callee) return nullptr;
    
    std::vector<llvm::Value*> args;
    for (const auto& arg : expr->arguments) {
        auto compiledArg = compileExpression(arg.get());
        if (compiledArg) {
            args.push_back(compiledArg);
        }
    }
    
    // For now, assume all functions return int64
    return builder->CreateCall(
        llvm::FunctionType::get(builder->getInt64Ty(), false),
        callee,
        args
    );
}

void Compiler::compileStatement(pulse::parser::Statement* stmt) {
    if (!stmt) return;
    
    if (auto assign = dynamic_cast<pulse::parser::AssignmentStatement*>(stmt)) {
        compileAssignmentStatement(assign);
    } else if (auto expr = dynamic_cast<pulse::parser::ExpressionStatement*>(stmt)) {
        compileExpressionStatement(expr);
    } else if (auto ret = dynamic_cast<pulse::parser::ReturnStatement*>(stmt)) {
        compileReturnStatement(ret);
    } else if (auto if_stmt = dynamic_cast<pulse::parser::IfStatement*>(stmt)) {
        compileIfStatement(if_stmt);
    } else if (auto while_stmt = dynamic_cast<pulse::parser::WhileStatement*>(stmt)) {
        compileWhileStatement(while_stmt);
    } else if (auto for_stmt = dynamic_cast<pulse::parser::ForStatement*>(stmt)) {
        compileForStatement(for_stmt);
    }
}

void Compiler::compileAssignmentStatement(pulse::parser::AssignmentStatement* stmt) {
    auto value = compileExpression(stmt->value.get());
    if (!value) return;
    
    // Create or update variable
    auto it = variables.find(stmt->name);
    if (it != variables.end()) {
        builder->CreateStore(value, it->second);
    } else {
        auto alloca = builder->CreateAlloca(value->getType(), nullptr, stmt->name);
        builder->CreateStore(value, alloca);
        variables[stmt->name] = alloca;
    }
}

void Compiler::compileExpressionStatement(pulse::parser::ExpressionStatement* stmt) {
    compileExpression(stmt->expression.get());
}

void Compiler::compileReturnStatement(pulse::parser::ReturnStatement* stmt) {
    if (stmt->value) {
        auto value = compileExpression(stmt->value.get());
        builder->CreateRet(value);
    } else {
        builder->CreateRetVoid();
    }
}

void Compiler::compileIfStatement(pulse::parser::IfStatement* stmt) {
    // Implementation for if statements
    // This is a simplified version - would need more complex control flow handling
}

void Compiler::compileWhileStatement(pulse::parser::WhileStatement* stmt) {
    // Implementation for while statements
    // This is a simplified version - would need more complex control flow handling
}

void Compiler::compileForStatement(pulse::parser::ForStatement* stmt) {
    // Implementation for for statements
    // This is a simplified version - would need more complex control flow handling
}

void Compiler::compileDeclaration(pulse::parser::Declaration* decl) {
    if (!decl) return;
    
    if (auto func = dynamic_cast<pulse::parser::FunctionDeclaration*>(decl)) {
        compileFunctionDeclaration(func);
    } else if (auto cls = dynamic_cast<pulse::parser::ClassDeclaration*>(decl)) {
        compileClassDeclaration(cls);
    }
}

void Compiler::compileFunctionDeclaration(pulse::parser::FunctionDeclaration* decl) {
    // Create function type
    std::vector<llvm::Type*> paramTypes;
    for (size_t i = 0; i < decl->parameters.size(); ++i) {
        paramTypes.push_back(builder->getInt64Ty());
    }
    
    auto funcType = llvm::FunctionType::get(builder->getInt64Ty(), paramTypes, false);
    
    // Create function
    auto function = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        decl->name,
        module.get()
    );
    
    // Create entry block
    auto entryBlock = llvm::BasicBlock::Create(*context, "entry", function);
    builder->SetInsertPoint(entryBlock);
    
    // Store function parameters
    size_t paramIndex = 0;
    for (auto& arg : function->args()) {
        auto alloca = builder->CreateAlloca(builder->getInt64Ty(), nullptr, decl->parameters[paramIndex]);
        builder->CreateStore(&arg, alloca);
        variables[decl->parameters[paramIndex]] = alloca;
        paramIndex++;
    }
    
    // Compile function body
    for (const auto& stmt : decl->body) {
        compileStatement(stmt.get());
    }
    
    // Add return if not present
    if (!entryBlock->getTerminator()) {
        builder->CreateRet(builder->getInt64(0));
    }
}

void Compiler::compileClassDeclaration(pulse::parser::ClassDeclaration* decl) {
    // Implementation for class declarations
    // This would involve creating LLVM struct types and methods
}

llvm::Type* Compiler::getLLVMType(const std::string& typeName) {
    if (typeName == "int" || typeName == "i64") {
        return builder->getInt64Ty();
    } else if (typeName == "float" || typeName == "f64") {
        return builder->getDoubleTy();
    } else if (typeName == "bool") {
        return builder->getInt1Ty();
    } else if (typeName == "str") {
        return builder->getInt8PtrTy();
    }
    
    return builder->getInt64Ty(); // Default to int64
}

llvm::Type* Compiler::getLLVMType(pulse::parser::Expression* expr) {
    // Infer type from expression
    if (auto literal = dynamic_cast<pulse::parser::LiteralExpression*>(expr)) {
        if (std::holds_alternative<int64_t>(literal->value)) {
            return builder->getInt64Ty();
        } else if (std::holds_alternative<double>(literal->value)) {
            return builder->getDoubleTy();
        } else if (std::holds_alternative<bool>(literal->value)) {
            return builder->getInt1Ty();
        } else if (std::holds_alternative<std::string>(literal->value)) {
            return builder->getInt8PtrTy();
        }
    }
    
    return builder->getInt64Ty(); // Default to int64
}

void Compiler::createMainFunction() {
    auto mainType = llvm::FunctionType::get(builder->getInt32Ty(), false);
    auto mainFunc = llvm::Function::Create(
        mainType,
        llvm::Function::ExternalLinkage,
        "main",
        module.get()
    );
    
    auto entryBlock = llvm::BasicBlock::Create(*context, "entry", mainFunc);
    builder->SetInsertPoint(entryBlock);
    
    currentFunction = mainFunc;
    currentBlock = entryBlock;
}

void Compiler::setupStandardLibrary() {
    // Declare printf
    std::vector<llvm::Type*> printfArgs = {builder->getInt8PtrTy()};
    auto printfType = llvm::FunctionType::get(builder->getInt32Ty(), printfArgs, true);
    llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", module.get());
}

llvm::Function* Compiler::getOrCreateFunction(const std::string& name, llvm::Type* returnType,
                                             const std::vector<llvm::Type*>& paramTypes) {
    auto funcType = llvm::FunctionType::get(returnType, paramTypes, false);
    return llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, module.get());
}

} // namespace pulse::compiler 