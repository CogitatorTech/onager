---
title: Centrality Algorithms
description: Identify the most important nodes in your graph.
---

# Centrality Algorithms

Centrality measures identify the most important nodes (or vertices) in a graph.
The importance of a node depends on the context and measured differently by each algorithm.

## Setup

All examples use this sample social network:

```sql
create table edges as select * from (values
  (1::bigint, 2::bigint), (1, 3), (2, 3), (2, 4),
  (3, 4), (3, 5), (4, 5), (5, 6), (6, 7), (7, 8)
) t(src, dst);
```

---

## PageRank

PageRank measures importance based on the link structure.
A node is important if many important nodes point to it.
It was originally developed by Google for ranking web pages.

\[
PR(u) = \frac{1-d}{N} + d \sum_{v \in B_u} \frac{PR(v)}{L(v)}
\]

where \(d\) is the damping factor, \(N\) is the number of nodes, \(B_u\) is the set of nodes linking to \(u\), and \(L(v)\) is the out-degree of \(v\).

```sql
select node_id, round(rank, 4) as rank
from onager_ctr_pagerank((select src, dst from edges))
order by rank desc;
```

| Column  | Type   | Description                              |
|---------|--------|------------------------------------------|
| node_id | bigint | Node identifier                          |
| rank    | double | PageRank score (higher = more important) |

Optional parameters:

- `damping` (default 0.85): Probability of following a link vs jumping randomly
- `iterations` (default 100): Maximum iterations
- `directed` (default false): Treat graph as directed

```sql
-- Custom damping factor
select * from onager_ctr_pagerank(
  (select src, dst from edges),
  damping := 0.9,
  iterations := 50
);
```

---

## Degree Centrality

The simplest centrality measure — counts the number of connections per node.
High degree nodes are "hubs" with many direct connections.

\[
C_D(v) = \frac{\deg(v)}{N-1}
\]

```sql
select node_id, in_degree, out_degree, (in_degree + out_degree) as total
from onager_ctr_degree((select src, dst from edges))
order by total desc;
```

| Column     | Type   | Description              |
|------------|--------|--------------------------|
| node_id    | bigint | Node identifier          |
| in_degree  | double | Number of incoming edges |
| out_degree | double | Number of outgoing edges |

---

## Betweenness Centrality

Measures how often a node appears on shortest paths between other nodes.
High betweenness nodes act as bridges or bottlenecks controlling information flow.

\[
C_B(v) = \sum_{s \neq v \neq t} \frac{\sigma_{st}(v)}{\sigma_{st}}
\]

where \(\sigma_{st}\) is the number of shortest paths from \(s\) to \(t\), and \(\sigma_{st}(v)\) is the number passing through \(v\).

!!! warning "Performance"
    This algorithm has O(n·m) complexity. So, it may be slow on dense graphs with more than 10,000 nodes.

```sql
select node_id, round(betweenness, 4) as betweenness
from onager_ctr_betweenness((select src, dst from edges))
where betweenness > 0
order by betweenness desc;
```

| Column      | Type   | Description                  |
|-------------|--------|------------------------------|
| node_id     | bigint | Node identifier              |
| betweenness | double | Betweenness centrality score |

---

## Closeness Centrality

Measures how close a node is to all other nodes (average shortest path distance).
High closeness means information can spread quickly from this node.

\[
C_C(v) = \frac{N-1}{\sum_{u \neq v} d(u, v)}
\]

where \(d(u, v)\) is the shortest path distance between \(u\) and \(v\).

```sql
select node_id, round(closeness, 4) as closeness
from onager_ctr_closeness((select src, dst from edges))
order by closeness desc;
```

| Column    | Type   | Description                           |
|-----------|--------|---------------------------------------|
| node_id   | bigint | Node identifier                       |
| closeness | double | Inverse of average distance to others |

---

## Harmonic Centrality

A variant of closeness that handles disconnected graphs.
Uses the sum of inverse distances instead of inverse of sum.

