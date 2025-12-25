---
title: Traversal and Paths
description: Navigate graphs with BFS, DFS, and shortest path algorithms.
---

# Traversal and Paths

Traversal algorithms systematically visit nodes in a graph.
Shortest path algorithms find optimal routes between nodes.
These are fundamental operations for graph analysis.

## Setup

```sql
create table edges as
select *
from (values (1::bigint, 2::bigint),
             (1, 3),
             (2, 4),
             (3, 4),
             (4, 5),
             (4, 6),
             (5, 7),
             (6, 7),
             (7, 8)) t(src, dst);
```

---

## Breadth-First Search (BFS)

Explores nodes level by level, starting from a source.
Visits all neighbors before moving to neighbors of neighbors.
Useful for finding shortest paths in unweighted graphs.

```sql
select node_id, order_num
from onager_trv_bfs((select src, dst from edges), source := 1)
order by order_num;
```

| Column    | Type   | Description                               |
|-----------|--------|-------------------------------------------|
| node_id   | bigint | Node identifier                           |
| order_num | bigint | Order in which node was visited (0-based) |

---

## Depth-First Search (DFS)

Explores as far as possible along each branch before backtracking.
Visits a neighbor, then that neighbor's neighbor, and so on.
Useful for topological sorting and cycle detection.

```sql
select node_id, order_num
from onager_trv_dfs((select src, dst from edges), source := 1)
order by order_num;
```

---

## Dijkstra's Algorithm

Finds shortest paths from a source to all reachable nodes.
Assumes non-negative edge weights. The classic algorithm for shortest paths.

```sql
select node_id, distance
from onager_pth_dijkstra((select src, dst from edges), source := 1)
order by distance;
```

| Column   | Type   | Description                   |
|----------|--------|-------------------------------|
| node_id  | bigint | Node identifier               |
| distance | double | Shortest distance from source |

---

## Bellman-Ford Algorithm

Finds shortest paths even with negative edge weights.
Slower than Dijkstra but more general. Can detect negative cycles.

For weighted edges, provide a third column:

```sql
create table weighted_edges as
select *
from (values (1::bigint, 2::bigint, 1.0),
             (1, 3, 4.0),
             (2, 3, 2.0),
             (2, 4, 5.0),
             (3, 4, 1.0)) t(src, dst, weight);

select node_id, distance
from onager_pth_bellman_ford((select src, dst, weight from weighted_edges), source := 1)
order by distance;
```

---

## Floyd-Warshall Algorithm

Computes shortest paths between all pairs of nodes.
Returns a row for every (source, destination) pair. Useful when you need distances between many node pairs.

!!! warning "Performance"
    This algorithm has O(n³) time and O(n²) space complexity. So, it's recommended to use it only on smaller graphs (like with fewer than 1,000 nodes).

```sql
select src, dst, round(distance, 2) as distance
from onager_pth_floyd_warshall((select src, dst, 1.0 as weight from edges))
where distance < 1000
order by src, dst;
```

| Column   | Type   | Description                    |
|----------|--------|--------------------------------|
| src      | bigint | Source node                    |
| dst      | bigint | Destination node               |
| distance | double | Shortest distance between them |

---

## Complete Example: Network Distance Analysis

Find the most central nodes by shortest path distances:

```sql
create table network as
select *
from (values (1, 2),
             (1, 3),
             (2, 3),
             (2, 4),
             (3, 5),
             (4, 5),
             (4, 6),
             (5, 6),
             (5, 7),
             (6, 7),
             (7, 8)) t(src, dst);

-- Find nodes closest to node 1
select node_id, distance
from onager_pth_dijkstra((select src, dst from network), source := 1)
order by distance limit 5;

-- Compare BFS order vs distance
with bfs as (select node_id, order_num from onager_trv_bfs((select src, dst from network), source := 1)),
     dist as (select node_id, distance from onager_pth_dijkstra((select src, dst from network), source := 1))
select bfs.node_id, bfs.order_num as visit_order, dist.distance
from bfs
         join dist on bfs.node_id = dist.node_id
order by bfs.order_num;
```
