---
title: Subgraph Operations
description: Extract portions of a graph around specific nodes.
---

# Subgraph Operations

Subgraph operations extract portions of a larger graph based on structural proximity or node membership. Useful for
analyzing local neighborhoods or focusing on specific regions.

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
             (7, 8),
             (8, 9)) t(src, dst);
```

---

## Ego Graph

Extracts the subgraph within a given radius of a center node. The ego graph includes the center node, all nodes within
the specified number of hops, and all edges between them.

```sql
select src, dst
from onager_sub_ego_graph((select src, dst from edges), center := 4, radius := 2);
```

| Column | Type   | Description         |
|--------|--------|---------------------|
| src    | bigint | Source node of edge |
| dst    | bigint | Destination node    |

Parameters:

- `center`: The central node of the ego graph
- `radius`: Maximum distance from center (number of hops)

```sql
-- Compare ego graphs at different radii
select 'radius=1' as scope, count(*) as edges
from onager_sub_ego_graph((select src, dst from edges), center := 4, radius := 1)
union all
select 'radius=2', count(*)
from onager_sub_ego_graph((select src, dst from edges), center := 4, radius := 2)
union all
select 'radius=3', count(*)
from onager_sub_ego_graph((select src, dst from edges), center := 4, radius := 3);
```

---

## K-Hop Neighbors

Returns all nodes within k hops of a starting node. Unlike ego graph, this returns only node IDs, not edges.

```sql
select node_id
from onager_sub_k_hop((select src, dst from edges), start := 1, k := 2)
order by node_id;
```

| Column  | Type   | Description                      |
|---------|--------|----------------------------------|
| node_id | bigint | Node within k hops of start node |

Parameters:

- `start`: Starting node
- `k`: Maximum number of hops (0 returns just the start node)

```sql
-- Find nodes at exactly distance 2 (in 2-hop but not in 1-hop)
with hop1 as (select node_id from onager_sub_k_hop((select src, dst from edges), start := 1, k := 1)),
     hop2 as (select node_id from onager_sub_k_hop((select src, dst from edges), start := 1, k := 2))
select h2.node_id
from hop2 h2
         left join hop1 h1 on h2.node_id = h1.node_id
where h1.node_id is null;
```

---

## Induced Subgraph

Given a set of nodes, returns the subgraph containing only those nodes and the edges between them. The induced subgraph
preserves the original graph structure within the specified node set.

```sql
-- Extract subgraph for specific nodes using array syntax
select src, dst
from onager_sub_induced((select src, dst from edges), nodes := [2, 3, 4, 5]);
```

| Column | Type   | Description         |
|--------|--------|---------------------|
| src    | bigint | Source node of edge |
| dst    | bigint | Destination node    |

> [!NOTE]
> DuckDB table functions support only one subquery parameter, so use the array syntax shown above.

---

## Complete Example: Neighborhood Analysis

Analyze the local structure around a node of interest:

```sql
create table social as
select *
from (values (1, 2),
             (1, 3),
             (2, 3),
             (2, 4),
             (3, 4),
             (3, 5),
             (4, 5),
             (4, 6),
             (5, 6),
             (5, 7),
             (6, 7),
             (6, 8),
             (7, 8),
             (7, 9),
             (8, 9),
             (8, 10)) t(src, dst);

-- Find nodes near user 5
select node_id as nearby_user
from onager_sub_k_hop((select src, dst from social), start := 5, k := 2)
order by node_id;

-- Get the ego network and analyze it
with ego as (select src, dst
             from onager_sub_ego_graph((select src, dst from social), center := 5, radius := 2))
select (select count(distinct src) + count(distinct dst) from (select src from ego union select dst from ego)) as nodes,
       (select count(*) from ego)                                                                              as edges,
       (select round(avg_clustering, 3)
        from onager_mtr_avg_clustering((select * from ego)))                                                   as clustering;

-- Compute centrality within the neighborhood
with ego as (select src, dst
             from onager_sub_ego_graph((select src, dst from social), center := 5, radius := 2))
select node_id, round(rank, 4) as local_importance
from onager_ctr_pagerank((select * from ego))
order by rank desc limit 5;
```
