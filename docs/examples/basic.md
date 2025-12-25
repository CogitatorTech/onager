# Basic Usage

This example shows basic graph analytics workflows using the Onager.

## Setup

```sql
-- Create a sample social network
create table social_edges as
select *
from (values (1::bigint, 2::bigint),
             (1, 3),
             (2, 3),
             (2, 4),
             (3, 4),
             (3, 5),
             (4, 5),
             (5, 6)) t(user_from, user_to);
```

## Find Key Influencers

```sql
select *
from onager_ctr_pagerank((select user_from, user_to from social_edges))
order by rank desc;
```

## Detect Communities

```sql
select *
from onager_cmm_louvain((select user_from, user_to from social_edges));
```

## Check Network Connectivity

```sql
-- How many connected components?
select count(distinct component) as num_components
from onager_cmm_components((select user_from, user_to from social_edges));
```

## Measure Network Structure

```sql
select diameter,
       (select avg_clustering
        from onager_mtr_avg_clustering((select user_from, user_to from social_edges))) as clustering
from onager_mtr_diameter((select user_from, user_to from social_edges));
```

## Generate Test Data

```sql
-- Create a larger random network for testing
create table test_network as
select *
from onager_gen_erdos_renyi(500, 0.02, seed := 42);

-- Analyze it
select count(*) as edge_count
from test_network;
```
