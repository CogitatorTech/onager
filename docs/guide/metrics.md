# Graph Metrics

Graph-level metrics that describe the overall structure of the network.

## Diameter

The longest shortest path in the graph.

```sql
select * from onager_mtr_diameter((select src, dst from edges));
```

Returns a single row with the diameter value.

## Radius

The minimum eccentricity (distance to furthest node) across all nodes.

```sql
select * from onager_mtr_radius((select src, dst from edges));
```

## Average Clustering Coefficient

Measures how much nodes tend to cluster together.

```sql
select * from onager_mtr_avg_clustering((select src, dst from edges));
```

Returns a value between 0 and 1. Higher values indicate more clustering.

## Average Path Length

The average shortest path distance between all pairs of nodes.

```sql
select * from onager_mtr_avg_path_length((select src, dst from edges));
```

## Transitivity

Global clustering coefficient — ratio of triangles to connected triples.

```sql
select * from onager_mtr_transitivity((select src, dst from edges));
```

## Example: Network Summary

```sql
select
  (select diameter from onager_mtr_diameter((select src, dst from edges))) as diameter,
  (select radius from onager_mtr_radius((select src, dst from edges))) as radius,
  (select avg_clustering from onager_mtr_avg_clustering((select src, dst from edges))) as clustering,
  (select transitivity from onager_mtr_transitivity((select src, dst from edges))) as transitivity;
```
