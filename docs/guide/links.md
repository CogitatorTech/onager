# Link Prediction

Link prediction algorithms predict which unconnected nodes are likely to become connected.

## Jaccard Coefficient

Measures similarity based on common neighbors.

```sql
SELECT * FROM onager_lnk_jaccard((SELECT src, dst FROM edges));
```

| Column | Type | Description |
|--------|------|-------------|
| node1 | BIGINT | First node |
| node2 | BIGINT | Second node |
| jaccard | DOUBLE | Jaccard similarity (0-1, higher = more similar) |

## Adamic-Adar Index

Weights common neighbors by the inverse log of their degree.

```sql
SELECT * FROM onager_lnk_adamic_adar((SELECT src, dst FROM edges));
```

## Preferential Attachment

Based on the idea that well-connected nodes attract more connections.

```sql
SELECT * FROM onager_lnk_pref_attach((SELECT src, dst FROM edges));
```

## Resource Allocation

Similar to Adamic-Adar but with inverse degree (not log).

```sql
SELECT * FROM onager_lnk_resource_alloc((SELECT src, dst FROM edges));
```

## Example: Top Predicted Links

```sql
SELECT * FROM onager_lnk_adamic_adar((SELECT src, dst FROM edges))
ORDER BY score DESC
LIMIT 20;
```
