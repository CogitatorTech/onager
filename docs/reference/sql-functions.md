---
title: SQL Function Reference
description: Complete reference for all Onager SQL functions.
---

# SQL Function Reference

Complete reference for all Onager SQL functions.

## Graph Management Functions

| Function                                   | Returns   | Description                      |
|--------------------------------------------|-----------|----------------------------------|
| `onager_create_graph(name, directed)`      | `integer` | Create a named graph (0=success) |
| `onager_drop_graph(name)`                  | `integer` | Delete a named graph             |
| `onager_add_node(graph, node_id)`          | `integer` | Add a node to graph              |
| `onager_add_edge(graph, src, dst, weight)` | `integer` | Add a weighted edge              |
| `onager_list_graphs()`                     | `varchar` | List all graphs (JSON array)     |
| `onager_node_count(graph)`                 | `bigint`  | Count nodes in graph             |
| `onager_edge_count(graph)`                 | `bigint`  | Count edges in graph             |

## Scalar Query Functions

| Function                              | Returns  | Description          |
|---------------------------------------|----------|----------------------|
| `onager_node_in_degree(graph, node)`  | `bigint` | In-degree of a node  |
| `onager_node_out_degree(graph, node)` | `bigint` | Out-degree of a node |

## Centrality Functions

| Function                                     | Returns                          | Description                    |
|----------------------------------------------|----------------------------------|--------------------------------|
| `onager_ctr_pagerank(edges)`                 | `node_id, rank`                  | PageRank centrality            |
| `onager_ctr_degree(edges)`                   | `node_id, in_degree, out_degree` | Degree centrality              |
| `onager_ctr_betweenness(edges)`              | `node_id, betweenness`           | Betweenness centrality         |
| `onager_ctr_closeness(edges)`                | `node_id, closeness`             | Closeness centrality           |
| `onager_ctr_eigenvector(edges)`              | `node_id, eigenvector`           | Eigenvector centrality         |
| `onager_ctr_katz(edges, alpha)`              | `node_id, katz`                  | Katz centrality                |
| `onager_ctr_harmonic(edges)`                 | `node_id, harmonic`              | Harmonic centrality            |
| `onager_ctr_personalized_pagerank(...)`      | `node_id, score`                 | Personalized PageRank          |
| `onager_ctr_voterank(edges, num_seeds)`      | `node_id`                        | VoteRank influential spreaders |
| `onager_ctr_local_reaching(edges, distance)` | `node_id, centrality`            | Local reaching centrality      |
| `onager_ctr_laplacian(edges)`                | `node_id, centrality`            | Laplacian centrality           |

## Community Detection Functions

| Function                                       | Returns              | Description                     |
|------------------------------------------------|----------------------|---------------------------------|
| `onager_cmm_louvain(edges [, seed])`           | `node_id, community` | Louvain modularity optimization |
| `onager_cmm_components(edges)`                 | `node_id, component` | Connected components            |
| `onager_cmm_label_prop(edges)`                 | `node_id, label`     | Label propagation               |
| `onager_cmm_girvan_newman(edges, communities)` | `node_id, community` | Girvan-Newman edge betweenness  |
| `onager_cmm_spectral(edges, k)`                | `node_id, community` | Spectral clustering             |
| `onager_cmm_infomap(edges)`                    | `node_id, community` | Infomap community detection     |

## Link Prediction Functions

| Function                             | Returns                 | Description             |
|--------------------------------------|-------------------------|-------------------------|
| `onager_lnk_jaccard(edges)`          | `node1, node2, coefficient` | Jaccard coefficient     |
| `onager_lnk_adamic_adar(edges)`      | `node1, node2, score`   | Adamic-Adar index       |
| `onager_lnk_pref_attach(edges)`      | `node1, node2, score`   | Preferential attachment |
| `onager_lnk_resource_alloc(edges)`   | `node1, node2, score`   | Resource allocation     |
| `onager_lnk_common_neighbors(edges)` | `node1, node2, count`   | Common neighbors count  |

## Metric Functions

