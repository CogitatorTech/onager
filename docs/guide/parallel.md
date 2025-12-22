---
title: Parallel Algorithms
description: High-performance parallel graph algorithms for large-scale analysis.
---

# Parallel Algorithms

Onager provides parallel implementations of common graph algorithms that leverage multiple CPU cores for improved performance on large graphs.

## When to Use Parallel Algorithms

Use parallel versions when:

- Your graph has **1M+ edges**
- You're running algorithms on multiple sources
- Your workload is CPU-bound

For smaller graphs, the sequential versions may be faster due to reduced overhead.

## Parallel BFS

Multi-threaded breadth-first search:

```sql
-- Parallel BFS from node 1
select *
from onager_par_bfs((select src, dst from edges), source := 1);
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | bigint | Nodes in BFS traversal order |

## Parallel Shortest Paths

Parallel single-source shortest paths:

```sql
select *
from onager_par_shortest_paths((select src, dst from edges), source := 1);
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | bigint | Target node |
| distance | double | Distance from source |

## Parallel Connected Components

Find connected components using parallel union-find:

```sql
select * from onager_par_components((select src, dst from edges));
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | bigint | Node identifier |
| component | bigint | Component identifier |

## Parallel Clustering Coefficients

Compute local clustering coefficients in parallel:

```sql
select * from onager_par_clustering((select src, dst from edges));
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | bigint | Node identifier |
| coefficient | double | Local clustering coefficient |

## Parallel Triangle Count

Count triangles per node using parallel computation:

```sql
select * from onager_par_triangles((select src, dst from edges));
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | bigint | Node identifier |
| triangles | bigint | Number of triangles containing node |

## Performance Comparison

Example on a 10M edge graph:

| Algorithm | Sequential | Parallel | Speedup |
|-----------|------------|----------|---------|
| BFS | 2.1s | 0.6s | 3.5x |
| Connected Components | 3.2s | 0.9s | 3.6x |
| Triangle Count | 8.5s | 2.2s | 3.9x |

*Results vary based on graph structure and hardware.*

## Best Practices

1. **Thread count**: Onager automatically uses available CPU cores
2. **Memory**: Parallel algorithms require more memory for thread-local storage
3. **Graph density**: Parallel algorithms provide more speedup on denser graphs
