# Link Prediction Example

## Friend Recommendations

```sql
-- Current friendships
CREATE TABLE current_friends AS SELECT * FROM (VALUES 
  (1::BIGINT, 2::BIGINT), (1, 3), (2, 3), (2, 4),
  (3, 5), (4, 5), (4, 6), (5, 6)
) t(user1, user2);

-- Predict new friendships using Adamic-Adar
SELECT node1, node2, score 
FROM onager_lnk_adamic_adar((SELECT user1, user2 FROM current_friends))
WHERE score > 0
ORDER BY score DESC
LIMIT 10;
```

## Comparing Prediction Methods

```sql
WITH 
  aa AS (SELECT node1, node2, score as adamic_adar FROM onager_lnk_adamic_adar((SELECT user1, user2 FROM current_friends))),
  jc AS (SELECT node1, node2, jaccard FROM onager_lnk_jaccard((SELECT user1, user2 FROM current_friends))),
  pa AS (SELECT node1, node2, score as pref_attach FROM onager_lnk_pref_attach((SELECT user1, user2 FROM current_friends)))
SELECT aa.node1, aa.node2, aa.adamic_adar, jc.jaccard, pa.pref_attach
FROM aa
JOIN jc ON aa.node1 = jc.node1 AND aa.node2 = jc.node2
JOIN pa ON aa.node1 = pa.node1 AND aa.node2 = pa.node2
ORDER BY aa.adamic_adar DESC
LIMIT 10;
```
