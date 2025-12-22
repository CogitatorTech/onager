# Quick Start

Get up and running with Onager in 5 minutes.

## 1. Start DuckDB with Onager

```bash
./build/release/duckdb
```

## 2. Create Edge Data

Onager works with edge tables — any table with two columns representing source and destination nodes:

```sql
create table edges as select * from (values
  (1::bigint, 2::bigint),
  (2, 3),
  (3, 1),
  (3, 4),
  (4, 5)
) t(src, dst);
```

## 3. Run Graph Algorithms

### PageRank

```sql
select * from onager_ctr_pagerank((select src, dst from edges));
```

```
┌─────────┬────────────────────┐
│ node_id │        rank        │
│  int64  │       double       │
├─────────┼────────────────────┤
│       1 │ 0.2156862745098039 │
│       2 │ 0.1568627450980392 │
│       3 │ 0.2549019607843137 │
│       4 │ 0.2156862745098039 │
│       5 │ 0.1568627450980392 │
└─────────┴────────────────────┘
```

### Community Detection

```sql
select * from onager_cmm_louvain((select src, dst from edges));
```

### Graph Diameter

```sql
select * from onager_mtr_diameter((select src, dst from edges));
```

## 4. Generate Test Graphs

```sql
-- Random graph with 100 nodes, 10% edge probability
select * from onager_gen_erdos_renyi(100, 0.1, seed := 42) limit 5;
```

## Input Format

All analysis functions accept edge data as:

- **Column 1**: Source node (`bigint`)
- **Column 2**: Destination node (`bigint`)

Use a subquery to select the edge columns:

```sql
-- From any table with edge data
select * from onager_ctr_pagerank((
  select from_user, to_user from follows
));
```

## Next Steps

- [Centrality Algorithms](../guide/centrality.md)
- [Community Detection](../guide/community.md)
- [SQL Function Reference](../reference/sql-functions.md)
