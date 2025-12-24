---
title: Installation
description: Install Onager in DuckDB.
---

# Installation

Onager is available in the DuckDB community extensions repository. Installation takes just two commands.

## Install from Community Extensions

Open any DuckDB client (CLI, Python, Node.js, etc.) and run:

```sql
install onager from community;
load onager;
```

That's it. You're ready to run graph algorithms on your data.

## Verify Installation

```sql
select onager_version();
```

Expected output:

```
┌──────────────────┐
│ onager_version() │
│     varchar      │
├──────────────────┤
│ 0.1.0            │
└──────────────────┘
```

## Loading Automatically

To load Onager automatically when DuckDB starts, run:

```sql
install onager from community;
load onager;
set autoload_known_extensions=true;
set autoinstall_known_extensions=true;
```

## Supported Platforms

At the moment, Onager is available for:

- Linux (x86_64 and aarch64)
- macOS (x86_64 and aarch64)
- Windows (x86_64)

## Building from Source

For development or to use the latest unreleased features:

```bash
git clone --recursive https://github.com/CogitatorTech/onager.git
cd onager
make release
./build/release/duckdb
```

Build requirements: Rust 1.70+, CMake 3.15+, C++17 compiler.
