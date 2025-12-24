---
title: Community Detection
description: Find groups of densely connected nodes in your graph.
---

# Community Detection

Community detection algorithms identify clusters of nodes that are more densely connected to each other than to the rest
of the graph. These groups often represent meaningful structures like social circles, functional modules, or topic
clusters.

## Setup

All examples use this sample network with two natural communities:

```sql
create table edges as
select *
from (values
          -- Community 1 (nodes 1-4)
          (1::bigint, 2::bigint),
          (1, 3),
          (2, 3),
          (2, 4),
          (3, 4),
          -- Community 2 (nodes 5-8)
          (5, 6),
          (5, 7),
          (6, 7),
          (6, 8),
          (7, 8),
          -- Bridge between communities
          (4, 5)) t(src, dst);
```

---

## Louvain Algorithm

The Louvain algorithm is a greedy optimization method that maximizes modularity. It works hierarchically, first finding
small communities, then aggregating them into larger ones. Fast and produces high-quality results on most networks.

```sql
select node_id, community
from onager_cmm_louvain((select src, dst from edges))
order by community, node_id;
```

| Column    | Type   | Description                       |
|-----------|--------|-----------------------------------|
| node_id   | bigint | Node identifier                   |
| community | bigint | Community ID this node belongs to |

Use the `seed` parameter for reproducible results:

```sql
select *
from onager_cmm_louvain((select src, dst from edges), seed := 42);
```

---

## Connected Components

Finds groups of nodes where every node is reachable from every other node in the group. Unlike community detection, this
is a topological property - nodes are in the same component if any path connects them.

```sql
select node_id, component
from onager_cmm_components((select src, dst from edges))
order by component, node_id;
```

| Column    | Type   | Description                       |
|-----------|--------|-----------------------------------|
| node_id   | bigint | Node identifier                   |
| component | bigint | Component ID (connected subgraph) |

---

## Label Propagation

A fast, near-linear time algorithm. Each node adopts the most common label among its neighbors. Simple but can produce
different results on each run.

```sql
select node_id, label
from onager_cmm_label_prop((select src, dst from edges))
order by label, node_id;
```

| Column  | Type   | Description     |
|---------|--------|-----------------|
| node_id | bigint | Node identifier |
| label   | bigint | Community label |

---

## Girvan-Newman

Detects communities by progressively removing edges with highest betweenness. Produces a hierarchy of communities.
Specify the desired number of communities.

```sql
select node_id, community
from onager_cmm_girvan_newman((select src, dst from edges), communities := 2)
order by community, node_id;
```

---

## Spectral Clustering

Uses eigenvalues of the graph Laplacian to partition nodes. Works well when communities have similar sizes.

```sql
select node_id, cluster
from onager_cmm_spectral((select src, dst from edges), k := 2)
order by cluster, node_id;
```

---

## Infomap

Optimizes the map equation to find communities that minimize the description length of a random walk. Particularly good
at finding flow-based communities.

```sql
select node_id, community
from onager_cmm_infomap((select src, dst from edges))
order by community, node_id;
```

---

## Complete Example: Community Analysis

Analyze community structure and find bridge nodes:

```sql
-- Create a network
create table social as
select *
from (values (1, 2),
             (1, 3),
             (2, 3),
             (2, 4),
             (3, 4),
             (4, 5),
             (5, 6),
             (5, 7),
             (6, 7),
             (6, 8),
             (7, 8),
             (8, 9),
             (9, 10),
             (9, 11),
             (10, 11),
             (10, 12),
             (11, 12)) t(src, dst);

-- Detect communities
with comms as (select node_id, community
               from onager_cmm_louvain((select src, dst from social), seed := 42))
select community, count(*) as size, array_agg(node_id order by node_id) as members
from comms
group by community
order by size desc;

-- Find bridge nodes (high betweenness between communities)
select c.node_id, c.community, round(b.betweenness, 2) as bridge_score
from onager_cmm_louvain((select src, dst from social), seed := 42) c
         join onager_ctr_betweenness((select src, dst from social)) b
              on c.node_id = b.node_id
where b.betweenness > 0
order by b.betweenness desc limit 5;
```
