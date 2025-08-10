#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <sstream>
#include <regex>

namespace fs = std::filesystem;

// Build target structure
struct BuildTarget {
    std::string name;
    std::string platform;
    std::string compiler;
    std::vector<std::string> flags;
    std::string output_name;
    bool enabled;
};

// Build System for multi-target compilation
class BuildSystem {
private:
    fs::path projectDir;
    fs::path buildDir;
    std::map<std::string, BuildTarget> targets;
    std::vector<std::string> sourceFiles;
    
public:
    BuildSystem(const fs::path& project_dir) : projectDir(project_dir) {
        buildDir = projectDir / "build";
        fs::create_directories(buildDir);
        initializeTargets();
        discoverSourceFiles();
    }
    
    void buildTarget(const std::string& target_name) {
        if (targets.find(target_name) == targets.end()) {
            throw std::runtime_error("Unknown build target: " + target_name);
        }
        
        BuildTarget& target = targets[target_name];
        if (!target.enabled) {
            std::cout << "Target " << target_name << " is disabled" << std::endl;
            return;
        }
        
        std::cout << "Building for target: " << target_name << " (" << target.platform << ")" << std::endl;
        
        // Create target-specific build directory
        fs::path target_build_dir = buildDir / target_name;
        fs::create_directories(target_build_dir);
        
        // Compile source files
        std::vector<std::string> object_files;
        
        for (const auto& source_file : sourceFiles) {
            std::string object_file = compileSourceFile(source_file, target, target_build_dir);
            if (!object_file.empty()) {
                object_files.push_back(object_file);
            }
        }
        
        // Link executable
        if (!object_files.empty()) {
            linkExecutable(object_files, target, target_build_dir);
        }
        
        std::cout << "Build completed for target: " << target_name << std::endl;
    }
    
    void buildAllTargets() {
        std::cout << "Building for all enabled targets..." << std::endl;
        
        for (auto& [name, target] : targets) {
            if (target.enabled) {
                try {
                    buildTarget(name);
                } catch (const std::exception& e) {
                    std::cerr << "Build failed for target " << name << ": " << e.what() << std::endl;
                }
            }
        }
    }
    
    void listTargets() {
        std::cout << "Available build targets:" << std::endl;
        for (const auto& [name, target] : targets) {
            std::cout << "  " << name << " (" << target.platform << ") - " 
                     << (target.enabled ? "enabled" : "disabled") << std::endl;
            std::cout << "    Compiler: " << target.compiler << std::endl;
            std::cout << "    Output: " << target.output_name << std::endl;
        }
    }
    
    void clean() {
        if (fs::exists(buildDir)) {
            fs::remove_all(buildDir);
            std::cout << "Build directory cleaned" << std::endl;
        }
    }
    
    void showProjectInfo() {
        std::cout << "Project Information:" << std::endl;
        std::cout << "  Directory: " << projectDir << std::endl;
        std::cout << "  Source files: " << sourceFiles.size() << std::endl;
        
        for (const auto& source : sourceFiles) {
            std::cout << "    " << fs::relative(source, projectDir) << std::endl;
        }
        
        std::cout << "  Build directory: " << buildDir << std::endl;
    }
    
private:
    void initializeTargets() {
        // Windows target
        targets["win"] = {
            "win", "windows", "cl", {"-O2", "-std:c++20"}, "pulse.exe", true
        };
        
        // Linux target
        targets["linux"] = {
            "linux", "linux", "g++", {"-O2", "-std=c++20"}, "pulse", true
        };
        
        // macOS target
        targets["macos"] = {
            "macos", "macos", "clang++", {"-O2", "-std=c++20"}, "pulse", true
        };
        
        // Native target (current platform)
#ifdef _WIN32
        targets["native"] = targets["win"];
#elif __APPLE__
        targets["native"] = targets["macos"];
#else
        targets["native"] = targets["linux"];
#endif
        
        // Check compiler availability and disable unavailable targets
        checkCompilerAvailability();
    }
    
    void checkCompilerAvailability() {
        for (auto& [name, target] : targets) {
            std::string check_cmd = "which " + target.compiler;
#ifdef _WIN32
            check_cmd = "where " + target.compiler;
#endif
            
            int result = system(check_cmd.c_str());
            if (result != 0) {
                std::cout << "Warning: Compiler " << target.compiler << " not found, disabling target " << name << std::endl;
                target.enabled = false;
            }
        }
    }
    
