---
title: Input Formats
description: How to pass graph data to Onager functions.
---

# Input Formats

This page describes how to pass graph data to Onager functions.

## Table Functions

Table functions expect edges as a subquery with two `bigint` columns:

```sql
select *
from onager_ctr_pagerank((select src, dst from my_edges));
```

## Named Graphs

Scalar functions operate on named graphs stored in the registry:

```sql
select onager_create_graph('social', true);
select onager_add_node('social', 1);
select onager_node_in_degree('social', 1);
```

---

!!! note "Edge list format"
    Onager uses edge lists, not adjacency matrices. Each row represents one edge with a source and destination node.

!!! note "Node IDs"
    Node IDs must be `bigint` integers. They do not need to be continuous or start from zero. Any integer value works, like `1, 2, 3` or `100, 200, 500`.

!!! note "Directed vs undirected"
    Most functions treat graphs as undirected by default. Use the `directed := true` parameter where available to treat edges as one-way.

!!! note "Weighted functions"
    Some functions accept edge weights as a third column. These include:

    - `onager_pth_bellman_ford` — shortest paths with negative weights
    - `onager_pth_floyd_warshall` — all-pairs shortest paths
    - `onager_mst_kruskal` — minimum spanning tree
    - `onager_mst_prim` — minimum spanning tree
    - `onager_apx_tsp` — traveling salesman approximation

    Pass weights like this:
    ```sql
    select * from onager_mst_kruskal((select src, dst, weight from edges));
    ```
