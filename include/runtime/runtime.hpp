#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <variant>

namespace pulse::runtime {

// Forward declarations
class RuntimeValue;
class RuntimeFunction;
class RuntimeContext;

// Type definitions
using RuntimeValuePtr = std::unique_ptr<RuntimeValue>;
using RuntimeFunctionPtr = std::unique_ptr<RuntimeFunction>;

// Runtime value types
enum class ValueType {
    NONE,
    INTEGER,
    FLOAT,
    BOOLEAN,
    STRING,
    LIST,
    DICT,
    FUNCTION,
    CLASS_INSTANCE
};

// Base runtime value class
class RuntimeValue {
public:
    virtual ~RuntimeValue() = default;
    virtual ValueType getType() const = 0;
    virtual std::string toString() const = 0;
    virtual RuntimeValuePtr clone() const = 0;
};

// Integer value
class IntegerValue : public RuntimeValue {
public:
    int64_t value;
    
    explicit IntegerValue(int64_t val) : value(val) {}
    
    ValueType getType() const override { return ValueType::INTEGER; }
    std::string toString() const override { return std::to_string(value); }
    RuntimeValuePtr clone() const override { return std::make_unique<IntegerValue>(value); }
};

// Float value
class FloatValue : public RuntimeValue {
public:
    double value;
    
    explicit FloatValue(double val) : value(val) {}
    
    ValueType getType() const override { return ValueType::FLOAT; }
    std::string toString() const override { return std::to_string(value); }
    RuntimeValuePtr clone() const override { return std::make_unique<FloatValue>(value); }
};

// Boolean value
class BooleanValue : public RuntimeValue {
public:
    bool value;
    
    explicit BooleanValue(bool val) : value(val) {}
    
    ValueType getType() const override { return ValueType::BOOLEAN; }
    std::string toString() const override { return value ? "True" : "False"; }
    RuntimeValuePtr clone() const override { return std::make_unique<BooleanValue>(value); }
};

// String value
class StringValue : public RuntimeValue {
public:
    std::string value;
    
    explicit StringValue(const std::string& val) : value(val) {}
    
    ValueType getType() const override { return ValueType::STRING; }
    std::string toString() const override { return value; }
    RuntimeValuePtr clone() const override { return std::make_unique<StringValue>(value); }
};

// List value
class ListValue : public RuntimeValue {
public:
    std::vector<RuntimeValuePtr> elements;
    
    explicit ListValue(std::vector<RuntimeValuePtr> elems = {}) : elements(std::move(elems)) {}
    
    ValueType getType() const override { return ValueType::LIST; }
    std::string toString() const override;
    RuntimeValuePtr clone() const override;
    
    void append(RuntimeValuePtr element);
    RuntimeValuePtr get(size_t index) const;
    void set(size_t index, RuntimeValuePtr value);
    size_t size() const { return elements.size(); }
};

// Dictionary value
class DictValue : public RuntimeValue {
public:
    std::map<std::string, RuntimeValuePtr> pairs;
    
    explicit DictValue(std::map<std::string, RuntimeValuePtr> p = {}) : pairs(std::move(p)) {}
    
    ValueType getType() const override { return ValueType::DICT; }
    std::string toString() const override;
    RuntimeValuePtr clone() const override;
    
    void set(const std::string& key, RuntimeValuePtr value);
    RuntimeValuePtr get(const std::string& key) const;
    bool hasKey(const std::string& key) const;
    std::vector<std::string> keys() const;
};

// Function value
class RuntimeFunction : public RuntimeValue {
public:
    using FunctionPtr = std::function<RuntimeValuePtr(const std::vector<RuntimeValuePtr>&)>;
    
    std::string name;
    FunctionPtr function;
    std::vector<std::string> parameterNames;
    
    RuntimeFunction(const std::string& funcName, FunctionPtr func, 
                   std::vector<std::string> params = {})
        : name(funcName), function(std::move(func)), parameterNames(std::move(params)) {}
    
    ValueType getType() const override { return ValueType::FUNCTION; }
    std::string toString() const override { return "<function " + name + ">"; }
    RuntimeValuePtr clone() const override { return std::make_unique<RuntimeFunction>(name, function, parameterNames); }
    
    RuntimeValuePtr call(const std::vector<RuntimeValuePtr>& arguments);
};

// Runtime context for variable scope management
class RuntimeContext {
public:
    RuntimeContext(RuntimeContext* parent = nullptr);
    
    void setVariable(const std::string& name, RuntimeValuePtr value);
    RuntimeValuePtr getVariable(const std::string& name) const;
    bool hasVariable(const std::string& name) const;
    void removeVariable(const std::string& name);
    
    // Scope management
    RuntimeContext* getParent() const { return parent; }
    std::unique_ptr<RuntimeContext> createChildScope();
    
    // Get all variables in current scope
    std::map<std::string, RuntimeValuePtr> getVariables() const;

private:
    std::map<std::string, RuntimeValuePtr> variables;
    RuntimeContext* parent;
};

// Main runtime system
class Runtime {
public:
    Runtime();
    ~Runtime();
    
    // Initialize runtime with standard library
    void initialize();
    
    // Execute code
    RuntimeValuePtr execute(const std::string& code);
    
    // Get global context
    RuntimeContext* getGlobalContext() const;
    
    // Standard library functions
    void setupStandardLibrary();
    
    // Error handling
    void reportError(const std::string& message);
    bool hasErrors() const;
    std::vector<std::string> getErrors() const;

private:
    std::unique_ptr<RuntimeContext> globalContext;
    std::vector<std::string> errors;
    
    // Standard library functions
    RuntimeValuePtr printFunction(const std::vector<RuntimeValuePtr>& args);
    RuntimeValuePtr lenFunction(const std::vector<RuntimeValuePtr>& args);
    RuntimeValuePtr strFunction(const std::vector<RuntimeValuePtr>& args);
    RuntimeValuePtr intFunction(const std::vector<RuntimeValuePtr>& args);
    RuntimeValuePtr floatFunction(const std::vector<RuntimeValuePtr>& args);
    RuntimeValuePtr boolFunction(const std::vector<RuntimeValuePtr>& args);
};

} // namespace pulse::runtime 