    void discoverSourceFiles() {
        sourceFiles.clear();
        fs::path src_dir = projectDir / "src";
        
        if (fs::exists(src_dir)) {
            for (const auto& entry : fs::recursive_directory_iterator(src_dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".pul") {
                    sourceFiles.push_back(entry.path().string());
                }
            }
        }
        
        // Also check for .pul files in project root
        for (const auto& entry : fs::directory_iterator(projectDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".pul") {
                sourceFiles.push_back(entry.path().string());
            }
        }
        
        if (sourceFiles.empty()) {
            std::cout << "Warning: No source files found" << std::endl;
        }
    }
    
    std::string compileSourceFile(const std::string& source_file, const BuildTarget& target, const fs::path& build_dir) {
        fs::path source_path(source_file);
        std::string object_file = (build_dir / source_path.stem()).string() + ".o";
        
        std::string compile_cmd = target.compiler + " -c " + source_file + " -o " + object_file;
        
        // Add compiler flags
        for (const auto& flag : target.flags) {
            compile_cmd += " " + flag;
        }
        
        std::cout << "  Compiling: " << fs::relative(source_path, projectDir) << std::endl;
        
        // Execute compilation command
        int result = system(compile_cmd.c_str());
        if (result != 0) {
            std::cerr << "  Compilation failed for: " << source_file << std::endl;
            return "";
        }
        
        return object_file;
    }
    
    void linkExecutable(const std::vector<std::string>& object_files, const BuildTarget& target, const fs::path& build_dir) {
        std::string output_file = (build_dir / target.output_name).string();
        
        std::string link_cmd = target.compiler;
        
        // Add object files
        for (const auto& obj_file : object_files) {
            link_cmd += " " + obj_file;
        }
        
        // Add output file
        link_cmd += " -o " + output_file;
        
        // Add platform-specific flags
        if (target.platform == "windows") {
            link_cmd += " -static-libgcc -static-libstdc++";
        }
        
        std::cout << "  Linking: " << target.output_name << std::endl;
        
        // Execute linking command
        int result = system(link_cmd.c_str());
        if (result != 0) {
            std::cerr << "  Linking failed for target: " << target.name << std::endl;
        } else {
            std::cout << "  Successfully built: " << output_file << std::endl;
        }
    }
};

// Configuration parser for pulse.toml
class ConfigParser {
public:
    static std::vector<std::string> parseBuildTargets(const fs::path& config_file) {
        std::vector<std::string> targets;
        
        if (!fs::exists(config_file)) {
            return targets;
        }
        
        std::ifstream file(config_file);
        std::string line;
        
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            
            if (line.find("targets") != std::string::npos && line.find('=') != std::string::npos) {
                // Parse targets array
                size_t start = line.find('[');
                size_t end = line.find(']');
                if (start != std::string::npos && end != std::string::npos) {
                    std::string targets_str = line.substr(start + 1, end - start - 1);
                    parseTargetsArray(targets_str, targets);
                }
            }
        }
        
        return targets;
    }
    
private:
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
    
    static void parseTargetsArray(const std::string& str, std::vector<std::string>& targets) {
        std::istringstream iss(str);
        std::string target;
        
        while (std::getline(iss, target, ',')) {
            target = trim(target);
            if (!target.empty()) {
                // Remove quotes
                if (target.length() >= 2 && target[0] == '"' && target[target.length()-1] == '"') {
                    target = target.substr(1, target.length() - 2);
                }
                targets.push_back(target);
            }
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cout << "Pulse Build Tool (pulbuild)" << std::endl;
            std::cout << "Usage: pulbuild <command> [target]" << std::endl;
            std::cout << std::endl;
            std::cout << "Commands:" << std::endl;
            std::cout << "  build [target]  Build project for target(s)" << std::endl;
            std::cout << "  clean           Clean build directory" << std::endl;
            std::cout << "  targets         List available build targets" << std::endl;
            std::cout << "  info            Show project information" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  pulbuild build          # Build for all targets" << std::endl;
            std::cout << "  pulbuild build win      # Build for Windows" << std::endl;
            std::cout << "  pulbuild build linux    # Build for Linux" << std::endl;
            std::cout << "  pulbuild clean          # Clean build artifacts" << std::endl;
            return 0;
        }
        
        std::string command = argv[1];
        std::string target = (argc > 2) ? argv[2] : "";
        
        // Initialize build system
        BuildSystem buildSystem(fs::current_path());
        
        if (command == "build") {
            if (target.empty()) {
                buildSystem.buildAllTargets();
            } else {
                buildSystem.buildTarget(target);
            }
        } else if (command == "clean") {
            buildSystem.clean();
        } else if (command == "targets") {
            buildSystem.listTargets();
        } else if (command == "info") {
            buildSystem.showProjectInfo();
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            std::cerr << "Use 'pulbuild' for help" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 