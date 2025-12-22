# Community Detection

Community detection algorithms identify groups of densely connected nodes.

## Louvain Algorithm

The Louvain algorithm optimizes modularity to find communities.

```sql
SELECT * FROM onager_cmm_louvain((SELECT src, dst FROM edges));
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | BIGINT | Node identifier |
| community | BIGINT | Community ID this node belongs to |

### Optional: Set Random Seed

```sql
SELECT * FROM onager_cmm_louvain((SELECT src, dst FROM edges), seed := 42);
```

## Connected Components

Find groups of nodes that are reachable from each other.

```sql
SELECT * FROM onager_cmm_components((SELECT src, dst FROM edges));
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | BIGINT | Node identifier |
| component | BIGINT | Component ID |

## Label Propagation

Fast community detection by propagating labels through the network.

```sql
SELECT * FROM onager_cmm_label_prop((SELECT src, dst FROM edges));
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | BIGINT | Node identifier |
| label | BIGINT | Community label |

## Example: Analyze Community Sizes

```sql
WITH communities AS (
  SELECT * FROM onager_cmm_louvain((SELECT src, dst FROM edges))
)
SELECT community, COUNT(*) as size
FROM communities
GROUP BY community
ORDER BY size DESC;
```
