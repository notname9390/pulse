#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include "compiler/compiler.hpp"

// Forward declarations
namespace llvm {
    class LLVMContext;
    class Module;
    class IRBuilder;
    class Function;
    class BasicBlock;
    class Value;
    class Type;
    class StructType;
    class AllocaInst;
}

namespace pulse::compiler {

class LLVMBackend {
public:
    LLVMBackend();
    ~LLVMBackend();
    
    // Initialize LLVM
    bool initialize();
    
    // Create module
    std::unique_ptr<llvm::Module> createModule(const std::string& name);
    
    // Type management
    llvm::Type* getIntType() const;
    llvm::Type* getFloatType() const;
    llvm::Type* getBoolType() const;
    llvm::Type* getStringType() const;
    llvm::Type* getVoidType() const;
    llvm::Type* getPointerType(llvm::Type* elementType) const;
    
    // Function management
    llvm::Function* createFunction(llvm::Module* module, const std::string& name,
                                  llvm::Type* returnType, 
                                  const std::vector<llvm::Type*>& paramTypes);
    
    // Basic block management
    llvm::BasicBlock* createBasicBlock(llvm::Function* function, const std::string& name);
    
    // Value creation
    llvm::Value* createIntegerLiteral(llvm::LLVMContext* context, int64_t value);
    llvm::Value* createFloatLiteral(llvm::LLVMContext* context, double value);
    llvm::Value* createBooleanLiteral(llvm::LLVMContext* context, bool value);
    llvm::Value* createStringLiteral(llvm::Module* module, const std::string& value);
    
    // Arithmetic operations
    llvm::Value* createAdd(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createSubtract(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createMultiply(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createDivide(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createModulo(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    
    // Comparison operations
    llvm::Value* createEqual(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createNotEqual(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createLessThan(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createLessEqual(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createGreaterThan(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createGreaterEqual(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    
    // Logical operations
    llvm::Value* createAnd(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createOr(llvm::IRBuilder<>* builder, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* createNot(llvm::IRBuilder<>* builder, llvm::Value* operand);
    
    // Control flow
    void createReturn(llvm::IRBuilder<>* builder, llvm::Value* value = nullptr);
    void createBranch(llvm::IRBuilder<>* builder, llvm::BasicBlock* target);
    void createConditionalBranch(llvm::IRBuilder<>* builder, llvm::Value* condition,
                                llvm::BasicBlock* trueBlock, llvm::BasicBlock* falseBlock);
    
    // Variable management
    llvm::AllocaInst* createAlloca(llvm::IRBuilder<>* builder, llvm::Type* type, 
                                   const std::string& name);
    void createStore(llvm::IRBuilder<>* builder, llvm::Value* value, llvm::Value* address);
    llvm::Value* createLoad(llvm::IRBuilder<>* builder, llvm::Type* type, llvm::Value* address);
    
    // Function calls
    llvm::Value* createCall(llvm::IRBuilder<>* builder, llvm::Function* function,
                           const std::vector<llvm::Value*>& arguments);
    
    // Standard library functions
    void declarePrintf(llvm::Module* module);
    void declareMalloc(llvm::Module* module);
    void declareFree(llvm::Module* module);
    
    // Error handling
    void reportError(const std::string& message);
    bool hasErrors() const;
    std::vector<std::string> getErrors() const;

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::vector<std::string> errors;
    
    // Type cache
    std::map<std::string, llvm::Type*> typeCache;
    
    // Helper methods
    void initializeTypes();
    llvm::Type* getCachedType(const std::string& name);
    void cacheType(const std::string& name, llvm::Type* type);
};

} // namespace pulse::compiler 