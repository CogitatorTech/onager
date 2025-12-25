---
title: Graph Metrics
description: Compute structural properties of your graph.
---

# Graph Metrics

Graph metrics provide summary statistics about the overall structure of a network.
These help understand properties like how tightly connected the graph is, how clustered it is, and how node degrees are distributed.

## Setup

```sql
create table edges as
select *
from (values (1::bigint, 2::bigint),
             (1, 3),
             (2, 3),
             (2, 4),
             (3, 4),
             (3, 5),
             (4, 5),
             (5, 6)) t(src, dst);
```

---

## Diameter

The diameter is the longest shortest path between any two nodes.
It measures how "spread out" the graph is.
A small diameter means information can travel quickly across the network.

```sql
select diameter
from onager_mtr_diameter((select src, dst from edges));
```

| Column   | Type   | Description                        |
|----------|--------|------------------------------------|
| diameter | bigint | Maximum shortest path in the graph |

---

## Radius

The radius is the minimum eccentricity of any node (the smallest maximum distance from any node to all other nodes).
Nodes achieving the radius are called center nodes.

```sql
select radius
from onager_mtr_radius((select src, dst from edges));
```

| Column | Type   | Description                       |
|--------|--------|-----------------------------------|
| radius | bigint | Minimum eccentricity in the graph |

---

## Average Clustering Coefficient

Measures how much nodes tend to cluster together.
A clustering coefficient of 1.0 means every node's neighbors are all connected to each other (perfect cliques).

\[
C_i = \frac{2 \cdot |\{e_{jk}\}|}{k_i(k_i - 1)}
\]

where \(k_i\) is the degree of node \(i\) and \(|\{e_{jk}\}|\) is the number of edges between its neighbors.

```sql
select round(avg_clustering, 4) as clustering
from onager_mtr_avg_clustering((select src, dst from edges));
```

| Column         | Type   | Description                          |
|----------------|--------|--------------------------------------|
| avg_clustering | double | Average local clustering coefficient |

---

## Transitivity

Global clustering coefficient. The ratio of triangles to connected triples.
Measures the overall tendency of the graph to form triangles.

\[
T = \frac{3 \times \text{triangles}}{\text{connected triples}}
\]

```sql
select round(transitivity, 4) as transitivity
from onager_mtr_transitivity((select src, dst from edges));
```

| Column       | Type   | Description                      |
|--------------|--------|----------------------------------|
| transitivity | double | Fraction of triangles to triples |

---

## Triangle Count

Counts the number of triangles each node participates in.
Triangles indicate strong local connectivity and are the basis for clustering coefficients.

```sql
select node_id, triangles
from onager_mtr_triangles((select src, dst from edges))
where triangles > 0
order by triangles desc;
```

| Column    | Type   | Description                         |
|-----------|--------|-------------------------------------|
| node_id   | bigint | Node identifier                     |
| triangles | bigint | Number of triangles containing node |

---

## Assortativity

Degree assortativity measures whether nodes tend to connect to nodes with similar degree.
Positive values mean high-degree nodes connect to other high-degree nodes (assortative).
Negative values mean high-degree nodes connect to low-degree nodes (disassortative).

```sql
select round(assortativity, 4) as assortativity
from onager_mtr_assortativity((select src, dst from edges));
```

| Column        | Type   | Description                    |
|---------------|--------|--------------------------------|
| assortativity | double | Degree correlation coefficient |

---

## Average Path Length

The average shortest path distance between all pairs of reachable nodes.
Lower values indicate a more tightly connected network where information can spread quickly.

```sql
select round(avg_path_length, 4) as avg_path
from onager_mtr_avg_path_length((select src, dst from edges));
```

| Column          | Type   | Description                         |
|-----------------|--------|-------------------------------------|
| avg_path_length | double | Mean shortest path across all pairs |

---

## Graph Density

The fraction of possible edges that actually exist.
A density of 1.0 means a complete graph (all possible edges exist), while 0.0 means no edges.

```sql
select round(density, 4) as density
from onager_mtr_density((select src, dst from edges));

-- For directed graphs
select round(density, 4) as density
from onager_mtr_density((select src, dst from edges), directed := true);
```

| Column  | Type   | Description               |
|---------|--------|---------------------------|
| density | double | Graph density (0.0 to 1.0) |

Optional parameters:

- `directed` (default false): Treat graph as directed

---

## Complete Example: Network Health Report

Generate a comprehensive report of network properties:

```sql
create table network as
select *
from (values (1, 2),
             (1, 3),
             (2, 3),
             (2, 4),
             (3, 4),
             (3, 5),
             (4, 5),
             (4, 6),
             (5, 6),
             (6, 7),
             (7, 8),
             (8, 9)) t(src, dst);

-- All metrics in one query
select (select diameter from onager_mtr_diameter((select src, dst from network)))                       as diameter,
       (select radius from onager_mtr_radius((select src, dst from network)))                           as radius,
       (select round(avg_clustering, 3) from onager_mtr_avg_clustering((select src, dst from network))) as clustering,
       (select round(transitivity, 3) from onager_mtr_transitivity((select src, dst from network)))     as transitivity,
       (select round(assortativity, 3)
        from onager_mtr_assortativity((select src, dst from network)))                                  as assortativity;

-- Find the most clustered nodes
select node_id, triangles
from onager_mtr_triangles((select src, dst from network))
order by triangles desc limit 5;
```
