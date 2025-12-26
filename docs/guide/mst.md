---
title: Minimum Spanning Tree
description: Find the minimum-weight tree connecting all nodes.
---

# Minimum Spanning Tree

Minimum Spanning Tree (MST) algorithms find a tree that connects all nodes with minimum total edge weight.

## Setup

```sql
-- Create weighted edges
create table weighted_edges as select * from (values
  (1::bigint, 2::bigint, 1.0::double),
  (1, 3, 3.0), (2, 3, 2.0), (2, 4, 4.0), (3, 4, 5.0)
) t(src, dst, weight);
```

---

## Kruskal's Algorithm

Classic MST algorithm that sorts edges and greedily adds the smallest ones.

```sql
select src, dst, weight
from onager_mst_kruskal((select src, dst, weight from weighted_edges))
order by weight;
```

| Column | Type   | Description       |
|--------|--------|-------------------|
| src    | bigint | Source node       |
| dst    | bigint | Destination node  |
| weight | double | Edge weight       |
