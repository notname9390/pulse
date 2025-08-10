#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <algorithm>
#include <sstream>
#include <regex>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace fs = std::filesystem;

// Forward declarations
class HTTPClient;
class ManifestParser;
class BuildSystem;

// Package structure
struct Package {
    std::string name;
    std::string version;
    std::string description;
    std::string source_url;
    std::vector<std::string> dependencies;
    std::map<std::string, std::string> targets;
    std::vector<std::string> source_files;
    std::string manifest_path;
    std::chrono::system_clock::time_point last_updated;
};

// Library declaration structure
struct LibraryDeclaration {
    std::string name;
    std::string url;
    std::string version;
};

// Build target structure
struct BuildTarget {
    std::string name;
    std::string platform;
    std::string compiler;
    std::vector<std::string> flags;
    std::string output_name;
    bool enabled;
};

// HTTP Client for fetching libraries
class HTTPClient {
private:
    int sockfd;
    bool initialized;
    
public:
    HTTPClient() : sockfd(-1), initialized(false) {
#ifdef _WIN32
        WSADATA wsaData;
        initialized = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#endif
    }
    
    ~HTTPClient() {
        if (sockfd != -1) {
#ifdef _WIN32
            closesocket(sockfd);
        }
        if (initialized) {
            WSACleanup();
        }
#else
            close(sockfd);
        }
#endif
    }
    
    std::string fetchURL(const std::string& url) {
        try {
            // Parse URL
            auto [host, path] = parseURL(url);
            if (host.empty()) {
                throw std::runtime_error("Invalid URL format");
            }
            
            // Create socket
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd == -1) {
                throw std::runtime_error("Failed to create socket");
            }
            
            // Resolve hostname
            struct hostent* server = gethostbyname(host.c_str());
            if (server == nullptr) {
                throw std::runtime_error("Failed to resolve hostname: " + host);
            }
            
            // Setup connection
            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(80);
            memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
            
            // Connect
            if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                throw std::runtime_error("Failed to connect to server");
            }
            
            // Send HTTP request
            std::string request = "GET " + path + " HTTP/1.1\r\n"
                                "Host: " + host + "\r\n"
                                "User-Agent: Pulse-Package-Manager/1.0\r\n"
                                "Connection: close\r\n\r\n";
            
            if (send(sockfd, request.c_str(), request.length(), 0) < 0) {
                throw std::runtime_error("Failed to send HTTP request");
            }
            
            // Receive response
            std::string response;
            char buffer[4096];
            int bytes_received;
            
            while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
                buffer[bytes_received] = '\0';
                response += buffer;
            }
            
            // Close socket
#ifdef _WIN32
            closesocket(sockfd);
#else
            close(sockfd);
#endif
            sockfd = -1;
            
            // Extract body from response
            size_t body_start = response.find("\r\n\r\n");
            if (body_start != std::string::npos) {
                return response.substr(body_start + 4);
            }
            
            return response;
            
        } catch (const std::exception& e) {
            if (sockfd != -1) {
#ifdef _WIN32
                closesocket(sockfd);
#else
                close(sockfd);
#endif
                sockfd = -1;
            }
            throw;
        }
    }
    
    std::vector<std::string> listDirectory(const std::string& url) {
        std::string response = fetchURL(url);
        std::vector<std::string> files;
        
        // Simple HTML parsing to extract links
        std::regex link_regex("<a[^>]*href=[\"']([^\"']+)[\"'][^>]*>");
        std::sregex_iterator iter(response.begin(), response.end(), link_regex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::string link = (*iter)[1];
            if (link.find(".pul") != std::string::npos || 
                link.find(".toml") != std::string::npos ||
                link.find(".json") != std::string::npos) {
                files.push_back(link);
            }
        }
        
        return files;
    }
    
private:
    std::pair<std::string, std::string> parseURL(const std::string& url) {
        std::regex url_regex("https?://([^/]+)(/.*)?");
        std::smatch match;
        
        if (std::regex_match(url, match, url_regex)) {
            std::string host = match[1].str();
            std::string path = (match[2].matched && !match[2].str().empty()) ? match[2].str() : "/";
            return {host, path};
        }
        
        return {"", ""};
    }
};

