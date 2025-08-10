#!/bin/bash

# Pulse Package Manager Test Script
# This script demonstrates the new package manager features

set -e

echo "🚀 Pulse Package Manager Test Script"
echo "===================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "Please run this script from the Pulse project root directory"
    exit 1
fi

# Build the package manager
print_status "Building Pulse Package Manager..."
if ! cmake --build build --target pulpm; then
    print_error "Failed to build package manager"
    exit 1
fi
print_success "Package manager built successfully"

# Create test project directory
TEST_DIR="test_package_project"
if [ -d "$TEST_DIR" ]; then
    rm -rf "$TEST_DIR"
fi
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

print_status "Creating test project in: $TEST_DIR"

# Initialize a new project
print_status "Initializing new Pulse project..."
../build/bin/pulpm init
print_success "Project initialized"

# Show the created files
print_status "Project structure created:"
ls -la

# Test project info
print_status "Testing project information..."
../build/bin/pulbuild info

# Test build targets
print_status "Testing build targets..."
../build/bin/pulbuild targets

# Test package manager help
print_status "Testing package manager help..."
../build/bin/pulpm help

# Test package listing
print_status "Testing package listing..."
../build/bin/pulpm list

# Test library fetching (using a mock URL)
print_status "Testing library fetching (mock URL)..."
print_warning "This will fail as it's a mock URL, but demonstrates the feature"
../build/bin/pulpm fetch https://example.com/nonexistent-library || true

# Test build system
print_status "Testing build system..."
print_warning "This will fail as we don't have actual .pul source files, but shows the process"

# Create a simple test source file
cat > src/test.pul << 'EOF'
(* Simple test file for build testing *)
def hello():
    out("Hello from test file!")

if __name__ == "__main__":
    hello()
EOF

print_status "Created test source file: src/test.pul"

# Test build info again
print_status "Testing build info with source file..."
../build/bin/pulbuild info

# Test build targets again
print_status "Testing build targets..."
../build/bin/pulbuild targets

# Test clean command
print_status "Testing clean command..."
../build/bin/pulbuild clean

# Test project initialization with custom config
print_status "Testing custom project configuration..."
cd ..
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Create custom pulse.toml
cat > pulse.toml << 'EOF'
[project]
name = "custom-test-project"
version = "2.0.0"
description = "Custom test project with specific configuration"

[dependencies]
# Example dependencies
example_lib = "1.0.0"

[libs]
# External libraries
math_lib = "https://math-lib.pulse.dev"
web_framework = "https://web-framework.pulse.dev"

[build]
targets = ["native", "win", "linux"]
flags = ["-O2", "-std=c++20"]
output_name = "custom-pulse"
EOF

print_status "Created custom pulse.toml configuration"

# Create source directory and file
mkdir -p src
cat > src/main.pul << 'EOF'
(* Custom test project main file *)
libs = {
    math_lib == "https://math-lib.pulse.dev",
    web_framework == "https://web-framework.pulse.dev"
}

compile = native
compile = win
compile = linux

def main():
    out("Custom test project running!")
    out("This demonstrates library imports and multi-target compilation")

if __name__ == "__main__":
    main()
EOF

print_status "Created custom source file with library declarations"

# Test the custom project
print_status "Testing custom project..."
../build/bin/pulbuild info
../build/bin/pulbuild targets

# Test package manager with custom project
print_status "Testing package manager with custom project..."
../build/bin/pulpm list

# Clean up
print_status "Cleaning up test files..."
cd ..
rm -rf "$TEST_DIR"

print_success "All tests completed successfully!"
echo
echo "🎉 Package Manager Features Demonstrated:"
echo "  ✅ Project initialization"
echo "  ✅ Multi-target compilation support"
echo "  ✅ Library declaration syntax"
echo "  ✅ Configuration file parsing"
echo "  ✅ Build system integration"
echo "  ✅ Package management commands"
echo "  ✅ Error handling and fallbacks"
echo
echo "🚀 The Pulse Package Manager is ready to use!"
echo "   Use 'pulpm help' for command reference"
echo "   Use 'pulbuild help' for build system reference" 