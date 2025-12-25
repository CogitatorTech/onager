## Feature Roadmap

This document includes the roadmap for the Onager DuckDB extension.
It outlines features to be implemented and their current status.

> [!IMPORTANT]
> This roadmap is a work in progress and is subject to change.

### 1. Centrality Algorithms

* [x] PageRank centrality
* [x] Personalized PageRank
* [x] Degree centrality (in/out)
* [x] Betweenness centrality
* [x] Closeness centrality
* [x] Harmonic centrality
* [x] Eigenvector centrality
* [x] Katz centrality
* [x] VoteRank (influential spreaders)
* [x] Local reaching centrality
* [x] Laplacian centrality

### 2. Community Detection

* [x] Louvain modularity optimization
* [x] Connected components
* [x] Label propagation
* [x] Girvan-Newman edge betweenness
* [x] Spectral clustering
* [x] Infomap community detection
* [ ] Hierarchical clustering
* [ ] K-core decomposition

### 3. Path and Traversal Algorithms

* [x] Dijkstra shortest paths
* [x] Bellman-Ford shortest paths
* [x] Breadth-first search (BFS)
* [x] Depth-first search (DFS)
* [x] Floyd-Warshall all-pairs shortest paths
* [ ] Bidirectional search

### 4. Graph Metrics

* [x] Graph diameter
* [x] Graph radius
* [x] Average clustering coefficient
* [x] Transitivity (global clustering)
* [x] Triangle count per node
* [x] Assortativity coefficient
* [x] Average path length
* [x] Graph density

### 5. Subgraph Operations

* [x] Ego graph extraction
* [x] K-hop neighbors
* [x] Induced subgraph
* [ ] Subgraph isomorphism
* [ ] Motif detection

### 6. Approximation Algorithms

* [x] Maximum clique approximation
* [x] Maximum independent set approximation
* [x] Minimum vertex cover approximation
* [x] Traveling salesman approximation
* [ ] Graph coloring approximation

### 7. Parallel Algorithms

* [x] Parallel PageRank
* [x] Parallel BFS
* [x] Parallel shortest paths
* [x] Parallel connected components
* [x] Parallel clustering coefficients
* [x] Parallel triangle counting

### 8. Graph Generators

* [x] Erdős-Rényi random graphs
* [x] Barabási-Albert scale-free graphs
* [x] Watts-Strogatz small-world graphs
* [ ] Random regular graphs
* [ ] Stochastic block models

### 9. Minimum Spanning Tree

* [x] Kruskal's algorithm
* [x] Prim's algorithm

### 10. Link Prediction

* [x] Jaccard coefficient
* [x] Adamic-Adar index
* [x] Preferential attachment
* [x] Resource allocation
* [x] Common neighbors
* [ ] Katz similarity

### 11. Documentation and Testing

* [x] SQL function reference
* [x] User guide documentation
* [x] SQL integration tests
* [x] Rust unit tests
* [ ] Performance benchmarks
* [ ] Example notebooks
