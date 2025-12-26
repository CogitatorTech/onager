---
title: Community Detection Example
description: Segmenting users into groups.
---

# Community Detection Example

## Segmenting Users

```sql
-- Social connections
create table friendships as select * from (values
  -- Group 1: Tight-knit friends
  (1::bigint, 2::bigint), (1, 3), (2, 3),
  -- Group 2: Work colleagues  
  (4::bigint, 5::bigint), (4, 6), (5, 6), (5, 7), (6, 7),
  -- Bridge between groups
  (3, 4)
) t(user1, user2);

-- Detect communities
select * from onager_cmm_louvain((select user1, user2 from friendships))
order by community, node_id;
```

## Analyzing Community Structure

```sql
with communities as (
  select * from onager_cmm_louvain((select user1, user2 from friendships))
)
select
  community,
  count(*) as members,
  array_agg(node_id order by node_id) as member_list
from communities
group by community
order by members desc;
```

## Label Propagation (Faster Alternative)

```sql
select * from onager_cmm_label_prop((select user1, user2 from friendships));
```
