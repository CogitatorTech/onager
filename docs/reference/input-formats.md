---
title: Input Formats
description: How to pass graph data to Onager functions.
---

# Input Formats

## Table Functions

Table functions expect edges as a subquery with two `bigint` columns:

```sql
create table my_edges as select * from (values (1::bigint, 2::bigint), (2, 3)) t(src, dst);

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
    Every function treats the edge list as undirected by default. Direction-sensitive functions (centrality, traversal, shortest paths, the diameter, radius, average path length, assortativity, and density metrics, and the parallel PageRank, BFS, and shortest path variants) accept `directed := true` to treat each edge row as one-way. See the [SQL Function Reference](sql-functions.md) for the functions that take the parameter. Note that edges are not deduplicated, so passing both `(1, 2)` and `(2, 1)` to an undirected function creates two parallel edges between the nodes.

!!! note "Weighted functions"
    Some functions accept edge weights as a third column. These include:

    - `onager_pth_bellman_ford`: Shortest paths with negative weights
    - `onager_pth_floyd_warshall`: All-pairs shortest paths
    - `onager_mst_kruskal`: Minimum spanning tree (Kruskal's)
    - `onager_mst_prim`: Minimum spanning tree (Prim's)
    - `onager_apx_tsp`: Traveling salesman approximation
    - `onager_pth_dijkstra`: Shortest paths with nonnegative weights (the weights column is optional)

    `onager_ctr_pagerank` also uses a third `double` column as edge weights when it is present, and falls back to weight 1.0 for every edge when it is absent. `onager_par_pagerank` is unweighted. Weight values must not be NULL or NaN, and PageRank additionally rejects negative weights.

    Pass weights like this:
    ```sql
    create table edges as select * from (values
      (1::bigint, 2::bigint, 1.0::double), (2, 3, 2.0), (3, 4, 1.5)
    ) t(src, dst, weight);

    select * from onager_mst_kruskal((select src, dst, weight from edges));
    ```