// Manifest Parser for pulse.toml and pulse.json
class ManifestParser {
public:
    static Package parseManifest(const std::string& content, const std::string& source_url) {
        Package pkg;
        pkg.source_url = source_url;
        
        // Try parsing as TOML first
        if (parseTOML(content, pkg)) {
            return pkg;
        }
        
        // Try parsing as JSON
        if (parseJSON(content, pkg)) {
            return pkg;
        }
        
        // Fallback to basic parsing
        parseBasic(content, pkg);
        return pkg;
    }
    
private:
    static bool parseTOML(const std::string& content, Package& pkg) {
        try {
            // Simple TOML-like parsing
            std::istringstream iss(content);
            std::string line;
            
            while (std::getline(iss, line)) {
                line = trim(line);
                if (line.empty() || line[0] == '#') continue;
                
                if (line[0] == '[') {
                    // Section header
                    continue;
                }
                
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = trim(line.substr(0, eq_pos));
                    std::string value = trim(line.substr(eq_pos + 1));
                    
                    // Remove quotes
                    if (value.length() >= 2 && value[0] == '"' && value[value.length()-1] == '"') {
                        value = value.substr(1, value.length() - 2);
                    }
                    
                    if (key == "name") pkg.name = value;
                    else if (key == "version") pkg.version = value;
                    else if (key == "description") pkg.description = value;
                    else if (key == "source_url") pkg.source_url = value;
                }
            }
            
            return !pkg.name.empty();
        } catch (...) {
            return false;
        }
    }
    
    static bool parseJSON(const std::string& content, Package& pkg) {
        try {
            // Simple JSON-like parsing
            std::istringstream iss(content);
            std::string line;
            
            while (std::getline(iss, line)) {
                line = trim(line);
                if (line.empty()) continue;
                
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    std::string key = trim(line.substr(0, colon_pos));
                    std::string value = trim(line.substr(colon_pos + 1));
                    
                    // Remove quotes and commas
                    if (value.length() >= 2 && value[0] == '"' && value[value.length()-1] == '"') {
                        value = value.substr(1, value.length() - 2);
                    }
                    if (value.back() == ',') {
                        value = value.substr(0, value.length() - 1);
                    }
                    
                    if (key == "\"name\"") pkg.name = value;
                    else if (key == "\"version\"") pkg.version = value;
                    else if (key == "\"description\"") pkg.description = value;
                    else if (key == "\"source_url\"") pkg.source_url = value;
                }
            }
            
            return !pkg.name.empty();
        } catch (...) {
            return false;
        }
    }
    
    static void parseBasic(const std::string& content, Package& pkg) {
        // Extract basic information from content
        (void)content; // Suppress unused parameter warning
        if (pkg.name.empty()) {
            pkg.name = "unknown";
        }
        if (pkg.version.empty()) {
            pkg.version = "1.0.0";
        }
        if (pkg.description.empty()) {
            pkg.description = "Package from " + pkg.source_url;
        }
    }
    
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
};

