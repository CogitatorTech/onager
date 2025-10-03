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

Onager is a DuckDB extension that enables graph data analytics directly within SQL queries.
It provides graph creation, manipulation, and analysis capabilities powered by the
[graphina](https://github.com/habedi/graphina) graph data science library.

### Features

- **Graph Storage**: Create and manage named in-memory graphs
- **Node/Edge Management**: Add nodes and weighted edges to graphs
- **Centrality Measures**: PageRank, betweenness, closeness, and more (coming soon)
- **Community Detection**: Louvain, label propagation (coming soon)
- **Path Finding**: Shortest path algorithms (coming soon)
- **Graph Generators**: Random graph generation (coming soon)

> [!IMPORTANT]
> Onager is in early development, so bugs and breaking changes are expected.
> Please use the [issues page](https://github.com/CogitatorTech/onager/issues) to report bugs or request features.

---

### Quickstart

#### Build from Source

1. Clone the repository and initialize submodules:

```bash
git clone --recursive https://github.com/CogitatorTech/onager.git
cd onager
```

2. Build the extension:

```bash
# This may take a while on first run
make release
```

3. Start DuckDB shell with Onager:

```bash
./build/release/duckdb
```

#### Basic Usage

```sql
-- Get extension version
SELECT onager_version();

-- Create a new undirected graph
SELECT onager_create_graph('social_network');

-- Add nodes (node IDs 0, 1, 2)
SELECT onager_add_node('social_network', 0);
SELECT onager_add_node('social_network', 1);
SELECT onager_add_node('social_network', 2);

-- Add edges with weights
SELECT onager_add_edge('social_network', 0, 1, 1.0);
SELECT onager_add_edge('social_network', 1, 2, 2.5);
SELECT onager_add_edge('social_network', 0, 2, 1.5);

-- Get graph statistics
SELECT onager_node_count('social_network');
SELECT onager_edge_count('social_network');

-- List all graphs
SELECT onager_list_graphs();

-- Drop a graph
SELECT onager_drop_graph('social_network');
```

---

### SQL Functions

| Function                                     | Description                                 |
|----------------------------------------------|---------------------------------------------|
| `onager_version()`                           | Returns extension version                   |
| `onager_create_graph(name[, directed])`      | Creates a new graph (undirected by default) |
| `onager_drop_graph(name)`                    | Drops an existing graph                     |
| `onager_list_graphs()`                       | Returns JSON array of all graph names       |
| `onager_add_node(graph, node_id)`            | Adds a node to the graph                    |
| `onager_add_edge(graph, src, dst[, weight])` | Adds an edge (weight defaults to 1.0)       |
| `onager_node_count(graph)`                   | Returns number of nodes                     |
| `onager_edge_count(graph)`                   | Returns number of edges                     |
| `onager_last_error()`                        | Returns last error message or NULL          |

---

### Documentation

See the [ROADMAP.md](ROADMAP.md) for planned features and development status.

---

### Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to make a contribution.

### License

Onager is available under either of the following licenses:

* MIT License ([LICENSE-MIT](LICENSE-MIT))
* Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE))

### Acknowledgements

* Graph algorithms powered by [graphina](https://github.com/habedi/graphina)
