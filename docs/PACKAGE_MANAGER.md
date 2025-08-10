# Pulse Package Manager & Multi-Target Compilation

The Pulse programming language now includes a comprehensive package manager and build system that supports library fetching from URLs and multi-target compilation.

## Features

### 🚀 Library Import via URLs
- Declare external libraries directly in Pulse source code
- Automatic fetching and caching of library dependencies
- Support for both manifest-based and discovery-based library fetching

### 🏗️ Multi-Target Compilation
- Build for multiple platforms from a single command
- Support for Windows, Linux, and macOS targets
- Automatic platform detection and compiler selection
- Incremental builds with caching

### 📦 Smart Package Management
- Automatic dependency resolution
- Version management and updates
- Clean local storage organization
- Network error handling and fallbacks

## Quick Start

### 1. Initialize a New Project

```bash
pulpm init
```

This creates:
- `pulse.toml` - Project configuration
- `src/main.pul` - Main source file
- `.gitignore` - Git ignore file

### 2. Declare Library Dependencies

In your `pulse.toml`:

```toml
[project]
name = "my-pulse-project"
version = "0.1.0"

[dependencies]
math_lib = "https://math-lib.pulse.dev"
web_framework = "https://web-framework.pulse.dev"

[build]
targets = ["native", "win", "linux"]
```

Or directly in your `.pul` source files:

```pul
libs = {
    math_lib == "https://math-lib.pulse.dev",
    web_framework == "https://web-framework.pulse.dev"
}

compile = native
compile = win
compile = linux
```

### 3. Install Dependencies

```bash
# Install from URL
pulpm install https://math-lib.pulse.dev

# Or fetch directly
pulpm fetch https://math-lib.pulse.dev
```

### 4. Build Your Project

```bash
# Build for all targets
pulpm build

# Build for specific target
pulpm build win
pulpm build linux
pulpm build native

# List available targets
pulpm targets
```

## Package Manager Commands

### Core Commands

```bash
pulpm init                    # Initialize new project
pulpm install <package|url>  # Install package or fetch from URL
pulpm fetch <url>            # Fetch library from URL
pulpm remove <package>       # Remove package
pulpm list                   # List installed packages
pulpm update                 # Update all packages
pulpm search <term>          # Search for packages
```

### Build Commands

```bash
pulpm build [target]         # Build project for target(s)
pulpm targets                # List available build targets
```

### Help

```bash
pulpm help                   # Show help message
```

## Library Declaration Syntax

### In Source Files

```pul
(* Declare external libraries *)
libs = {
    library_name == "https://library-url.com",
    another_lib == "https://another-lib.dev"
}

(* Declare build targets *)
compile = native
compile = win
compile = linux
compile = macos
```

### In Configuration Files

```toml
[libs]
library_name = "https://library-url.com"
another_lib = "https://another-lib.dev"

[build]
targets = ["native", "win", "linux", "macos"]
```

## Build Targets

### Supported Platforms

| Target | Platform | Compiler | Output |
|--------|----------|----------|---------|
| `native` | Current platform | Auto-detected | `pulse` or `pulse.exe` |
| `win` | Windows | MSVC (`cl`) | `pulse.exe` |
| `linux` | Linux | GCC (`g++`) | `pulse` |
| `macos` | macOS | Clang (`clang++`) | `pulse` |

### Target-Specific Features

- **Windows**: Static linking, Windows-specific optimizations
- **Linux**: GCC optimizations, Linux system calls
- **macOS**: Clang optimizations, macOS frameworks
- **Native**: Platform-optimized for current system

## Library Fetching Process

### 1. Manifest Detection
The package manager first tries to fetch:
- `pulse.toml` - TOML manifest file
- `pulse.json` - JSON manifest file

### 2. Manifest Parsing
If a manifest is found, it extracts:
- Package name and version
- Source file list
- Dependencies
- Build configuration

### 3. File Discovery
If no manifest exists, the system:
- Scans the URL for `.pul` files
- Downloads all discovered source files
- Preserves directory structure

### 4. Local Storage
Libraries are stored in:
- `~/.pulse/libs/<package_name>/` - Fetched libraries
- `~/.pulse/packages/<package_name>/` - Traditional packages

## Example Project Structure

```
my-pulse-project/
├── pulse.toml              # Project configuration
├── src/
│   ├── main.pul           # Main source file
│   └── utils.pul          # Utility functions
├── build/                  # Build outputs
│   ├── native/            # Native platform build
│   ├── win/               # Windows build
│   └── linux/             # Linux build
└── .gitignore             # Git ignore file
```

## Configuration File Reference

### pulse.toml

```toml
[project]
name = "project-name"
version = "1.0.0"
description = "Project description"

[dependencies]
# Traditional package dependencies
package_name = "1.0.0"

[libs]
# External library URLs
library_name = "https://library-url.com"

[build]
# Build targets
targets = ["native", "win", "linux"]

# Compiler flags
flags = ["-O2", "-std=c++20"]

# Output configuration
output_name = "my-program"
```

## Error Handling

### Network Errors
- Automatic retry with exponential backoff
- Fallback to file discovery if manifest fails
- Clear error messages for debugging

### Build Errors
- Target-specific error reporting
- Compilation failure details
- Platform compatibility warnings

### Dependency Errors
- Missing library notifications
- Version conflict resolution
- Circular dependency detection

## Best Practices

### Library Development
1. **Always include a manifest** (`pulse.toml` or `pulse.json`)
2. **Use semantic versioning** for releases
3. **Document dependencies** clearly
4. **Test on multiple platforms**

### Project Configuration
1. **Specify build targets** explicitly
2. **Use relative URLs** when possible
3. **Pin library versions** for stability
4. **Include comprehensive examples**

### Build Optimization
1. **Use incremental builds** for development
2. **Cache build artifacts** locally
3. **Parallel compilation** for multiple targets
4. **Platform-specific optimizations**

## Troubleshooting

### Common Issues

**Library not found:**
```bash
# Check if URL is accessible
curl -I https://library-url.com

# Verify manifest exists
curl https://library-url.com/pulse.toml
```

**Build failures:**
```bash
# Check compiler availability
g++ --version
cl --version

# Verify platform compatibility
pulpm targets
```

**Network issues:**
```bash
# Check network connectivity
ping library-url.com

# Use alternative URLs or mirrors
pulpm fetch https://alternative-url.com
```

### Debug Mode

Enable verbose output:
```bash
export PULSE_DEBUG=1
pulpm build
```

## Advanced Features

### Custom Build Targets
```toml
[build.targets.custom]
name = "embedded"
platform = "arm"
compiler = "arm-none-eabi-g++"
flags = ["-mcpu=cortex-m4", "-mthumb"]
output_name = "pulse-embedded"
```

### Library Versioning
```toml
[libs]
math_lib = "https://math-lib.pulse.dev#v2.1.0"
web_framework = "https://web-framework.pulse.dev#latest"
```

### Build Hooks
```toml
[build.hooks]
pre_build = "scripts/prepare.sh"
post_build = "scripts/package.sh"
```

## Contributing

The package manager is designed to be extensible. Key areas for contribution:

1. **Additional manifest formats** (YAML, XML)
2. **More build targets** (Android, iOS, WebAssembly)
3. **Advanced dependency resolution** algorithms
4. **Package repository** integration
5. **CI/CD pipeline** support

## License

This package manager is part of the Pulse programming language project and follows the same license terms. 