\[
C_H(v) = \sum_{u \neq v} \frac{1}{d(u, v)}
\]

```sql
select node_id, round(harmonic, 4) as harmonic
from onager_ctr_harmonic((select src, dst from edges))
order by harmonic desc;
```

---

## Eigenvector Centrality

A node is important if connected to other important nodes.
This creates a recursive definition where connections to high-scoring nodes contribute more.

!!! warning "Performance"
    This algorithm uses power iteration on the adjacency matrix. May be slow on graphs with more than 10,000 nodes.

```sql
select node_id, round(eigenvector, 4) as eigenvector
from onager_ctr_eigenvector((select src, dst from edges))
order by eigenvector desc;
```

Optional parameters:

- `max_iter` (default 100): Maximum iterations
- `tolerance` (default 1e-6): Convergence threshold

---

## Katz Centrality

Similar to eigenvector centrality but counts all paths, not just direct connections.
Longer paths contribute less based on an attenuation factor.

```sql
select node_id, round(katz, 4) as katz
from onager_ctr_katz((select src, dst from edges), alpha := 0.1)
order by katz desc;
```

Optional parameters:

- `alpha` (default 0.1): Attenuation factor for longer paths
- `beta` (default 1.0): Weight for initial centrality

---

## VoteRank

VoteRank identifies influential spreaders in a network.
It iteratively selects nodes that can best spread information, penalizing neighbors of already-selected nodes to ensure diversity.

```sql
select node_id
from onager_ctr_voterank((select src, dst from edges), num_seeds := 5)
order by node_id;
```

| Column  | Type   | Description                  |
|---------|--------|------------------------------|
| node_id | bigint | Influential spreader node ID |

Optional parameters:

- `num_seeds` (default 10): Number of influential nodes to return

---

## Local Reaching Centrality

Measures how many nodes can be reached within a specified distance.
Higher values indicate nodes that can quickly spread information.

```sql
select node_id, round(centrality, 2) as reach
from onager_ctr_local_reaching((select src, dst from edges), distance := 2)
order by centrality desc;
```

| Column     | Type   | Description                         |
|------------|--------|-------------------------------------|
| node_id    | bigint | Node identifier                     |
| centrality | double | Number of reachable nodes           |

Optional parameters:

- `distance` (default 2): Maximum distance to consider

---

## Laplacian Centrality

Based on the Laplacian matrix of the graph.
Captures both node degree and neighbor connectivity.

```sql
select node_id, round(centrality, 2) as laplacian
from onager_ctr_laplacian((select src, dst from edges))
order by centrality desc;
```

| Column     | Type   | Description             |
|------------|--------|-------------------------|
| node_id    | bigint | Node identifier         |
| centrality | double | Laplacian centrality    |

---

## Complete Example: Influencer Analysis

Find the top influencers using multiple centrality metrics:

```sql
-- Create a social network
create table follows as select * from (values
  (1, 2), (1, 3), (2, 3), (2, 4), (3, 4), (3, 5),
  (4, 5), (4, 6), (5, 6), (5, 7), (6, 7), (6, 8),
  (7, 8), (7, 9), (8, 9), (8, 10), (9, 10)
) t(follower, followed);

-- Combine multiple centrality measures
with pr as (
  select node_id, rank from onager_ctr_pagerank(
    (select follower as src, followed as dst from follows)
  )
),
deg as (
  select node_id, in_degree from onager_ctr_degree(
    (select follower as src, followed as dst from follows)
  )
),
bet as (
  select node_id, betweenness from onager_ctr_betweenness(
    (select follower as src, followed as dst from follows)
  )
)
select
  pr.node_id as user_id,
  round(pr.rank, 4) as pagerank,
  deg.in_degree as followers,
  round(bet.betweenness, 2) as bridge_score
from pr
join deg on pr.node_id = deg.node_id
join bet on pr.node_id = bet.node_id
order by pr.rank desc
limit 5;
```
