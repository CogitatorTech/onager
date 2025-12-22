# Traversal & Path Finding

Algorithms for exploring graphs and finding shortest paths.

## Dijkstra's Shortest Path

Find shortest distances from a source node to all other nodes.

```sql
SELECT * FROM onager_pth_dijkstra((SELECT src, dst FROM edges), source := 1);
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | BIGINT | Destination node |
| distance | DOUBLE | Distance from source (∞ if unreachable) |

## Breadth-First Search (BFS)

Explore nodes level by level from a source.

```sql
SELECT * FROM onager_trv_bfs((SELECT src, dst FROM edges), source := 1);
```

| Column | Type | Description |
|--------|------|-------------|
| node_id | BIGINT | Node in traversal order |
| order | BIGINT | Discovery order (0 = source) |

## Depth-First Search (DFS)

Explore as deep as possible along each branch before backtracking.

```sql
SELECT * FROM onager_trv_dfs((SELECT src, dst FROM edges), source := 1);
```

## Example: Reachable Nodes

```sql
-- Find all nodes reachable from node 1
SELECT node_id FROM onager_trv_bfs((SELECT src, dst FROM edges), source := 1);
```
