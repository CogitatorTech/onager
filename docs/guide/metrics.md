# Graph Metrics

Graph-level metrics that describe the overall structure of the network.

## Diameter

The longest shortest path in the graph.

```sql
SELECT * FROM onager_mtr_diameter((SELECT src, dst FROM edges));
```

Returns a single row with the diameter value.

## Radius

The minimum eccentricity (distance to furthest node) across all nodes.

```sql
SELECT * FROM onager_mtr_radius((SELECT src, dst FROM edges));
```

## Average Clustering Coefficient

Measures how much nodes tend to cluster together.

```sql
SELECT * FROM onager_mtr_avg_clustering((SELECT src, dst FROM edges));
```

Returns a value between 0 and 1. Higher values indicate more clustering.

## Average Path Length

The average shortest path distance between all pairs of nodes.

```sql
SELECT * FROM onager_mtr_avg_path_length((SELECT src, dst FROM edges));
```

## Transitivity

Global clustering coefficient — ratio of triangles to connected triples.

```sql
SELECT * FROM onager_mtr_transitivity((SELECT src, dst FROM edges));
```

## Example: Network Summary

```sql
SELECT 
  (SELECT diameter FROM onager_mtr_diameter((SELECT src, dst FROM edges))) as diameter,
  (SELECT radius FROM onager_mtr_radius((SELECT src, dst FROM edges))) as radius,
  (SELECT avg_clustering FROM onager_mtr_avg_clustering((SELECT src, dst FROM edges))) as clustering,
  (SELECT transitivity FROM onager_mtr_transitivity((SELECT src, dst FROM edges))) as transitivity;
```
