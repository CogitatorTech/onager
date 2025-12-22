# Link Prediction Example

## Friend Recommendations

```sql
-- Current friendships
create table current_friends as select * from (values
  (1::bigint, 2::bigint), (1, 3), (2, 3), (2, 4),
  (3, 5), (4, 5), (4, 6), (5, 6)
) t(user1, user2);

-- Predict new friendships using Adamic-Adar
select node1, node2, score
from onager_lnk_adamic_adar((select user1, user2 from current_friends))
where score > 0
order by score desc
limit 10;
```

## Comparing Prediction Methods

```sql
with
  aa as (select node1, node2, score as adamic_adar from onager_lnk_adamic_adar((select user1, user2 from current_friends))),
  jc as (select node1, node2, jaccard from onager_lnk_jaccard((select user1, user2 from current_friends))),
  pa as (select node1, node2, score as pref_attach from onager_lnk_pref_attach((select user1, user2 from current_friends)))
select aa.node1, aa.node2, aa.adamic_adar, jc.jaccard, pa.pref_attach
from aa
join jc on aa.node1 = jc.node1 and aa.node2 = jc.node2
join pa on aa.node1 = pa.node1 and aa.node2 = pa.node2
order by aa.adamic_adar desc
limit 10;
```
