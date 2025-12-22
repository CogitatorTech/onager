# Traversal & Path Finding

Algorithms for exploring graphs and finding shortest paths.

## Dijkstra's Shortest Path

Find shortest distances from a source node to all other nodes.

```sql
select * from onager_pth_dijkstra((select src, dst from edges), source := 1);
```

| Column   | Type   | Description                             |
|----------|--------|-----------------------------------------|
| node_id  | bigint | Destination node                        |
| distance | double | Distance from source (∞ if unreachable) |

## Breadth-First Search (BFS)

Explore nodes level by level from a source.

```sql
select * from onager_trv_bfs((select src, dst from edges), source := 1);
```

| Column  | Type   | Description                  |
|---------|--------|------------------------------|
| node_id | bigint | Node in traversal order      |
| order   | bigint | Discovery order (0 = source) |

## Depth-First Search (DFS)

Explore as deep as possible along each branch before backtracking.

```sql
select * from onager_trv_dfs((select src, dst from edges), source := 1);
```

## Example: Reachable Nodes

```sql
-- Find all nodes reachable from node 1
select node_id from onager_trv_bfs((select src, dst from edges), source := 1);
```
