---
title: Onager
description: A Graph Analytics Toolbox for DuckDB
---

# Onager

<p align="center">
  <img src="https://raw.githubusercontent.com/CogitatorTech/onager/main/logo.svg" alt="Onager Logo" width="200" />
</p>

Onager is a DuckDB extension that adds a large number of graph analytics functions to DuckDB,
including centrality measures, community detection algorithms, pathfinding algorithms, graph metrics,
and graph generators.

Onager is written in Rust 🦀 and uses [Graphina](https://github.com/habedi/graphina) graph library under the hood.

## Install

```sql
install onager from community;
load onager;
```

## Quick Example

```sql
-- Create edge data
create table edges as
select *
from (values (1::bigint, 2::bigint),
             (2, 3),
             (3, 1),
             (3, 4)) t(src, dst);

-- Compute PageRank
select node_id, round(rank, 4) as rank
from onager_ctr_pagerank((select src, dst from edges))
order by rank desc;

-- Detect communities
select node_id, community
from onager_cmm_louvain((select src, dst from edges));

-- Find shortest paths
select node_id, distance
from onager_pth_dijkstra((select src, dst from edges), source := 1);
```

## Why Onager?

- SQL-native: Run algorithms directly on DuckDB tables
- No ETL: Your data stays in DuckDB
- Fast: Written in Rust
- Simple: `select * from onager_function((select src, dst from edges))`

## Algorithms

Onager currently supports the following algorithms:

| Category        | Algorithms                                                                                              |
|-----------------|---------------------------------------------------------------------------------------------------------|
| Centrality      | PageRank, degree, betweenness, closeness, eigenvector, Katz, harmonic, and VoteRank                     |
| Community       | Louvain, connected components, label propagation, Girvan-Newman, spectral, and Infomap                  |
| Paths           | Dijkstra, Bellman-Ford, Floyd-Warshall, BFS, and DFS                                                    |
| Metrics         | diameter, radius, clustering, transitivity, triangles, assortativity, density, and avgerage path length |
| Link Prediction | Jaccard, Adamic-Adar, preferential attachment, and resource allocation                                  |
| Subgraphs       | ego graph, k-hop neighbors, and induced subgraph                                                        |
| Generators      | Erdős-Rényi, Barabási-Albert, and Watts-Strogatz                                                        |
| Approximation   | max clique, independent set, and vertex cover                                                           |

## Get Started

- [Installation](getting-started/installation.md)
- [Quick Start](getting-started/quickstart.md)
- [SQL Function Reference](reference/sql-functions.md)
