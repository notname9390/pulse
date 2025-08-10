#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <thread> // Required for std::this_thread::sleep_for
#include <chrono> // Required for std::chrono::milliseconds
#include <cstdlib> // Required for std::stoi
#include <cmath> // Required for std::any_of

struct Breakpoint {
    std::string file;
    int line;
    bool enabled;
    std::string condition;
};

struct Variable {
    std::string name;
    std::string type;
    std::string value;
};

class PulseDebugger {
public:
    PulseDebugger() : isRunning(false), currentLine(0), currentFile("") {}
    
    void run() {
        std::cout << "Pulse Debugger (puldbg)" << std::endl;
        std::cout << "Type 'help' for available commands" << std::endl;
        
        isRunning = true;
        while (isRunning) {
            std::cout << "(puldbg) ";
            std::string command;
            std::getline(std::cin, command);
            
            executeCommand(command);
        }
    }
    
private:
    bool isRunning;
    int currentLine;
    std::string currentFile;
    std::vector<Breakpoint> breakpoints;
    std::map<std::string, Variable> variables;
    std::vector<std::string> callStack;
    
    void executeCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "help" || cmd == "h") {
            showHelp();
        } else if (cmd == "quit" || cmd == "q") {
            isRunning = false;
        } else if (cmd == "break" || cmd == "b") {
            handleBreakpoint(iss);
        } else if (cmd == "run" || cmd == "r") {
            runProgram();
        } else if (cmd == "continue" || cmd == "c") {
            continueExecution();
        } else if (cmd == "step" || cmd == "s") {
            stepExecution();
        } else if (cmd == "next" || cmd == "n") {
            nextExecution();
        } else if (cmd == "print" || cmd == "p") {
            printVariable(iss);
        } else if (cmd == "info") {
            handleInfo(iss);
        } else if (cmd == "list" || cmd == "l") {
            listSource();
        } else if (cmd == "clear") {
            clearBreakpoints();
        } else if (cmd == "load") {
            loadFile(iss);
        } else if (cmd.empty()) {
            // Empty command, do nothing
        } else {
            std::cout << "Unknown command: " << cmd << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }
    
    void showHelp() {
        std::cout << "Available commands:" << std::endl;
        std::cout << "  help, h                    Show this help message" << std::endl;
        std::cout << "  quit, q                    Exit debugger" << std::endl;
        std::cout << "  break <file>:<line>, b     Set breakpoint at line" << std::endl;
        std::cout << "  run, r                     Run program until breakpoint" << std::endl;
        std::cout << "  continue, c                Continue execution" << std::endl;
        std::cout << "  step, s                    Step into function" << std::endl;
        std::cout << "  next, n                    Step over function" << std::endl;
        std::cout << "  print <var>, p             Print variable value" << std::endl;
        std::cout << "  info <type>                Show debug information" << std::endl;
        std::cout << "  list, l                    Show source code around current line" << std::endl;
        std::cout << "  clear                      Clear all breakpoints" << std::endl;
        std::cout << "  load <file>                Load source file" << std::endl;
    }
    
    void handleBreakpoint(std::istringstream& iss) {
        std::string location;
        iss >> location;
        
        if (location.empty()) {
            std::cout << "Usage: break <file>:<line>" << std::endl;
            return;
        }
        
        size_t colonPos = location.find(':');
        if (colonPos == std::string::npos) {
            std::cout << "Invalid breakpoint format. Use: <file>:<line>" << std::endl;
            return;
        }
        
        std::string file = location.substr(0, colonPos);
        std::string lineStr = location.substr(colonPos + 1);
        
        try {
            int line = std::stoi(lineStr);
            
            Breakpoint bp;
            bp.file = file;
            bp.line = line;
            bp.enabled = true;
            bp.condition = "";
            
            breakpoints.push_back(bp);
            std::cout << "Breakpoint set at " << file << ":" << line << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "Invalid line number: " << lineStr << std::endl;
        }
    }
    
    void runProgram() {
        if (currentFile.empty()) {
            std::cout << "No file loaded. Use 'load <file>' first." << std::endl;
            return;
        }
        
        std::cout << "Running program..." << std::endl;
        currentLine = 1;
        
        // Simulate program execution
        simulateExecution();
    }
    
    void continueExecution() {
        if (currentLine == 0) {
            std::cout << "No program running." << std::endl;
            return;
        }
        
        std::cout << "Continuing execution..." << std::endl;
        simulateExecution();
    }
    
    void stepExecution() {
        if (currentLine == 0) {
            std::cout << "No program running." << std::endl;
            return;
        }
        
        std::cout << "Stepping into..." << std::endl;
        currentLine++;
        checkBreakpoints();
        showCurrentLine();
    }
    
    void nextExecution() {
        if (currentLine == 0) {
            std::cout << "No program running." << std::endl;
            return;
        }
        
        std::cout << "Stepping over..." << std::endl;
        currentLine++;
        checkBreakpoints();
        showCurrentLine();
    }
    
    void printVariable(std::istringstream& iss) {
        std::string varName;
        iss >> varName;
        
        if (varName.empty()) {
            std::cout << "Usage: print <variable_name>" << std::endl;
            return;
        }
        
        auto it = variables.find(varName);
        if (it != variables.end()) {
            std::cout << varName << " = " << it->second.value << " (" << it->second.type << ")" << std::endl;
        } else {
            std::cout << "Variable '" << varName << "' not found" << std::endl;
        }
    }
    
    void handleInfo(std::istringstream& iss) {
        std::string type;
        iss >> type;
        
        if (type == "breakpoints" || type == "b") {
            showBreakpoints();
        } else if (type == "variables" || type == "v") {
            showVariables();
        } else if (type == "stack" || type == "s") {
            showCallStack();
        } else if (type.empty()) {
            std::cout << "Usage: info <type>" << std::endl;
            std::cout << "Types: breakpoints, variables, stack" << std::endl;
        } else {
            std::cout << "Unknown info type: " << type << std::endl;
        }
    }
    
    void listSource() {
        if (currentFile.empty()) {
            std::cout << "No file loaded." << std::endl;
            return;
        }
        
        std::ifstream file(currentFile);
        if (!file.is_open()) {
            std::cout << "Could not open file: " << currentFile << std::endl;
            return;
        }
        
        std::string line;
        int lineNum = 1;
        int startLine = std::max(1, currentLine - 5);
        int endLine = currentLine + 5;
        
        while (std::getline(file, line) && lineNum <= endLine) {
            if (lineNum >= startLine) {
                if (lineNum == currentLine) {
                    std::cout << "-> " << lineNum << ": " << line << std::endl;
                } else {
                    std::cout << "   " << lineNum << ": " << line << std::endl;
                }
            }
            lineNum++;
        }
        
        file.close();
    }
    
    void clearBreakpoints() {
        breakpoints.clear();
        std::cout << "All breakpoints cleared" << std::endl;
    }
    
    void loadFile(std::istringstream& iss) {
        std::string filename;
        iss >> filename;
        
        if (filename.empty()) {
            std::cout << "Usage: load <filename>" << std::endl;
            return;
        }
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Could not open file: " << filename << std::endl;
            return;
        }
        
        currentFile = filename;
        currentLine = 0;
        std::cout << "Loaded file: " << filename << std::endl;
        
        // Parse file to find functions and variables
        parseFile(filename);
    }
    
    void showBreakpoints() {
        if (breakpoints.empty()) {
            std::cout << "No breakpoints set" << std::endl;
            return;
        }
        
        std::cout << "Breakpoints:" << std::endl;
        for (size_t i = 0; i < breakpoints.size(); ++i) {
            const auto& bp = breakpoints[i];
            std::cout << "  " << (i + 1) << "  " << bp.file << ":" << bp.line;
            if (!bp.condition.empty()) {
                std::cout << " if " << bp.condition;
            }
            std::cout << (bp.enabled ? " (enabled)" : " (disabled)") << std::endl;
        }
    }
    
    void showVariables() {
        if (variables.empty()) {
            std::cout << "No variables defined" << std::endl;
            return;
        }
        
        std::cout << "Variables:" << std::endl;
        for (const auto& [name, var] : variables) {
            std::cout << "  " << name << " = " << var.value << " (" << var.type << ")" << std::endl;
        }
    }
    
    void showCallStack() {
        if (callStack.empty()) {
            std::cout << "Call stack is empty" << std::endl;
            return;
        }
        
        std::cout << "Call stack:" << std::endl;
        for (size_t i = callStack.size(); i > 0; --i) {
            std::cout << "  " << (callStack.size() - i + 1) << ": " << callStack[i - 1] << std::endl;
        }
    }
    
    void checkBreakpoints() {
        for (const auto& bp : breakpoints) {
            if (bp.enabled && bp.file == currentFile && bp.line == currentLine) {
                std::cout << "Breakpoint hit at " << currentFile << ":" << currentLine << std::endl;
                showCurrentLine();
                break;
            }
        }
    }
    
    void showCurrentLine() {
        if (currentFile.empty() || currentLine == 0) return;
        
        std::ifstream file(currentFile);
        if (!file.is_open()) return;
        
        std::string line;
        int lineNum = 1;
        
        while (std::getline(file, line) && lineNum <= currentLine) {
            if (lineNum == currentLine) {
                std::cout << "Current line: " << lineNum << ": " << line << std::endl;
                break;
            }
            lineNum++;
        }
        
        file.close();
    }
    
    void simulateExecution() {
        // Simple simulation of program execution
        for (int i = currentLine; i <= 20; ++i) {
            currentLine = i;
            
            // Check for breakpoints
            checkBreakpoints();
            
            // Simulate some execution time
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Break if we hit a breakpoint
            if (std::any_of(breakpoints.begin(), breakpoints.end(),
                           [this](const Breakpoint& bp) {
                               return bp.enabled && bp.file == currentFile && bp.line == currentLine;
                           })) {
                break;
            }
        }
        
        if (currentLine > 20) {
            std::cout << "Program finished execution" << std::endl;
            currentLine = 0;
        }
    }
    
    void parseFile(const std::string& filename) {
        // Simple parsing to find functions and variables
        std::ifstream file(filename);
        if (!file.is_open()) return;
        
        variables.clear();
        callStack.clear();
        
        std::string line;
        int lineNum = 1;
        
        while (std::getline(file, line)) {
            std::string trimmed = trim(line);
            
            // Look for function definitions
            if (trimmed.find("def ") == 0) {
                size_t parenPos = trimmed.find('(');
                if (parenPos != std::string::npos) {
                    std::string funcName = trimmed.substr(4, parenPos - 4);
                    callStack.push_back(funcName + "()");
                }
            }
            
            // Look for variable assignments
            size_t assignPos = trimmed.find(" = ");
            if (assignPos != std::string::npos) {
                std::string varName = trim(trimmed.substr(0, assignPos));
                std::string value = trim(trimmed.substr(assignPos + 3));
                
                Variable var;
                var.name = varName;
                var.value = value;
                
                // Simple type inference
                if (value.find('"') != std::string::npos || value.find("'") != std::string::npos) {
                    var.type = "string";
                } else if (value.find('.') != std::string::npos) {
                    var.type = "float";
                } else if (value == "True" || value == "False") {
                    var.type = "bool";
                } else {
                    var.type = "int";
                }
                
                variables[varName] = var;
            }
            
            lineNum++;
        }
        
        file.close();
    }
    
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t");
        return str.substr(start, end - start + 1);
    }
};

int main(int argc, char* argv[]) {
    PulseDebugger debugger;
    
    if (argc > 1) {
        std::cout << "Loading file: " << argv[1] << std::endl;
        // TODO: Implement file loading from command line
    }
    
    debugger.run();
    return 0;
} 