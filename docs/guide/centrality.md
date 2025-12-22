# Centrality Algorithms

Centrality measures identify the most important nodes in a graph. Onager provides 7 centrality algorithms.

## PageRank

Measures node importance based on the link structure of the graph.

```sql
select * from onager_ctr_pagerank((select src, dst from edges));
```

| Column  | Type   | Description                              |
|---------|--------|------------------------------------------|
| node_id | bigint | Node identifier                          |
| rank    | double | PageRank score (higher = more important) |

## Degree Centrality

Counts incoming and outgoing edges per node.

```sql
select * from onager_ctr_degree((select src, dst from edges));
```

| Column     | Type   | Description              |
|------------|--------|--------------------------|
| node_id    | bigint | Node identifier          |
| in_degree  | double | Number of incoming edges |
| out_degree | double | Number of outgoing edges |

## Betweenness Centrality

Measures how often a node lies on shortest paths between other nodes.

```sql
select * from onager_ctr_betweenness((select src, dst from edges));
```

## Closeness Centrality

Measures the average distance from a node to all other nodes.

```sql
select * from onager_ctr_closeness((select src, dst from edges));
```

## Eigenvector Centrality

A node is important if it's connected to other important nodes.

```sql
select * from onager_ctr_eigenvector((select src, dst from edges));
```

## Katz Centrality

Similar to eigenvector but accounts for all walks in the graph.

```sql
select * from onager_ctr_katz((select src, dst from edges));
```

## Harmonic Centrality

Variant of closeness that handles disconnected graphs better.

```sql
select * from onager_ctr_harmonic((select src, dst from edges));
```

## Example: Find Top Influencers

```sql
select * from onager_ctr_pagerank((select src, dst from edges))
order by rank desc
limit 10;
```
