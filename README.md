<div align="center">
  <picture>
    <img alt="Onager Logo" src="logo.svg" height="25%" width="25%">
  </picture>
<br>

<h2>Onager</h2>

[![License](https://img.shields.io/badge/license-MIT%2FApache--2.0-007ec6?style=flat&labelColor=282c34&logo=open-source-initiative)](https://github.com/CogitatorTech/onager)

Graph Data Analytics for DuckDB

</div>

---

Onager is a DuckDB extension for **graph analytics in SQL**. It provides **25 graph algorithms** that operate directly on edge tables — no external graph database required.

Powered by the [graphina](https://github.com/habedi/graphina) Rust library.

---

## Quickstart

```bash
git clone --recursive https://github.com/CogitatorTech/onager.git
cd onager && make release
./build/release/duckdb
```

```sql
CREATE TABLE edges AS SELECT * FROM (VALUES
  (1::BIGINT, 2::BIGINT), (2, 3), (3, 1), (3, 4)
) t(src, dst);

SELECT * FROM onager_ctr_pagerank((SELECT src, dst FROM edges));
SELECT * FROM onager_gen_erdos_renyi(100, 0.1, seed := 42);
```

---

## SQL Table Functions (25)

### Centrality (`onager_ctr_*`) — 7 functions
| Function | Returns |
|----------|---------|
| `onager_ctr_pagerank(edges)` | node_id, rank |
| `onager_ctr_degree(edges)` | node_id, in_degree, out_degree |
| `onager_ctr_betweenness(edges)` | node_id, betweenness |
| `onager_ctr_closeness(edges)` | node_id, closeness |
| `onager_ctr_eigenvector(edges)` | node_id, eigenvector |
| `onager_ctr_katz(edges)` | node_id, katz |
| `onager_ctr_harmonic(edges)` | node_id, harmonic |

### Community (`onager_cmm_*`) — 3 functions
| Function | Returns |
|----------|---------|
| `onager_cmm_louvain(edges)` | node_id, community |
| `onager_cmm_components(edges)` | node_id, component |
| `onager_cmm_label_prop(edges)` | node_id, label |

### Link Prediction (`onager_lnk_*`) — 4 functions
| Function | Returns |
|----------|---------|
| `onager_lnk_jaccard(edges)` | node1, node2, jaccard |
| `onager_lnk_adamic_adar(edges)` | node1, node2, score |
| `onager_lnk_pref_attach(edges)` | node1, node2, score |
| `onager_lnk_resource_alloc(edges)` | node1, node2, score |

### Metrics (`onager_mtr_*`) — 5 functions
| Function | Returns |
|----------|---------|
| `onager_mtr_diameter(edges)` | diameter |
| `onager_mtr_radius(edges)` | radius |
| `onager_mtr_avg_clustering(edges)` | avg_clustering |
| `onager_mtr_avg_path_length(edges)` | avg_path_length |
| `onager_mtr_transitivity(edges)` | transitivity |

### Paths (`onager_pth_*`) — 1 function
| Function | Returns |
|----------|---------|
| `onager_pth_dijkstra(edges)` | node_id, distance |

### Traversal (`onager_trv_*`) — 2 functions
| Function | Returns |
|----------|---------|
| `onager_trv_bfs(edges)` | node_id, order |
| `onager_trv_dfs(edges)` | node_id, order |

### Generators (`onager_gen_*`) — 3 functions
| Function | Returns |
|----------|---------|
| `onager_gen_erdos_renyi(n, p)` | src, dst |
| `onager_gen_barabasi_albert(n, m)` | src, dst |
| `onager_gen_watts_strogatz(n, k, beta)` | src, dst |

---

## Input Format

All analysis functions accept edge tables with positional columns:
- **Column 1**: Source node (BIGINT)
- **Column 2**: Destination node (BIGINT)

```sql
SELECT * FROM onager_ctr_pagerank((SELECT a, b FROM my_edges));
```

---

## License

MIT or Apache 2.0 — your choice.

## Acknowledgements

* Graph algorithms powered by [graphina](https://github.com/habedi/graphina)
