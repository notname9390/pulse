#include <iostream>
#include <fstream>
#include <string>
#include "lexer/tokenizer.hpp"
#include "parser/parser.hpp"

void printTokens(const std::vector<pulse::lexer::Token>& tokens) {
    std::cout << "=== Tokens ===" << std::endl;
    for (const auto& token : tokens) {
        std::cout << "Type: " << static_cast<int>(token.type) 
                  << ", Lexeme: '" << token.lexeme << "'"
                  << ", Line: " << token.line 
                  << ", Column: " << token.column << std::endl;
    }
    std::cout << "=============" << std::endl;
}

void printAST(pulse::parser::ASTNode* node, int depth = 0) {
    std::string indent(depth * 2, ' ');
    
    if (auto program = dynamic_cast<pulse::parser::Program*>(node)) {
        std::cout << indent << "Program:" << std::endl;
        for (const auto& decl : program->declarations) {
            printAST(decl.get(), depth + 1);
        }
        for (const auto& stmt : program->statements) {
            printAST(stmt.get(), depth + 1);
        }
    } else if (auto func = dynamic_cast<pulse::parser::FunctionDeclaration*>(node)) {
        std::cout << indent << "Function: " << func->name << std::endl;
        for (const auto& param : func->parameters) {
            std::cout << indent << "  Param: " << param << std::endl;
        }
        for (const auto& stmt : func->body) {
            printAST(stmt.get(), depth + 1);
        }
    } else if (auto assign = dynamic_cast<pulse::parser::AssignmentStatement*>(node)) {
        std::cout << indent << "Assignment: " << assign->name << std::endl;
        printAST(assign->value.get(), depth + 1);
    } else if (auto expr = dynamic_cast<pulse::parser::ExpressionStatement*>(node)) {
        std::cout << indent << "Expression:" << std::endl;
        printAST(expr->expression.get(), depth + 1);
    } else if (auto literal = dynamic_cast<pulse::parser::LiteralExpression*>(node)) {
        std::cout << indent << "Literal: ";
        if (std::holds_alternative<std::string>(literal->value)) {
            std::cout << "'" << std::get<std::string>(literal->value) << "'";
        } else if (std::holds_alternative<int64_t>(literal->value)) {
            std::cout << std::get<int64_t>(literal->value);
        } else if (std::holds_alternative<double>(literal->value)) {
            std::cout << std::get<double>(literal->value);
        } else if (std::holds_alternative<bool>(literal->value)) {
            std::cout << (std::get<bool>(literal->value) ? "True" : "False");
        } else {
            std::cout << "None";
        }
        std::cout << std::endl;
    } else if (auto id = dynamic_cast<pulse::parser::IdentifierExpression*>(node)) {
        std::cout << indent << "Identifier: " << id->name << std::endl;
    } else if (auto binary = dynamic_cast<pulse::parser::BinaryExpression*>(node)) {
        std::cout << indent << "Binary Op: " << static_cast<int>(binary->op) << std::endl;
        printAST(binary->left.get(), depth + 1);
        printAST(binary->right.get(), depth + 1);
    } else if (auto call = dynamic_cast<pulse::parser::CallExpression*>(node)) {
        std::cout << indent << "Function Call:" << std::endl;
        printAST(call->callee.get(), depth + 1);
        for (const auto& arg : call->arguments) {
            printAST(arg.get(), depth + 1);
        }
    } else if (auto if_stmt = dynamic_cast<pulse::parser::IfStatement*>(node)) {
        std::cout << indent << "If Statement:" << std::endl;
        for (const auto& branch : if_stmt->branches) {
            std::cout << indent << "  Condition:" << std::endl;
            printAST(branch.condition.get(), depth + 2);
            std::cout << indent << "  Body:" << std::endl;
            for (const auto& stmt : branch.body) {
                printAST(stmt.get(), depth + 2);
            }
        }
        if (!if_stmt->else_body.empty()) {
            std::cout << indent << "  Else:" << std::endl;
            for (const auto& stmt : if_stmt->else_body) {
                printAST(stmt.get(), depth + 2);
            }
        }
    } else {
        std::cout << indent << "Unknown Node Type" << std::endl;
    }
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

int main(int argc, char* argv[]) {
    try {
        std::string source;
        
        if (argc > 1) {
            // Read from file
            source = readFile(argv[1]);
        } else {
            // Use example source code
            source = R"(
# Example Pulse program
def greet(name):
    if name == "World":
        out("Hello, " + name + "!")
    else:
        out("Hello, " + name)

def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

# Main program
greet("World")
result = factorial(5)
out("Factorial of 5 is: " + str(result))
)";
        }
        
        std::cout << "=== Pulse Compiler ===" << std::endl;
        std::cout << "Source code:" << std::endl;
        std::cout << source << std::endl;
        
        // Tokenize
        std::cout << "\n=== Tokenization ===" << std::endl;
        pulse::lexer::Tokenizer tokenizer(source);
        auto tokens = tokenizer.tokenize();
        printTokens(tokens);
        
        // Parse
        std::cout << "\n=== Parsing ===" << std::endl;
        pulse::parser::Parser parser(tokens);
        auto ast = parser.parse();
        
        if (ast) {
            std::cout << "Parse successful!" << std::endl;
            std::cout << "\n=== Abstract Syntax Tree ===" << std::endl;
            printAST(ast.get());
        } else {
            std::cout << "Parse failed!" << std::endl;
            return 1;
        }
        
        std::cout << "\n=== Compilation Complete ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 