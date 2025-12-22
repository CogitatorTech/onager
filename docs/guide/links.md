# Link Prediction

Link prediction algorithms predict which unconnected nodes are likely to become connected.

## Jaccard Coefficient

Measures similarity based on common neighbors.

```sql
select * from onager_lnk_jaccard((select src, dst from edges));
```

| Column  | Type   | Description                                     |
|---------|--------|-------------------------------------------------|
| node1   | bigint | First node                                      |
| node2   | bigint | Second node                                     |
| jaccard | double | Jaccard similarity (0-1, higher = more similar) |

## Adamic-Adar Index

Weights common neighbors by the inverse log of their degree.

```sql
select * from onager_lnk_adamic_adar((select src, dst from edges));
```

## Preferential Attachment

Based on the idea that well-connected nodes attract more connections.

```sql
select * from onager_lnk_pref_attach((select src, dst from edges));
```

## Resource Allocation

Similar to Adamic-Adar but with inverse degree (not log).

```sql
select * from onager_lnk_resource_alloc((select src, dst from edges));
```

## Example: Top Predicted Links

```sql
select * from onager_lnk_adamic_adar((select src, dst from edges))
order by score desc
limit 20;
```
