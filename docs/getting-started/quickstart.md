---
title: Quick Start
description: Get up and running with Onager in 5 minutes.
---

# Quick Start

This guide walks you through your first graph analysis with Onager.

## 1. Install and Load

Onager is available in the [DuckDB community extensions repository](https://duckdb.org/community_extensions/extensions/onager) and can be installed with the following commands:

```sql
install onager from community;
load onager;
```

## 2. Create Some Edge Data

Onager works with edge tables — any table with source and destination columns:

```sql
create table edges as
select *
from (values (1::bigint, 2::bigint),
             (2, 3),
             (3, 1),
             (3, 4),
             (4, 5),
             (5, 6)) t(src, dst);
```

## 3. Find Important Nodes

Use PageRank to identify the most important nodes:

```sql
select node_id, round(rank, 4) as importance
from onager_ctr_pagerank((select src, dst from edges))
order by rank desc;
```

## 4. Detect Communities

Find clusters of connected nodes:

```sql
select node_id, community
from onager_cmm_louvain((select src, dst from edges))
order by community, node_id;
```

## 5. Explore Graph Structure

Measure network properties:

```sql
-- How many hops across the network?
select diameter
from onager_mtr_diameter((select src, dst from edges));

-- How clustered is it?
select round(avg_clustering, 4) as clustering
from onager_mtr_avg_clustering((select src, dst from edges));
```

## 6. Find Shortest Paths

Calculate distances from a starting node:

```sql
select node_id, distance
from onager_pth_dijkstra((select src, dst from edges), source := 1::bigint)
order by distance;
```

## Working with Real Data

Import your edge data from any source DuckDB supports:

```sql
-- From an existing table (ensure bigint columns)
create table my_edges as select * from (values
  (1::bigint, 2::bigint), (2, 3), (3, 4)
) t(source_id, target_id);

-- Run PageRank on your data
select *
from onager_ctr_pagerank((select source_id as src, target_id as dst
                          from my_edges));
```

## Input Format

All functions expect edges as a subquery with two `bigint` columns:

```sql
create table your_table as select * from (values (1::bigint, 2::bigint), (2, 3)) t(source_column, target_column);

select *
from onager_ctr_pagerank((select source_column as src, target_column as dst
                           from your_table));
-- Replace your_table and column names with your actual table and columns
```

See the [Input Formats](../reference/input-formats.md) reference for details on node IDs, weighted functions, and directed graphs.

## Next Steps

- [Centrality Algorithms](../guide/centrality.md) — Find important nodes
- [Community Detection](../guide/community.md) — Discover clusters
- [Graph Metrics](../guide/metrics.md) — Measure network properties
- [SQL Function Reference](../reference/sql-functions.md) — All functions
