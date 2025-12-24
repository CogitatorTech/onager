---
title: Parallel Algorithms
description: High-performance algorithms using parallel execution.
---

# Parallel Algorithms

Parallel algorithms use multiple threads to process large graphs faster. These are optimized versions of common
algorithms designed for better performance on multi-core systems.

## Setup

```sql
create table edges as
select *
from (values (1::bigint, 2::bigint),
             (1, 3),
             (2, 3),
             (2, 4),
             (3, 4),
             (4, 5),
             (4, 6),
             (5, 6),
             (5, 7),
             (6, 7),
             (7, 8)) t(src, dst);
```

---

## Parallel PageRank

Computes PageRank centrality using parallel processing. Same semantics as the sequential version but faster on large
graphs.

```sql
select node_id, round(rank, 4) as rank
from onager_par_pagerank((select src, dst from edges))
order by rank desc;
```

| Column  | Type   | Description     |
|---------|--------|-----------------|
| node_id | bigint | Node identifier |
| rank    | double | PageRank score  |

Optional parameters:

- `damping` (default 0.85): Probability of following a link
- `iterations` (default 100): Maximum iterations
- `directed` (default false): Treat graph as directed

---

## Parallel BFS

Breadth-first search using parallel execution. Returns nodes in BFS order from the source.

```sql
select node_id
from onager_par_bfs((select src, dst from edges), source := 1);
```

| Column  | Type   | Description             |
|---------|--------|-------------------------|
| node_id | bigint | Node visited during BFS |

---

## Parallel Shortest Paths

Computes shortest path distances from a source to all reachable nodes using parallel processing.

```sql
select node_id, distance
from onager_par_shortest_paths((select src, dst from edges), source := 1)
order by distance;
```

| Column   | Type   | Description                   |
|----------|--------|-------------------------------|
| node_id  | bigint | Node identifier               |
| distance | double | Shortest distance from source |

---

## Parallel Connected Components

Finds connected components using a parallel union-find algorithm.

```sql
select node_id, component
from onager_par_components((select src, dst from edges))
order by component, node_id;
```

| Column    | Type   | Description     |
|-----------|--------|-----------------|
| node_id   | bigint | Node identifier |
| component | bigint | Component ID    |

---

## Parallel Clustering Coefficients

Computes local clustering coefficient for each node using parallel processing.

```sql
select node_id, round(coefficient, 4) as clustering
from onager_par_clustering((select src, dst from edges))
where coefficient > 0
order by coefficient desc;
```

| Column      | Type   | Description                  |
|-------------|--------|------------------------------|
| node_id     | bigint | Node identifier              |
| coefficient | double | Local clustering coefficient |

---

## Parallel Triangle Count

Counts triangles participating at each node using parallel processing.

```sql
select node_id, triangles
from onager_par_triangles((select src, dst from edges))
where triangles > 0
order by triangles desc;
```

| Column    | Type   | Description                 |
|-----------|--------|-----------------------------|
| node_id   | bigint | Node identifier             |
| triangles | bigint | Number of triangles at node |

---

## Complete Example: Large Graph Analysis

Use parallel algorithms for analyzing larger networks:

```sql
-- Generate a larger test graph
create table large_net as
select src, dst
from onager_gen_barabasi_albert(1000, 3, seed := 42);

-- Parallel analysis pipeline
select 'Edges' as metric, count(*) ::varchar as value
from large_net
union all
select 'Components', count(distinct component) ::varchar
from onager_par_components((select src, dst from large_net))
union all
select 'Avg Clustering', round(avg(coefficient), 4) ::varchar
from onager_par_clustering((select src, dst from large_net))
union all
select 'Total Triangles', (sum(triangles) / 3) ::varchar
from onager_par_triangles((select src, dst from large_net));

-- Find top nodes by parallel PageRank
select node_id, round(rank, 6) as rank
from onager_par_pagerank((select src, dst from large_net))
order by rank desc limit 10;
```

## When to Use Parallel Algorithms

Parallel algorithms provide benefits for:

- Graphs with thousands of nodes or more
- Analysis pipelines running multiple algorithms
- Batch processing of multiple graphs

For small graphs (under 1000 nodes), sequential algorithms may be faster due to parallelization overhead.