| Function                            | Returns              | Description                      |
|-------------------------------------|----------------------|----------------------------------|
| `onager_mtr_diameter(edges)`        | `diameter`           | Graph diameter                   |
| `onager_mtr_radius(edges)`          | `radius`             | Graph radius                     |
| `onager_mtr_avg_clustering(edges)`  | `avg_clustering`     | Average clustering coefficient   |
| `onager_mtr_avg_path_length(edges)` | `avg_path_length`    | Average shortest path length     |
| `onager_mtr_transitivity(edges)`    | `transitivity`       | Global clustering (transitivity) |
| `onager_mtr_triangles(edges)`       | `node_id, triangles` | Triangle count per node          |
| `onager_mtr_assortativity(edges)`   | `assortativity`      | Degree assortativity coefficient |
| `onager_mtr_density(edges)`         | `density`            | Graph density (0 to 1)           |

## Path and Traversal Functions

| Function                                          | Returns              | Description                       |
|---------------------------------------------------|----------------------|-----------------------------------|
| `onager_pth_dijkstra(edges, source)`              | `node_id, distance`  | Shortest paths from source        |
| `onager_pth_bellman_ford(weighted_edges, source)` | `node_id, distance`  | Shortest paths (negative weights) |
| `onager_pth_floyd_warshall(weighted_edges)`       | `src, dst, distance` | All-pairs shortest paths          |
| `onager_trv_bfs(edges, source)`                   | `node_id`            | Breadth-first traversal           |
| `onager_trv_dfs(edges, source)`                   | `node_id`            | Depth-first traversal             |

## Approximation Functions

| Function                            | Returns          | Description                      |
|-------------------------------------|------------------|----------------------------------|
| `onager_apx_max_clique(edges)`      | `node_id`        | Maximum clique (approximation)   |
| `onager_apx_independent_set(edges)` | `node_id`        | Maximum independent set (approx) |
| `onager_apx_vertex_cover(edges)`    | `node_id`        | Minimum vertex cover (approx)    |
| `onager_apx_tsp(weighted_edges)`    | `order, node_id` | TSP tour (greedy approx)         |

## Minimum Spanning Tree

| Function                             | Returns            | Description   |
|--------------------------------------|--------------------|---------------|
| `onager_mst_kruskal(weighted_edges)` | `src, dst, weight` | Kruskal's MST |

## Generator Functions

| Function                                         | Returns    | Description           |
|--------------------------------------------------|------------|-----------------------|
| `onager_gen_erdos_renyi(n, p [, seed])`          | `src, dst` | Random graph (G(n,p)) |
| `onager_gen_barabasi_albert(n, m [, seed])`      | `src, dst` | Scale-free graph      |
| `onager_gen_watts_strogatz(n, k, beta [, seed])` | `src, dst` | Small-world graph     |

## Subgraph Operations

| Function                                      | Returns    | Description             |
|-----------------------------------------------|------------|-------------------------|
| `onager_sub_ego_graph(edges, center, radius)` | `src, dst` | Ego graph around a node |
| `onager_sub_k_hop(edges, start, k)`           | `node_id`  | Nodes within k hops     |
| `onager_sub_induced(edges, nodes)`            | `src, dst` | Induced subgraph        |

## Parallel Algorithms

| Function                                   | Returns                | Description                      |
|--------------------------------------------|------------------------|----------------------------------|
| `onager_par_pagerank(edges)`               | `node_id, rank`        | Parallel PageRank                |
| `onager_par_bfs(edges, source)`            | `node_id`              | Parallel BFS traversal           |
| `onager_par_shortest_paths(edges, source)` | `node_id, distance`    | Parallel shortest paths          |
| `onager_par_components(edges)`             | `node_id, component`   | Parallel connected components    |
| `onager_par_clustering(edges)`             | `node_id, coefficient` | Parallel clustering coefficients |
| `onager_par_triangles(edges)`              | `node_id, triangles`   | Parallel triangle count          |

## Utility Functions

| Function              | Returns   | Description        |
|-----------------------|-----------|--------------------|
| `onager_version()`    | `varchar` | Extension version  |
| `onager_last_error()` | `varchar` | Last error message |

See [Input Formats](input-formats.md) for details on how to pass graph data to functions.
