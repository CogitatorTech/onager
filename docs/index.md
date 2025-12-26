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
Onager is written in Rust and uses [Graphina](https://github.com/habedi/graphina) graph library under the hood.

Compared to [DuckPGQ](https://github.com/cwida/duckpgq-extension), Onager is focused on graph analytics instead of graph querying.
More specifically, DuckPGQ tries to implement SQL/PGQ (the SQL:2023 standard)
for graph pattern matching and path-finding queries with a property graph model.
Onager instead provides a collection of ready-to-use graph algorithms (like PageRank, Louvain community detection, Dijkstra's shortest path, etc.)
as simple table functions.
Users typically want something like DuckPGQ when they need to query graph patterns or model the data in a property graph model
(like a graph database), and use Onager when they need to run specific graph algorithms on their graph data for a specific application.

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

## Algorithms

Onager currently includes the following graph algorithms:

| Category        | Algorithms                                                                                                     |
|-----------------|----------------------------------------------------------------------------------------------------------------|
| Centrality      | PageRank, degree, betweenness, closeness, eigenvector, Katz, harmonic, VoteRank, local reaching, and Laplacian |
| Community       | Louvain, connected components, label propagation, Girvan-Newman, spectral, and Infomap                         |
| Paths           | Dijkstra, Bellman-Ford, Floyd-Warshall, BFS, and DFS                                                           |
| Metrics         | diameter, radius, clustering, transitivity, triangles, assortativity, density, and average path length         |
| Link Prediction | Jaccard, Adamic-Adar, preferential attachment, resource allocation, and common neighbors                       |
| Subgraphs       | ego graph, k-hop neighbors, and induced subgraph                                                               |
| Generators      | Erdős-Rényi, Barabási-Albert, and Watts-Strogatz                                                               |
| Approximation   | max clique, independent set, vertex cover, and TSP                                                             |

## Get Started

- [Quick Start](getting-started/quickstart.md)
- [SQL Function Reference](reference/sql-functions.md)
