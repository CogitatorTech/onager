## Testing Onager Extension

This directory contains a collection of mainly Sqllogictest-style tests for the Onager DuckDB extension.
These tests are different than other tests like Rust tests for the [onager](../onager) crate.

### Prerequisites

- Rust (nightly version).
- GNU Make, CMake, and a modern C++ compiler.
- Python 3.10+ (optional; only needed for test written in Python).

### Building Onager

```bash
make release
```

### Running the SQL Tests

```bash
make test
```

What this does:

1. Make sure `build/release/extension/onager/onager.duckdb_extension` exists.
2. Run DuckDB's `unittest` runner on all `test/sql/*.test` files.
3. Fails if any statement or expected result mismatches.

### Running a Single Test File

```bash
./build/release/"/test/unittest" test/sql/test_core_functionality.test
```

> [!NOTE]
> The harness path contains `/test/unittest`; keep the quote mark if your shell expands slashes weirdly.
