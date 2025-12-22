# Installation

## Building from Source

Onager is built as a DuckDB extension. Clone and build:

```bash
git clone --recursive https://github.com/CogitatorTech/onager.git
cd onager
make release
```

This creates a DuckDB binary with Onager built-in at `./build/release/duckdb`.

## Requirements

- **Rust** 1.70+ (for building the graph algorithms)
- **CMake** 3.15+
- **C++ compiler** with C++17 support
- **Make**

### Linux/macOS

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Ubuntu/Debian
sudo apt install build-essential cmake

# macOS
xcode-select --install
brew install cmake
```

## Verifying Installation

```bash
./build/release/duckdb -c "SELECT onager_version();"
```

Should output:
```
┌──────────────────┐
│ onager_version() │
│     varchar      │
├──────────────────┤
│ 0.1.0-alpha.1    │
└──────────────────┘
```
