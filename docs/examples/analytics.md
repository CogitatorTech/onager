---
title: Analytics Examples
description: Real-world graph analytics use cases with Onager.
---

# Graph Analytics Examples

## Social Network Analysis

### Finding Influencers

```sql
-- Create social network edges
create or replace table follows as select * from (values
  (1::bigint, 2::bigint), (1, 3), (2, 3), (3, 4), (4, 5), (5, 1), (2, 4), (3, 5)
) t(follower, followed);

-- Find top influencers by PageRank
select node_id as user_id, rank
from onager_ctr_pagerank((select follower as src, followed as dst from follows))
order by rank desc
limit 5;

-- Weight edges by interaction count so frequent interactions matter more
create or replace table interactions as select * from (values
  (1::bigint, 2::bigint, 12.0::double), (1, 3, 3.0), (2, 3, 7.0), (3, 4, 1.0),
  (4, 5, 9.0), (5, 1, 2.0), (2, 4, 4.0), (3, 5, 6.0)
) t(src, dst, msg_count);

select node_id as user_id, rank
from onager_ctr_pagerank((select src, dst, msg_count as weight from interactions))
order by rank desc
limit 5;

-- Combine multiple centrality metrics
with centralities as (
  select p.node_id, p.rank as pagerank, d.in_degree
  from onager_ctr_pagerank((select follower as src, followed as dst from follows)) p
  join onager_ctr_degree((select follower as src, followed as dst from follows)) d
    on p.node_id = d.node_id
)
select *, (pagerank + in_degree/10) as combined_score
from centralities
order by combined_score desc;
```

### Community Detection

```sql
-- Detect communities with Louvain
select node_id as user_id, community
from onager_cmm_louvain((select follower as src, followed as dst from follows));

-- Count users per community
select community, count(*) as size
from onager_cmm_louvain((select follower as src, followed as dst from follows))
group by community
order by size desc;
```

---

## Fraud Detection

### Ring Detection With Clustering

```sql
create or replace table transactions as select * from (values
  (100::bigint, 200::bigint), (200, 300), (300, 100), -- Triangle (suspicious)
  (400, 500), (500, 600)                              -- Normal chain
) t(sender, receiver);

-- Find nodes with high clustering (potential fraud rings)
select node_id as account, coefficient
from onager_par_clustering((select sender as src, receiver as dst from transactions))
where coefficient > 0.5;

-- Count triangles per account
select node_id as account, triangles
from onager_par_triangles((select sender as src, receiver as dst from transactions))
where triangles > 0;
```

---

## Recommendation Systems

### Neighborhood-Based Recommendations

```sql
-- User interaction graph
create or replace table interactions as select * from (values
  (1::bigint, 10::bigint), (1, 20), (2, 10), (2, 30), (3, 20), (3, 30)
) t(user_id, item_id);

-- Create edges (user-item bipartite graph)
create or replace table edges as
  select user_id as src, item_id + 1000 as dst from interactions;

-- Recommend items for user 1 based on their interactions
-- Using ego graph to explore local neighborhood
select *
from onager_sub_k_hop((select src, dst from edges), start := 1::bigint, k := 2)
where node_id > 1000;  -- Filter to items only
```

### Personalized PageRank for Recommendations

```sql
-- Recommend items for user 1 using Personalized PageRank
-- We personalize the random walks on user 1
select node_id - 1000 as item_id, score
from onager_ctr_personalized_pagerank((
  select e.src, e.dst, p.node as pers_node, p.weight::double as pers_weight
  from edges e
  cross join (values (1::bigint, 1.0::double)) p(node, weight)
))
where node_id > 1000  -- Filter to items only
order by score desc;
```

---

## Network Health Metrics

### Detailed Network Summary

```sql
-- Get network health metrics
select 'Diameter' as metric, diameter as value
from onager_mtr_diameter((select src, dst from edges))
union all
select 'Radius', radius
from onager_mtr_radius((select src, dst from edges))
union all
select 'Avg Clustering', avg_clustering
from onager_mtr_avg_clustering((select src, dst from edges))
union all
select 'Transitivity', transitivity
from onager_mtr_transitivity((select src, dst from edges))
union all
select 'Assortativity', assortativity
from onager_mtr_assortativity((select src, dst from edges));
```
