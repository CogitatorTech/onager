# SQL Function Reference

Complete reference for all 25 Onager SQL functions.

## Centrality Functions (`onager_ctr_*`)

| Function | Returns | Description |
|----------|---------|-------------|
| `onager_ctr_pagerank(edges)` | `node_id, rank` | PageRank centrality |
| `onager_ctr_degree(edges)` | `node_id, in_degree, out_degree` | Degree centrality |
| `onager_ctr_betweenness(edges)` | `node_id, betweenness` | Betweenness centrality |
| `onager_ctr_closeness(edges)` | `node_id, closeness` | Closeness centrality |
| `onager_ctr_eigenvector(edges)` | `node_id, eigenvector` | Eigenvector centrality |
| `onager_ctr_katz(edges)` | `node_id, katz` | Katz centrality |
| `onager_ctr_harmonic(edges)` | `node_id, harmonic` | Harmonic centrality |

## Community Detection Functions (`onager_cmm_*`)

| Function | Returns | Description |
|----------|---------|-------------|
| `onager_cmm_louvain(edges [, seed])` | `node_id, community` | Louvain modularity optimization |
| `onager_cmm_components(edges)` | `node_id, component` | Connected components |
| `onager_cmm_label_prop(edges)` | `node_id, label` | Label propagation |

## Link Prediction Functions (`onager_lnk_*`)

| Function | Returns | Description |
|----------|---------|-------------|
| `onager_lnk_jaccard(edges)` | `node1, node2, jaccard` | Jaccard coefficient |
| `onager_lnk_adamic_adar(edges)` | `node1, node2, score` | Adamic-Adar index |
| `onager_lnk_pref_attach(edges)` | `node1, node2, score` | Preferential attachment |
| `onager_lnk_resource_alloc(edges)` | `node1, node2, score` | Resource allocation |

## Metric Functions (`onager_mtr_*`)

| Function | Returns | Description |
|----------|---------|-------------|
| `onager_mtr_diameter(edges)` | `diameter` | Graph diameter |
| `onager_mtr_radius(edges)` | `radius` | Graph radius |
| `onager_mtr_avg_clustering(edges)` | `avg_clustering` | Average clustering coefficient |
| `onager_mtr_avg_path_length(edges)` | `avg_path_length` | Average shortest path length |
| `onager_mtr_transitivity(edges)` | `transitivity` | Global clustering (transitivity) |

## Path & Traversal Functions (`onager_pth_*`, `onager_trv_*`)

| Function | Returns | Description |
|----------|---------|-------------|
| `onager_pth_dijkstra(edges, source)` | `node_id, distance` | Shortest paths from source |
| `onager_trv_bfs(edges, source)` | `node_id, order` | Breadth-first traversal |
| `onager_trv_dfs(edges, source)` | `node_id, order` | Depth-first traversal |

## Generator Functions (`onager_gen_*`)

| Function | Returns | Description |
|----------|---------|-------------|
| `onager_gen_erdos_renyi(n, p [, seed])` | `src, dst` | Random graph (G(n,p)) |
| `onager_gen_barabasi_albert(n, m [, seed])` | `src, dst` | Scale-free graph |
| `onager_gen_watts_strogatz(n, k, beta [, seed])` | `src, dst` | Small-world graph |

## Utility Functions

| Function | Returns | Description |
|----------|---------|-------------|
| `onager_version()` | `VARCHAR` | Extension version |
| `onager_last_error()` | `VARCHAR` | Last error message |

## Input Format

All functions expect edges as a subquery with two `BIGINT` columns:

```sql
SELECT * FROM onager_ctr_pagerank((SELECT src, dst FROM my_edges));
```
