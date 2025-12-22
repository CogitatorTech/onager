# Community Detection Example

## Segmenting Users

```sql
-- Social connections
CREATE TABLE friendships AS SELECT * FROM (VALUES 
  -- Group 1: Tight-knit friends
  (1::BIGINT, 2::BIGINT), (1, 3), (2, 3),
  -- Group 2: Work colleagues  
  (4::BIGINT, 5::BIGINT), (4, 6), (5, 6), (5, 7), (6, 7),
  -- Bridge between groups
  (3, 4)
) t(user1, user2);

-- Detect communities
SELECT * FROM onager_cmm_louvain((SELECT user1, user2 FROM friendships))
ORDER BY community, node_id;
```

## Analyzing Community Structure

```sql
WITH communities AS (
  SELECT * FROM onager_cmm_louvain((SELECT user1, user2 FROM friendships))
)
SELECT 
  community,
  COUNT(*) as members,
  ARRAY_AGG(node_id ORDER BY node_id) as member_list
FROM communities
GROUP BY community
ORDER BY members DESC;
```

## Label Propagation (Faster Alternative)

```sql
SELECT * FROM onager_cmm_label_prop((SELECT user1, user2 FROM friendships));
```
