# Onager Roadmap

This document outlines the current status and future plans for Onager, the graph data analytics extension for DuckDB.

## Current Status: Alpha (v0.1.0)

### ✅ Implemented

#### Core Infrastructure
- [x] Rust crate structure with graphina integration
- [x] C++ DuckDB extension bindings
- [x] CMake/Makefile build system
- [x] Thread-safe graph registry

#### Graph Management
- [x] `onager_create_graph(name[, directed])` - Create named graphs
- [x] `onager_drop_graph(name)` - Drop graphs
- [x] `onager_list_graphs()` - List all graphs
- [x] `onager_add_node(graph, node_id)` - Add nodes
- [x] `onager_add_edge(graph, src, dst[, weight])` - Add weighted edges
- [x] `onager_node_count(graph)` - Node count
- [x] `onager_edge_count(graph)` - Edge count

---

### 🔜 Planned

#### v0.2.0 - Centrality Measures
- [ ] `onager_pagerank(graph)` - PageRank centrality
- [ ] `onager_betweenness(graph)` - Betweenness centrality
- [ ] `onager_closeness(graph)` - Closeness centrality
- [ ] `onager_degree(graph)` - Degree centrality
- [ ] `onager_eigenvector(graph)` - Eigenvector centrality

#### v0.3.0 - Path Finding
- [ ] `onager_shortest_path(graph, src, dst)` - Shortest path
- [ ] `onager_dijkstra(graph, src)` - All shortest paths from source
- [ ] `onager_has_path(graph, src, dst)` - Path existence check
- [ ] `onager_path_length(graph, src, dst)` - Path length

#### v0.4.0 - Community Detection
- [ ] `onager_louvain(graph)` - Louvain community detection
- [ ] `onager_label_propagation(graph)` - Label propagation
- [ ] `onager_connected_components(graph)` - Connected components

#### v0.5.0 - Graph I/O
- [ ] `onager_load_edgelist(graph, file)` - Load from edge list
- [ ] `onager_save_edgelist(graph, file)` - Save to edge list
- [ ] `onager_load_from_table(graph, table)` - Load from DuckDB table
- [ ] `onager_export_to_table(graph)` - Export to DuckDB table

#### v0.6.0 - Graph Generators
- [ ] `onager_generate_erdos_renyi(n, p)` - Erdős-Rényi random graph
- [ ] `onager_generate_barabasi_albert(n, m)` - Barabási-Albert graph
- [ ] `onager_generate_watts_strogatz(n, k, p)` - Watts-Strogatz graph
- [ ] `onager_generate_complete(n)` - Complete graph

---

## Long-term Goals

- [ ] Graph traversal iterators (BFS, DFS)
- [ ] MST algorithms (Prim, Kruskal)
- [ ] Link prediction algorithms
- [ ] Subgraph extraction
- [ ] Parallel algorithm execution
- [ ] Graph visualization (export to DOT/GraphML)
- [ ] Persistent graph storage
