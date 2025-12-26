---
title: Link Prediction
description: Predict missing or future edges in your graph.
---

# Link Prediction

Link prediction algorithms estimate the likelihood of edges that don't currently exist.
These are useful for recommender systems, predicting future connections, or finding missing links in incomplete data.

!!! warning "Performance"
    Link prediction functions compute scores for all node pairs. That can produce O(n²) rows in the result set. So, it's recommended to use them only on smaller graphs (like with fewer than 1,000 nodes) and filter the results aggressively.

## Setup

```sql
create table edges as
select *
from (values (1::bigint, 2::bigint),
             (1, 3),
             (2, 3),
             (2, 4),
             (3, 4),
             (4, 5)) t(src, dst);
```

---

## Jaccard Coefficient

The ratio of common neighbors to total neighbors of two nodes.
Higher values indicate nodes are more similar and more likely to connect.

\[
Jaccard(u, v) = \frac{|N(u) \cap N(v)|}{|N(u) \cup N(v)|}
\]

```sql
select node1, node2, round(coefficient, 4) as jaccard
from onager_lnk_jaccard((select src, dst from edges))
where coefficient > 0
order by coefficient desc limit 10;
```

| Column      | Type   | Description                 |
|-------------|--------|-----------------------------|
| node1       | bigint | First node                  |
| node2       | bigint | Second node                 |
| coefficient | double | Jaccard similarity (0 to 1) |

---

## Adamic-Adar Index

Similar to common neighbors but gives more weight to rare neighbors.
A common neighbor with few connections is more significant than one with many.

\[
AA(u, v) = \sum_{z \in N(u) \cap N(v)} \frac{1}{\log|N(z)|}
\]

```sql
select node1, node2, round(score, 4) as adamic_adar
from onager_lnk_adamic_adar((select src, dst from edges))
where score > 0
order by score desc limit 10;
```

---

## Preferential Attachment

Based on the idea that nodes with many connections are likely to get more (the "rich get richer").
Simply the product of the degrees of two nodes.

\[
PA(u, v) = |N(u)| \times |N(v)|
\]

```sql
select node1, node2, score as pref_attach
from onager_lnk_pref_attach((select src, dst from edges))
order by score desc limit 10;
```

---

## Resource Allocation

Similar to Adamic-Adar but uses inverse degree instead of inverse log-degree.
Models resource flow through common neighbors.

\[
RA(u, v) = \sum_{z \in N(u) \cap N(v)} \frac{1}{|N(z)|}
\]

```sql
select node1, node2, round(score, 4) as resource_alloc
from onager_lnk_resource_alloc((select src, dst from edges))
where score > 0
order by score desc limit 10;
```

---

## Complete Example: Friend Recommendations

Find potential connections in a social network:

```sql
-- Sample social network
create table friends as
select *
from (values (1::bigint, 2::bigint),
             (1, 3),
             (1, 4),
             (2, 3),
             (2, 5),
             (3, 4),
             (3, 5),
             (4, 6),
             (5, 6),
             (5, 7),
             (6, 7),
             (6, 8)) t(user_id, friend_id);

-- Find non-connected pairs with high similarity
with existing as (select user_id as n1, friend_id as n2
                  from friends
                  union
                  select friend_id, user_id
                  from friends),
     predictions as (select node1, node2, score
                     from onager_lnk_adamic_adar((select user_id as src, friend_id as dst from friends))
                     where score > 0)
select p.node1 as user, p.node2 as suggested_friend, round(p.score, 3) as score
from predictions p
    left join existing e
on p.node1 = e.n1 and p.node2 = e.n2
where e.n1 is null -- not already friends
  and p.node1
    < p.node2      -- avoid duplicates
order by p.score desc
    limit 5;
```
