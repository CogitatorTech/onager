---
title: Subgraph Operations
description: Extract subgraphs and explore neighborhoods in your network data.
---

# Subgraph Operations

Onager provides powerful subgraph extraction functions for exploring local graph structures around nodes of interest.

## Ego Graph

Extract the ego network centered on a specific node within a given radius:

```sql
-- Get the 2-hop ego network around node 1
select *
from onager_sub_ego_graph((select src, dst from edges), center := 1, radius := 2);
```

Returns the edges of the subgraph induced by nodes within `radius` hops of `center`.

| Column | Type | Description |
|--------|------|-------------|
| src | bigint | Source node of each edge |
| dst | bigint | Destination node of each edge |

## K-Hop Neighbors

Find all nodes reachable within k steps from a starting node:

```sql
-- Get all nodes within 3 hops of node 1
select *
from onager_sub_k_hop_neighbors((select src, dst from edges), start := 1, k := 3);
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | bigint | Nodes reachable within k hops |

## Induced Subgraph

Extract the subgraph induced by a specific set of nodes:

```sql
-- Get subgraph containing only nodes 1, 2, 3, 4
with node_set as (
    select src, dst, filter_node
    from edges, (values (1), (2), (3), (4)) as t(filter_node)
)
select * from onager_sub_induced_subgraph(node_set);
```

| Column | Type | Description |
|--------|------|-------------|
| src | bigint | Source node of edges in induced subgraph |
| dst | bigint | Destination node of edges in induced subgraph |

## Use Cases

### Community Analysis

Combine ego networks with community detection to understand local structure:

```sql
-- Find ego network and its community structure
with ego as (
    select * from onager_sub_ego_graph(
        (select src, dst from edges), center := 42, radius := 2
    )
)
select * from onager_cmm_louvain(ego);
```

### Influence Propagation

Use k-hop neighbors to find potential influence reach:

```sql
-- Find all users within 2 degrees of an influencer
select count(*) as reach
from onager_sub_k_hop_neighbors(
    (select follower as src, followed as dst from follows),
    start := 12345, -- influencer ID
    k := 2
);
```
