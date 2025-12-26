---
title: Centrality Analysis Example
description: Finding influential nodes in a network.
---

# Centrality Analysis Example

## Finding Influential Nodes in a Network

```sql
-- Create a citation network
create table citations as select * from (values
  (1::bigint, 2::bigint), (1, 3), (2, 4), (2, 5),
  (3, 4), (3, 6), (4, 7), (5, 7),
  (6, 7), (7, 8), (7, 9), (8, 10), (9, 10)
) t(paper_from, paper_to);

-- PageRank: overall importance
select * from onager_ctr_pagerank((select paper_from, paper_to from citations))
order by rank desc limit 5;

-- Betweenness: information brokers
select * from onager_ctr_betweenness((select paper_from, paper_to from citations))
order by betweenness desc limit 5;

-- Degree: most connected
select * from onager_ctr_degree((select paper_from, paper_to from citations))
order by in_degree + out_degree desc limit 5;
```

## Comparing Centrality Measures

```sql
with
  pr as (select node_id, rank from onager_ctr_pagerank((select paper_from, paper_to from citations))),
  bt as (select node_id, betweenness from onager_ctr_betweenness((select paper_from, paper_to from citations))),
  dg as (select node_id, in_degree + out_degree as total_degree from onager_ctr_degree((select paper_from, paper_to from citations)))
select pr.node_id, pr.rank, bt.betweenness, dg.total_degree
from pr
join bt on pr.node_id = bt.node_id
join dg on pr.node_id = dg.node_id
order by pr.rank desc;
```
