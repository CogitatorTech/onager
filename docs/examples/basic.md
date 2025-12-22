# Basic Usage

This example shows basic graph analytics workflows.

## Setup

```sql
-- Create a sample social network
CREATE TABLE social_edges AS SELECT * FROM (VALUES 
  (1::BIGINT, 2::BIGINT),
  (1, 3),
  (2, 3),
  (2, 4),
  (3, 4),
  (3, 5),
  (4, 5),
  (5, 6)
) t(user_from, user_to);
```

## Find Key Influencers

```sql
SELECT * FROM onager_ctr_pagerank((SELECT user_from, user_to FROM social_edges))
ORDER BY rank DESC;
```

## Detect Communities

```sql
SELECT * FROM onager_cmm_louvain((SELECT user_from, user_to FROM social_edges));
```

## Check Network Connectivity

```sql
-- How many connected components?
SELECT COUNT(DISTINCT component) as num_components
FROM onager_cmm_components((SELECT user_from, user_to FROM social_edges));
```

## Measure Network Structure

```sql
SELECT 
  diameter,
  (SELECT avg_clustering FROM onager_mtr_avg_clustering((SELECT user_from, user_to FROM social_edges))) as clustering
FROM onager_mtr_diameter((SELECT user_from, user_to FROM social_edges));
```

## Generate Test Data

```sql
-- Create a larger random network for testing
CREATE TABLE test_network AS 
SELECT * FROM onager_gen_erdos_renyi(500, 0.02, seed := 42);

-- Analyze it
SELECT COUNT(*) as edge_count FROM test_network;
```
