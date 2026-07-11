// Entry point for the self-hosted DuckDB-Wasm bundle. esbuild inlines the
// package and its apache-arrow dependency into one browser-ready ES module,
// which the playground imports instead of the jsdelivr "+esm" endpoint.
export * from "@duckdb/duckdb-wasm";