// Build System for multi-target compilation
class BuildSystem {
private:
    fs::path projectDir;
    fs::path buildDir;
    std::map<std::string, BuildTarget> targets;
    
public:
    BuildSystem(const fs::path& project_dir) : projectDir(project_dir) {
        buildDir = projectDir / "build";
        fs::create_directories(buildDir);
        initializeTargets();
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
        
        std::cout << "Building for target: " << target_name << std::endl;
        
        // Create target-specific build directory
        fs::path target_build_dir = buildDir / target_name;
        fs::create_directories(target_build_dir);
        
        // Compile source files
        std::vector<std::string> source_files = findSourceFiles();
        std::vector<std::string> object_files;
        
        for (const auto& source_file : source_files) {
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
        }
    }
    
private:
    void initializeTargets() {
        // Windows target
        targets["win"] = {
            "win", "windows", "cl", {"-O2", "-std=c++20"}, "pulse.exe", true
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
    }
    
    std::vector<std::string> findSourceFiles() {
        std::vector<std::string> source_files;
        fs::path src_dir = projectDir / "src";
        
        if (fs::exists(src_dir)) {
            for (const auto& entry : fs::recursive_directory_iterator(src_dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".pul") {
                    source_files.push_back(entry.path().string());
                }
            }
        }
        
        return source_files;
    }
    
    std::string compileSourceFile(const std::string& source_file, const BuildTarget& target, const fs::path& build_dir) {
        fs::path source_path(source_file);
        std::string object_file = (build_dir / source_path.stem()).string() + ".o";
        
        std::string compile_cmd = target.compiler + " -c " + source_file + " -o " + object_file;
        
        // Add compiler flags
        for (const auto& flag : target.flags) {
            compile_cmd += " " + flag;
        }
        
        std::cout << "Compiling: " << source_path.filename().string() << std::endl;
        
        // Execute compilation command
        int result = system(compile_cmd.c_str());
        if (result != 0) {
            std::cerr << "Compilation failed for: " << source_file << std::endl;
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
        
        std::cout << "Linking: " << target.output_name << std::endl;
        
        // Execute linking command
        int result = system(link_cmd.c_str());
        if (result != 0) {
            std::cerr << "Linking failed for target: " << target.name << std::endl;
        }
    }
};

// Enhanced Package Manager
class PackageManager {
private:
    fs::path homeDir;
    fs::path pulseDir;
    fs::path packagesDir;
    fs::path cacheDir;
    fs::path libsDir;
    std::unique_ptr<HTTPClient> httpClient;
    std::unique_ptr<BuildSystem> buildSystem;
    
public:
    PackageManager() {
        // Initialize directories
        homeDir = std::getenv("HOME") ? std::getenv("HOME") : ".";
        pulseDir = homeDir / ".pulse";
        packagesDir = pulseDir / "packages";
        cacheDir = pulseDir / "cache";
        libsDir = pulseDir / "libs";
        
        // Create directories
        fs::create_directories(pulseDir);
        fs::create_directories(packagesDir);
        fs::create_directories(cacheDir);
        fs::create_directories(libsDir);
        
        // Initialize components
        httpClient = std::make_unique<HTTPClient>();
        buildSystem = std::make_unique<BuildSystem>(fs::current_path());
    }
    
    void run(int argc, char* argv[]) {
        if (argc < 2) {
            showHelp();
            return;
        }
        
        std::string command = argv[1];
        
        if (command == "install") {
            if (argc < 3) {
                std::cerr << "Error: Package name or URL required for install command" << std::endl;
                return;
            }
            installPackage(argv[2]);
        } else if (command == "remove") {
            if (argc < 3) {
                std::cerr << "Error: Package name required for remove command" << std::endl;
                return;
            }
            removePackage(argv[2]);
        } else if (command == "list") {
            listPackages();
        } else if (command == "search") {
            if (argc < 3) {
                std::cerr << "Error: Search term required" << std::endl;
                return;
            }
            searchPackages(argv[2]);
        } else if (command == "update") {
            updatePackages();
        } else if (command == "init") {
            initProject();
        } else if (command == "build") {
            if (argc < 3) {
                buildSystem->buildAllTargets();
            } else {
                buildSystem->buildTarget(argv[2]);
            }
        } else if (command == "targets") {
            buildSystem->listTargets();
        } else if (command == "fetch") {
            if (argc < 3) {
                std::cerr << "Error: URL required for fetch command" << std::endl;
                return;
            }
            fetchLibrary(argv[2]);
        } else if (command == "help") {
            showHelp();
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            showHelp();
        }
    }
    
private:
    void showHelp() {
        std::cout << "Pulse Package Manager (pulpm)" << std::endl;
        std::cout << "Usage: pulpm <command> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  install <package|url>  Install a package or fetch from URL" << std::endl;
        std::cout << "  remove <package>        Remove a package" << std::endl;
        std::cout << "  list                    List installed packages" << std::endl;
        std::cout << "  search <term>           Search for packages" << std::endl;
        std::cout << "  update                  Update all packages" << std::endl;
        std::cout << "  init                    Initialize a new project" << std::endl;
        std::cout << "  build [target]          Build project for target(s)" << std::endl;
        std::cout << "  targets                 List available build targets" << std::endl;
        std::cout << "  fetch <url>             Fetch library from URL" << std::endl;
        std::cout << "  help                    Show this help message" << std::endl;
    }
    
    void installPackage(const std::string& package_spec) {
        // Check if it's a URL
        if (package_spec.find("http://") == 0 || package_spec.find("https://") == 0) {
            fetchLibrary(package_spec);
        } else {
            // Traditional package installation
            std::cout << "Installing package: " << package_spec << std::endl;
            
            if (isPackageInstalled(package_spec)) {
                std::cout << "Package " << package_spec << " is already installed" << std::endl;
                return;
            }
            
            Package pkg;
            pkg.name = package_spec;
            pkg.version = "1.0.0";
            pkg.description = "Placeholder package for " + package_spec;
            
            fs::path packageDir = packagesDir / package_spec;
            fs::create_directories(packageDir);
            createPackageManifest(packageDir, pkg);
            
            std::cout << "Package " << package_spec << " installed successfully" << std::endl;
        }
    }
    
    void fetchLibrary(const std::string& url) {
        std::cout << "Fetching library from: " << url << std::endl;
        
        try {
            // Try to fetch manifest first
            std::string manifest_content = httpClient->fetchURL(url + "/pulse.toml");
            Package pkg = ManifestParser::parseManifest(manifest_content, url);
            
            if (pkg.name.empty()) {
                // Try JSON manifest
                manifest_content = httpClient->fetchURL(url + "/pulse.json");
                pkg = ManifestParser::parseManifest(manifest_content, url);
            }
            
            if (pkg.name.empty()) {
                // No manifest found, try to discover files
                discoverAndDownloadFiles(url);
                return;
            }
            
            // Download package with manifest
            downloadPackageWithManifest(pkg);
            
        } catch (const std::exception& e) {
            std::cerr << "Failed to fetch library: " << e.what() << std::endl;
            
            // Fallback: try to discover and download files
            try {
                discoverAndDownloadFiles(url);
            } catch (const std::exception& e2) {
                std::cerr << "Failed to discover files: " << e2.what() << std::endl;
            }
        }
    }
    
    void discoverAndDownloadFiles(const std::string& url) {
        std::cout << "No manifest found, discovering files..." << std::endl;
        
        try {
            std::vector<std::string> files = httpClient->listDirectory(url);
            
            if (files.empty()) {
                std::cout << "No source files found" << std::endl;
                return;
            }
            
            // Create package directory
            std::string package_name = extractPackageNameFromURL(url);
            fs::path package_dir = libsDir / package_name;
            fs::create_directories(package_dir);
            
            // Download each file
            for (const auto& file : files) {
                std::string file_url = url + "/" + file;
                std::string file_content = httpClient->fetchURL(file_url);
                
                fs::path file_path = package_dir / file;
                fs::create_directories(file_path.parent_path());
                
                std::ofstream out_file(file_path);
                out_file << file_content;
                out_file.close();
                
                std::cout << "Downloaded: " << file << std::endl;
            }
            
            std::cout << "Library downloaded successfully to: " << package_dir << std::endl;
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to discover files: " + std::string(e.what()));
        }
    }
    
    void downloadPackageWithManifest(const Package& pkg) {
        std::cout << "Downloading package: " << pkg.name << " v" << pkg.version << std::endl;
        
        fs::path package_dir = libsDir / pkg.name;
        fs::create_directories(package_dir);
        
        // Download source files
        for (const auto& source_file : pkg.source_files) {
            std::string file_url = pkg.source_url + "/" + source_file;
            std::string file_content = httpClient->fetchURL(file_url);
            
            fs::path file_path = package_dir / source_file;
            fs::create_directories(file_path.parent_path());
            
            std::ofstream out_file(file_path);
            out_file << file_content;
            out_file.close();
            
            std::cout << "Downloaded: " << source_file << std::endl;
        }
        
        // Save package info
        savePackageInfo(package_dir, pkg);
        
        std::cout << "Package " << pkg.name << " downloaded successfully" << std::endl;
    }
    
    std::string extractPackageNameFromURL(const std::string& url) {
        size_t last_slash = url.find_last_of('/');
        if (last_slash != std::string::npos && last_slash < url.length() - 1) {
            return url.substr(last_slash + 1);
        }
        return "unknown";
    }
    
    void savePackageInfo(const fs::path& package_dir, const Package& pkg) {
        fs::path info_file = package_dir / "package.info";
        std::ofstream out_file(info_file);
        
        out_file << "Name: " << pkg.name << std::endl;
        out_file << "Version: " << pkg.version << std::endl;
        out_file << "Description: " << pkg.description << std::endl;
        out_file << "Source URL: " << pkg.source_url << std::endl;
        out_file << "Last Updated: " << std::chrono::system_clock::to_time_t(pkg.last_updated) << std::endl;
        
        out_file.close();
    }
    
    void removePackage(const std::string& packageName) {
        std::cout << "Removing package: " << packageName << std::endl;
        
        if (!isPackageInstalled(packageName)) {
            std::cout << "Package " << packageName << " is not installed" << std::endl;
            return;
        }
        
        fs::path packageDir = packagesDir / packageName;
        fs::path libDir = libsDir / packageName;
        
        if (fs::exists(packageDir)) {
            fs::remove_all(packageDir);
        }
        if (fs::exists(libDir)) {
            fs::remove_all(libDir);
        }
        
        std::cout << "Package " << packageName << " removed successfully" << std::endl;
    }
    
    void listPackages() {
        std::cout << "Installed packages:" << std::endl;
        
        // List traditional packages
        if (fs::exists(packagesDir) && !fs::is_empty(packagesDir)) {
            for (const auto& entry : fs::directory_iterator(packagesDir)) {
                if (entry.is_directory()) {
                    std::string packageName = entry.path().filename().string();
                    std::cout << "  " << packageName << " (traditional)" << std::endl;
                }
            }
        }
        
        // List fetched libraries
        if (fs::exists(libsDir) && !fs::is_empty(libsDir)) {
            for (const auto& entry : fs::directory_iterator(libsDir)) {
                if (entry.is_directory()) {
                    std::string packageName = entry.path().filename().string();
                    std::cout << "  " << packageName << " (fetched)" << std::endl;
                }
            }
        }
        
        if (fs::is_empty(packagesDir) && fs::is_empty(libsDir)) {
            std::cout << "  No packages installed" << std::endl;
        }
    }
    
    void searchPackages(const std::string& term) {
        std::cout << "Searching for packages containing: " << term << std::endl;
        std::cout << "Note: Package search not yet implemented" << std::endl;
    }
    
    void updatePackages() {
        std::cout << "Updating packages..." << std::endl;
        std::cout << "Note: Package updates not yet implemented" << std::endl;
    }
    
    void initProject() {
        std::cout << "Initializing new Pulse project..." << std::endl;
        
        // Create pulse.toml
        std::ofstream pulseToml("pulse.toml");
        pulseToml << "[project]\n";
        pulseToml << "name = \"my-pulse-project\"\n";
        pulseToml << "version = \"0.1.0\"\n";
        pulseToml << "description = \"A Pulse programming language project\"\n";
        pulseToml << "\n";
        pulseToml << "[dependencies]\n";
        pulseToml << "# Add your library dependencies here\n";
        pulseToml << "# example = \"https://example.pages.dev\"\n";
        pulseToml << "\n";
        pulseToml << "[build]\n";
        pulseToml << "targets = [\"native\", \"win\", \"linux\"]\n";
        pulseToml << "\n";
        pulseToml << "[libs]\n";
        pulseToml << "# Declare external libraries\n";
        pulseToml << "# example = \"https://example.pages.dev\"\n";
        pulseToml.close();
        
        // Create src directory
        fs::create_directories("src");
        
        // Create main.pul
        std::ofstream mainPul("src/main.pul");
        mainPul << "# Main entry point for the Pulse program\n";
        mainPul << "def main():\n";
        mainPul << "    out(\"Hello, Pulse!\")\n";
        mainPul << "\n";
        mainPul << "if __name__ == \"__main__\":\n";
        mainPul << "    main()\n";
        mainPul.close();
        
        // Create .gitignore
        std::ofstream gitignore(".gitignore");
        gitignore << "# Build artifacts\n";
        gitignore << "build/\n";
        gitignore << "*.o\n";
        gitignore << "*.so\n";
        gitignore << "*.dylib\n";
        gitignore << "*.exe\n";
        gitignore << "\n";
        gitignore << "# Dependencies\n";
        gitignore << ".pulse/\n";
        gitignore << "\n";
        gitignore << "# IDE files\n";
        gitignore << ".vscode/\n";
        gitignore << ".idea/\n";
        gitignore.close();
        
        std::cout << "Project initialized successfully!" << std::endl;
        std::cout << "Edit src/main.pul to get started" << std::endl;
        std::cout << "Use 'pulpm build' to compile your project" << std::endl;
    }
    
    bool isPackageInstalled(const std::string& packageName) {
        return fs::exists(packagesDir / packageName) || fs::exists(libsDir / packageName);
    }
    
    void createPackageManifest(const fs::path& packageDir, const Package& pkg) {
        std::ofstream manifest(packageDir / "pulse.toml");
        manifest << "[package]\n";
        manifest << "name = \"" << pkg.name << "\"\n";
        manifest << "version = \"" << pkg.version << "\"\n";
        manifest << "description = \"" << pkg.description << "\"\n";
        manifest << "\n";
        manifest << "[dependencies]\n";
        for (const auto& dep : pkg.dependencies) {
            manifest << dep << " = \"*\"\n";
        }
        manifest.close();
    }
};

int main(int argc, char* argv[]) {
    try {
        PackageManager pm;
        pm.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 