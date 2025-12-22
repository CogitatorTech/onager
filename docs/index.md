# Onager

<p align="center">
  <img src="https://raw.githubusercontent.com/CogitatorTech/onager/main/logo.svg" alt="Onager Logo" width="200" />
</p>

**Onager** is a DuckDB extension for **graph analytics in SQL**. It provides **25 graph algorithms** that operate directly on edge tables — no external graph database required.

Powered by the [graphina](https://github.com/habedi/graphina) Rust library.

## Why Onager?

- **SQL-Native**: Run graph algorithms directly on your DuckDB tables
- **No ETL Required**: Your edge data stays in DuckDB — no export needed
- **High Performance**: Rust-powered algorithms with parallel execution
- **Easy Integration**: `SELECT * FROM onager_ctr_pagerank((SELECT src, dst FROM edges))`

## Quick Example

```sql
-- Create edge data
CREATE TABLE edges AS SELECT * FROM (VALUES 
  (1::BIGINT, 2::BIGINT), (2, 3), (3, 1), (3, 4)
) t(src, dst);

-- Compute PageRank
SELECT * FROM onager_ctr_pagerank((SELECT src, dst FROM edges));

-- Detect communities
SELECT * FROM onager_cmm_louvain((SELECT src, dst FROM edges));

-- Generate random graphs
SELECT * FROM onager_gen_erdos_renyi(100, 0.1, seed := 42);
```

## Available Algorithms

| Category | Functions |
|----------|-----------|
| **Centrality** (7) | pagerank, degree, betweenness, closeness, eigenvector, katz, harmonic |
| **Community** (3) | louvain, components, label_prop |
| **Link Prediction** (4) | jaccard, adamic_adar, pref_attach, resource_alloc |
| **Metrics** (5) | diameter, radius, avg_clustering, avg_path_length, transitivity |
| **Traversal** (3) | dijkstra, bfs, dfs |
| **Generators** (3) | erdos_renyi, barabasi_albert, watts_strogatz |

## Quick Links

- [Getting Started](getting-started/installation.md)
- [SQL Function Reference](reference/sql-functions.md)
- [GitHub Repository](https://github.com/CogitatorTech/onager)
