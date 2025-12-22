# Centrality Algorithms

Centrality measures identify the most important nodes in a graph. Onager provides 7 centrality algorithms.

## PageRank

Measures node importance based on the link structure of the graph.

```sql
SELECT * FROM onager_ctr_pagerank((SELECT src, dst FROM edges));
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | BIGINT | Node identifier |
| rank | DOUBLE | PageRank score (higher = more important) |

## Degree Centrality

Counts incoming and outgoing edges per node.

```sql
SELECT * FROM onager_ctr_degree((SELECT src, dst FROM edges));
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | BIGINT | Node identifier |
| in_degree | DOUBLE | Number of incoming edges |
| out_degree | DOUBLE | Number of outgoing edges |

## Betweenness Centrality

Measures how often a node lies on shortest paths between other nodes.

```sql
SELECT * FROM onager_ctr_betweenness((SELECT src, dst FROM edges));
```

## Closeness Centrality

Measures the average distance from a node to all other nodes.

```sql
SELECT * FROM onager_ctr_closeness((SELECT src, dst FROM edges));
```

## Eigenvector Centrality

A node is important if it's connected to other important nodes.

```sql
SELECT * FROM onager_ctr_eigenvector((SELECT src, dst FROM edges));
```

## Katz Centrality

Similar to eigenvector but accounts for all walks in the graph.

```sql
SELECT * FROM onager_ctr_katz((SELECT src, dst FROM edges));
```

## Harmonic Centrality

Variant of closeness that handles disconnected graphs better.

```sql
SELECT * FROM onager_ctr_harmonic((SELECT src, dst FROM edges));
```

## Example: Find Top Influencers

```sql
SELECT * FROM onager_ctr_pagerank((SELECT src, dst FROM edges))
ORDER BY rank DESC
LIMIT 10;
```
