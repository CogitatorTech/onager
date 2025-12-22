# Onager

<p align="center">
  <img src="https://raw.githubusercontent.com/CogitatorTech/onager/main/logo.svg" alt="Onager Logo" width="200" />
</p>

**Onager** is a DuckDB extension for **graph analytics in SQL**. It provides graph algorithms and a graph registry that
operate directly on edge tables — no external graph database required.

Powered by the [graphina](https://github.com/habedi/graphina) Rust library.

## Why Onager?

- **SQL-Native**: Run graph algorithms directly on your DuckDB tables
- **No ETL Required**: Your edge data stays in DuckDB — no export needed
- **High Performance**: Rust-powered algorithms with parallel execution
- **Easy Integration**: `select * from onager_ctr_pagerank((select src, dst from edges))`

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
select *
from onager_ctr_pagerank((select src, dst from edges));

-- Detect communities
select *
from onager_cmm_louvain((select src, dst from edges));

-- Generate random graphs
select *
from onager_gen_erdos_renyi(100, 0.1, seed := 42);
```

## Available Features

| Category            | Functions                                                                                                          |
|---------------------|--------------------------------------------------------------------------------------------------------------------|
| **Centrality**      | pagerank, personalized_pagerank, degree, betweenness, closeness, eigenvector, katz, harmonic                       |
| **Community**       | louvain, components, label_prop, girvan_newman, spectral, infomap                                                  |
| **Link Prediction** | jaccard, adamic_adar, pref_attach, resource_alloc                                                                  |
| **Metrics**         | diameter, radius, avg_clustering, avg_path_length, transitivity, assortativity                                     |
| **Traversal**       | dijkstra, bfs, dfs                                                                                                 |
| **Generators**      | erdos_renyi, barabasi_albert, watts_strogatz                                                                       |
| **Subgraphs**       | ego_graph, k_hop_neighbors, induced_subgraph                                                                       |
| **Parallel**        | pagerank, bfs, shortest_paths, components, clustering, triangles                                                   |
| **Graph Registry**  | create_graph, drop_graph, add_node, add_edge, list_graphs, node_count, edge_count, node_in_degree, node_out_degree |

## Quick Links

- [Getting Started](getting-started/installation.md)
- [Graph Registry](guide/graph-registry.md)
- [SQL Function Reference](reference/sql-functions.md)
- [GitHub Repository](https://github.com/CogitatorTech/onager)
