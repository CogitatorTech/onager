# Community Detection

Community detection algorithms identify groups of densely connected nodes.

## Louvain Algorithm

The Louvain algorithm optimizes modularity to find communities.

```sql
select * from onager_cmm_louvain((select src, dst from edges));
```

| Column    | Type   | Description                       |
|-----------|--------|-----------------------------------|
| node_id   | bigint | Node identifier                   |
| community | bigint | Community ID this node belongs to |

### Optional: Set Random Seed

```sql
select * from onager_cmm_louvain((select src, dst from edges), seed := 42);
```

## Connected Components

Find groups of nodes that are reachable from each other.

```sql
select * from onager_cmm_components((select src, dst from edges));
```

| Column    | Type   | Description     |
|-----------|--------|-----------------|
| node_id   | bigint | Node identifier |
| component | bigint | Component ID    |

## Label Propagation

Fast community detection by propagating labels through the network.

```sql
select * from onager_cmm_label_prop((select src, dst from edges));
```

| Column  | Type   | Description     |
|---------|--------|-----------------|
| node_id | bigint | Node identifier |
| label   | bigint | Community label |

## Example: Analyze Community Sizes

```sql
with communities as (
  select * from onager_cmm_louvain((select src, dst from edges))
)
select community, count(*) as size
from communities
group by community
order by size desc;
```
