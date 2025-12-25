---
title: Approximation Algorithms
description: Fast approximations for computationally hard graph problems.
---

# Approximation Algorithms

These algorithms provide fast approximations for NP-hard graph problems.

## Setup

```sql
create table edges as select * from (values
  (1::bigint, 2::bigint), (1, 3), (2, 3), (2, 4),
  (3, 4), (3, 5), (4, 5), (5, 6)
) t(src, dst);
```

---

## Maximum Clique

Finds an approximate maximum clique (largest fully connected subgraph).

```sql
select node_id
from onager_apx_max_clique((select src, dst from edges));
```

| Column  | Type   | Description              |
|---------|--------|--------------------------|
| node_id | bigint | Node in the clique       |

---

## Maximum Independent Set

Finds an approximate maximum independent set (largest set of non-adjacent nodes).

```sql
select node_id
from onager_apx_independent_set((select src, dst from edges));
```

| Column  | Type   | Description                     |
|---------|--------|---------------------------------|
| node_id | bigint | Node in the independent set     |

---

## Minimum Vertex Cover

Finds an approximate minimum vertex cover (smallest set of nodes that covers all edges).

```sql
select node_id
from onager_apx_vertex_cover((select src, dst from edges));
```

| Column  | Type   | Description               |
|---------|--------|---------------------------|
| node_id | bigint | Node in the vertex cover  |

---

## Traveling Salesman Problem (TSP)

Finds an approximate TSP tour using a greedy algorithm. Requires weighted edges.

```sql
-- Create weighted edges
create table weighted_edges as select * from (values
  (1::bigint, 2::bigint, 1.0::double),
  (2, 3, 2.0), (3, 4, 1.5), (4, 1, 2.5)
) t(src, dst, weight);

select order_idx, node_id
from onager_apx_tsp((select src, dst, weight from weighted_edges))
order by order_idx;
```

| Column    | Type   | Description              |
|-----------|--------|--------------------------|
| order_idx | bigint | Position in the tour     |
| node_id   | bigint | Node at this position    |
