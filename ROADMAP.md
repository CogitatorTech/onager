# Onager Roadmap

## Current Status: Alpha (v0.1.0)

### ✅ Implemented (25 SQL Functions)

#### Centrality (`ctr`) — 7 functions
- [x] pagerank, degree, betweenness, closeness, eigenvector, katz, harmonic

#### Community (`cmm`) — 3 functions
- [x] louvain, components, label_prop

#### Link Prediction (`lnk`) — 4 functions  
- [x] jaccard, adamic_adar, pref_attach, resource_alloc

#### Metrics (`mtr`) — 5 functions
- [x] diameter, radius, avg_clustering, avg_path_length, transitivity

#### Paths (`pth`) — 1 function
- [x] dijkstra

#### Traversal (`trv`) — 2 functions
- [x] bfs, dfs

#### Generators (`gen`) — 3 functions
- [x] erdos_renyi, barabasi_albert, watts_strogatz

---

### 🔜 Planned

#### More Algorithms
- [ ] Bellman-Ford, A*, Floyd-Warshall shortest paths
- [ ] Girvan-Newman, Infomap community detection
- [ ] MST (Prim, Kruskal)
- [ ] Parallel versions of core algorithms

---

## Architecture

```
SQL → DuckDB C++ → Rust FFI → graphina
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.
