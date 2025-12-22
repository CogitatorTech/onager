# Graph Generators

Generate synthetic graphs for testing and benchmarking.

## Erdős-Rényi Random Graph

Generate a random graph where each edge exists with probability `p`.

```sql
SELECT * FROM onager_gen_erdos_renyi(n, p, seed := 42);
```

| Parameter | Type | Description |
|-----------|------|-------------|
| n | BIGINT | Number of nodes |
| p | DOUBLE | Edge probability (0.0 to 1.0) |
| seed | BIGINT | Random seed (optional) |

### Example

```sql
-- 100 nodes, 10% edge probability
SELECT * FROM onager_gen_erdos_renyi(100, 0.1, seed := 42) LIMIT 5;
```

## Barabási-Albert (Scale-Free)

Generate a scale-free graph using preferential attachment.

```sql
SELECT * FROM onager_gen_barabasi_albert(n, m, seed := 42);
```

| Parameter | Type | Description |
|-----------|------|-------------|
| n | BIGINT | Number of nodes |
| m | BIGINT | Edges per new node |
| seed | BIGINT | Random seed (optional) |

### Example

```sql
-- 100 nodes, 3 edges per new node
SELECT * FROM onager_gen_barabasi_albert(100, 3, seed := 42);
```

## Watts-Strogatz (Small-World)

Generate a small-world graph with high clustering and short paths.

```sql
SELECT * FROM onager_gen_watts_strogatz(n, k, beta, seed := 42);
```

| Parameter | Type | Description |
|-----------|------|-------------|
| n | BIGINT | Number of nodes |
| k | BIGINT | Each node connected to k nearest neighbors (must be even) |
| beta | DOUBLE | Rewiring probability (0.0 to 1.0) |
| seed | BIGINT | Random seed (optional) |

### Example

```sql
-- 100 nodes, 4 neighbors, 30% rewiring
SELECT * FROM onager_gen_watts_strogatz(100, 4, 0.3, seed := 42);
```

## Saving Generated Graphs

```sql
CREATE TABLE my_graph AS 
SELECT * FROM onager_gen_erdos_renyi(1000, 0.05, seed := 123);
```
