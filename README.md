<div align="center">
  <picture>
    <img alt="Onager Logo" src="logo.svg" height="25%" width="25%">
  </picture>
<br>

<h2>Onager</h2>

[![Tests](https://img.shields.io/github/actions/workflow/status/CogitatorTech/onager/tests.yml?label=tests&style=flat&labelColor=282c34&logo=github)](https://github.com/CogitatorTech/onager/actions/workflows/tests.yml)
[![Code Quality](https://img.shields.io/codefactor/grade/github/CogitatorTech/onager?label=code%20quality&style=flat&labelColor=282c34&logo=codefactor)](https://www.codefactor.io/repository/github/CogitatorTech/onager)
[![Docs](https://img.shields.io/badge/docs-read-blue?style=flat&labelColor=282c34&logo=read-the-docs)](https://cogitatortech.github.io/onager/)
[![License](https://img.shields.io/badge/license-MIT%2FApache--2.0-007ec6?style=flat&labelColor=282c34&logo=open-source-initiative)](https://github.com/CogitatorTech/onager)

A Graph Analytics Toolbox for DuckDB

</div>

---

Onager is a DuckDB extension that adds a large number of graph analytics functions to DuckDB,
including centrality measures, community detection algorithms, pathfinding algorithms, graph metrics,
and graph generators.
Onager is written in Rust and uses [Graphina](https://github.com/habedi/graphina) graph library under the hood.

Compared to [DuckPGQ](https://github.com/cwida/duckpgq-extension), Onager is focused on graph analytics instead of graph querying.
More specifically, DuckPGQ tries to implement [SQL/PGQ](https://pgql-lang.org/) (the SQL:2023 standard)
for graph pattern matching and path-finding queries with a property graph model.
Onager instead provides a collection of ready-to-use graph algorithms (like PageRank, Louvain community detection, Dijkstra's shortest path, etc.)
as simple table functions.
Users typically want something like DuckPGQ when they need to query graph patterns or model the data in a property graph model
(like a graph database), and use Onager when they need to run specific graph algorithms on their graph data for a specific application.

### Features

- Adds over 40 graph algorithms as SQL table functions
- Provides a simple and uniform API
- Supports both directed and undirected graphs
- Supports weighted and unweighted edges
- Includes multi-threaded algorithm implementations

See [ROADMAP.md](ROADMAP.md) for the list of implemented and planned features.

> [!IMPORTANT]
> Onager is in early development, so bugs and breaking changes are expected.
> Please use the [issues page](https://github.com/CogitatorTech/onager/issues) to report bugs or request features.

---

### Quickstart

#### Install from Community Extensions Repository

You can install and load Onager from
the [DuckDB community extensions](https://duckdb.org/community_extensions/extensions/onager) repository by running the
following SQL commands in the DuckDB shell:

```sql
install onager from community;
load onager;
```

#### Build from Source

Alternatively, you can build Onager from source and use it by following these steps:

1. Clone the repository and build the Onager extension from source:

```bash
git clone --recursive https://github.com/CogitatorTech/onager.git
cd onager

# This might take a while to run
make release
```

2. Start DuckDB shell (with Onager statically linked to it):

```bash
./build/release/duckdb
```

> [!NOTE]
> After building from source, the Onager binary will be `build/release/extension/onager/onager.duckdb_extension`.
> You can load it using the `load 'build/release/extension/onager/onager.duckdb_extension';` in the DuckDB shell.
> Note that the extension binary will only work with the DuckDB version that it was built against.
> You can download the pre-built binaries from the [releases page](https://github.com/CogitatorTech/onager/releases) for
> your platform.

#### Trying Onager

```sql
-- Create an edge table
create table edges as
select * from
             (values (1::bigint, 2::bigint),
             (2, 3),
             (3, 1),
             (3, 4)) t(src, dst);

-- Compute PageRank
select * from onager_ctr_pagerank((select src, dst from edges));

-- Detect communities
select * from onager_cmm_louvain((select src, dst from edges));

-- Find shortest paths
select * from onager_pth_dijkstra((select src, dst from edges), source := 1);

-- Generate a random graph
select * from onager_gen_erdos_renyi(100, 0.1, seed := 42);
```

[![Simple Demo 1](https://asciinema.org/a/5RBqT9bO2mB28EaEn8qPesn8t.svg)](https://asciinema.org/a/5RBqT9bO2mB28EaEn8qPesn8t)

---

### Documentation

Check out the [Onager documentation](https://cogitatortech.github.io/onager/) for the API documentation and examples.

---

### Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to make a contribution.

### License

Onager is available under either of the following licenses:

* MIT License ([LICENSE-MIT](LICENSE-MIT))
* Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE))

### Acknowledgements

* The logo is from [here](https://www.svgrepo.com/svg/452885/donkey) with some modifications.
* This project uses [Graphina](https://github.com/habedi/graphina) graph library.
