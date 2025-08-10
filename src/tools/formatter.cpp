#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

class PulseFormatter {
public:
    PulseFormatter() : indentSize(4), maxLineLength(80) {}
    
    void setIndentSize(int size) { indentSize = size; }
    void setMaxLineLength(int length) { maxLineLength = length; }
    
    std::string format(const std::string& input) {
        std::vector<std::string> lines = splitLines(input);
        std::vector<std::string> formattedLines;
        
        int currentIndent = 0;
        bool inMultilineString = false;
        std::string multilineDelimiter;
        
        for (size_t i = 0; i < lines.size(); ++i) {
            std::string line = lines[i];
            std::string trimmed = trim(line);
            
            // Skip empty lines
            if (trimmed.empty()) {
                formattedLines.push_back("");
                continue;
            }
            
            // Handle multiline strings
            if (isMultilineStringStart(trimmed)) {
                inMultilineString = true;
                multilineDelimiter = getMultilineDelimiter(trimmed);
                formattedLines.push_back(std::string(currentIndent * indentSize, ' ') + trimmed);
                continue;
            }
            
            if (inMultilineString) {
                if (isMultilineStringEnd(trimmed, multilineDelimiter)) {
                    inMultilineString = false;
                }
                formattedLines.push_back(std::string(currentIndent * indentSize, ' ') + trimmed);
                continue;
            }
            
            // Handle comments
            if (trimmed[0] == '#') {
                formattedLines.push_back(std::string(currentIndent * indentSize, ' ') + trimmed);
                continue;
            }
            
            // Handle indentation changes
            if (isIndentIncrease(trimmed)) {
                formattedLines.push_back(std::string(currentIndent * indentSize, ' ') + trimmed);
                currentIndent++;
            } else if (isIndentDecrease(trimmed)) {
                currentIndent = std::max(0, currentIndent - 1);
                formattedLines.push_back(std::string(currentIndent * indentSize, ' ') + trimmed);
            } else {
                formattedLines.push_back(std::string(currentIndent * indentSize, ' ') + trimmed);
            }
            
            // Format long lines
            if (trimmed.length() > maxLineLength && !isComment(trimmed)) {
                formattedLines.back() = formatLongLine(trimmed, currentIndent);
            }
        }
        
        return joinLines(formattedLines);
    }
    
private:
    int indentSize;
    int maxLineLength;
    
    std::vector<std::string> splitLines(const std::string& input) {
        std::vector<std::string> lines;
        std::istringstream stream(input);
        std::string line;
        
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        
        return lines;
    }
    
    std::string joinLines(const std::vector<std::string>& lines) {
        std::string result;
        for (size_t i = 0; i < lines.size(); ++i) {
            result += lines[i];
            if (i < lines.size() - 1) {
                result += '\n';
            }
        }
        return result;
    }
    
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t");
        return str.substr(start, end - start + 1);
    }
    
    bool isIndentIncrease(const std::string& line) {
        return line.find(':') != std::string::npos && 
               !isComment(line) && 
               !isStringLiteral(line);
    }
    
    bool isIndentDecrease(const std::string& line) {
        // Check for keywords that decrease indentation
        std::vector<std::string> decreaseKeywords = {"else", "elif", "except", "finally"};
        std::string lowerLine = toLower(line);
        
        for (const auto& keyword : decreaseKeywords) {
            if (lowerLine.find(keyword) == 0) {
                return true;
            }
        }
        
        return false;
    }
    
    bool isComment(const std::string& line) {
        return line[0] == '#';
    }
    
    bool isStringLiteral(const std::string& line) {
        return (line.find('"') != std::string::npos) || (line.find("'") != std::string::npos);
    }
    
    bool isMultilineStringStart(const std::string& line) {
        return line.find("'''") != std::string::npos || line.find("\"\"\"") != std::string::npos;
    }
    
    bool isMultilineStringEnd(const std::string& line, const std::string& delimiter) {
        return line.find(delimiter) != std::string::npos;
    }
    
    std::string getMultilineDelimiter(const std::string& line) {
        if (line.find("'''") != std::string::npos) return "'''";
        if (line.find("\"\"\"") != std::string::npos) return "\"\"\"";
        return "";
    }
    
    std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    std::string formatLongLine(const std::string& line, int indent) {
        if (line.length() <= maxLineLength) return line;
        
        // Simple line breaking at operators
        std::vector<std::string> operators = {"+", "-", "*", "/", "//", "%", "**", "==", "!=", "<", "<=", ">", ">=", "and", "or"};
        
        for (const auto& op : operators) {
            size_t pos = line.find(op);
            if (pos != std::string::npos && pos > maxLineLength / 2) {
                std::string firstPart = line.substr(0, pos + op.length());
                std::string secondPart = line.substr(pos + op.length());
                
                return firstPart + "\n" + 
                       std::string((indent + 1) * indentSize, ' ') + secondPart;
            }
        }
        
        // Break at commas for function calls
        size_t commaPos = line.find_last_of(',');
        if (commaPos != std::string::npos && commaPos > maxLineLength / 2) {
            std::string firstPart = line.substr(0, commaPos + 1);
            std::string secondPart = line.substr(commaPos + 1);
            
            return firstPart + "\n" + 
                   std::string((indent + 1) * indentSize, ' ') + secondPart;
        }
        
        // Force break at max length
        size_t breakPos = maxLineLength - (indent * indentSize);
        if (breakPos > 0 && breakPos < line.length()) {
            std::string firstPart = line.substr(0, breakPos);
            std::string secondPart = line.substr(breakPos);
            
            return firstPart + "\n" + 
                   std::string((indent + 1) * indentSize, ' ') + secondPart;
        }
        
        return line;
    }
};

void showHelp() {
    std::cout << "Pulse Code Formatter (pulfmt)" << std::endl;
    std::cout << "Usage: pulfmt [options] <file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -i, --indent <size>     Set indentation size (default: 4)" << std::endl;
    std::cout << "  -l, --line-length <len> Set maximum line length (default: 80)" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  pulfmt input.pul                    # Format input.pul" << std::endl;
    std::cout << "  pulfmt -i 2 input.pul              # Use 2-space indentation" << std::endl;
    std::cout << "  pulfmt -l 100 input.pul            # Set max line length to 100" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        showHelp();
        return 1;
    }
    
    std::string inputFile;
    int indentSize = 4;
    int maxLineLength = 80;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showHelp();
            return 0;
        } else if (arg == "-i" || arg == "--indent") {
            if (i + 1 < argc) {
                indentSize = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: Indent size not specified" << std::endl;
                return 1;
            }
        } else if (arg == "-l" || arg == "--line-length") {
            if (i + 1 < argc) {
                maxLineLength = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: Line length not specified" << std::endl;
                return 1;
            }
        } else if (arg[0] != '-') {
            inputFile = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            showHelp();
            return 1;
        }
    }
    
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        showHelp();
        return 1;
    }
    
    try {
        // Read input file
        std::ifstream file(inputFile);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file: " << inputFile << std::endl;
            return 1;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Format the content
        PulseFormatter formatter;
        formatter.setIndentSize(indentSize);
        formatter.setMaxLineLength(maxLineLength);
        
        std::string formatted = formatter.format(content);
        
        // Write output
        std::ofstream outFile(inputFile);
        outFile << formatted;
        outFile.close();
        
        std::cout << "Formatted: " << inputFile << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 