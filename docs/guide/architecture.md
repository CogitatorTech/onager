# Architecture

Onager is a DuckDB extension with three main layers.

## Overview

```
┌─────────────────────────────────────────────────┐
│               DuckDB SQL Layer                   │
│  SELECT * FROM onager_ctr_pagerank(...)         │
└─────────────────────┬───────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────┐
│           C++ Table Functions                    │
│  bindings/onager_extension.cpp                  │
└─────────────────────┬───────────────────────────┘
                      │ FFI
┌─────────────────────▼───────────────────────────┐
│              Rust Library                        │
│  onager/src/{algorithms/, ffi/}                 │
└─────────────────────┬───────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────┐
│              graphina (Rust)                     │
│  Graph algorithms library                        │
└─────────────────────────────────────────────────┘
```

## Components

### 1. SQL Table Functions (C++)

`bindings/onager_extension.cpp` defines DuckDB table functions that:

- Accept edge data as input tables
- Call Rust FFI functions
- Return algorithm results as tables

### 2. Rust FFI Layer

`onager/src/ffi/` exports C-compatible functions that:

- Convert between DuckDB arrays and Rust types
- Handle error propagation
- Connect to algorithm implementations

### 3. Algorithm Implementations

`onager/src/algorithms/` contains wrappers around graphina that:

- Build graphs from edge arrays
- Execute graph algorithms
- Convert results back to arrays

### 4. graphina Library

The [graphina](https://github.com/habedi/graphina) Rust library provides:

- High-performance graph data structures
- 25+ graph algorithms
- Parallel implementations

## Build Process

```bash
make release
```

1. Rust builds `libonager.a` static library
2. cbindgen generates `bindings/include/rust.h`
3. CMake builds DuckDB with the extension statically linked
