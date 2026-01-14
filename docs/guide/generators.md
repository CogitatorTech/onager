---
title: Graph Generators
description: Create synthetic graphs with known properties.
---

# Graph Generators

Graph generators create synthetic networks with specific structural properties.
They are usually useful for testing algorithms, simulating networks, or creating benchmark datasets.

!!! note "Return format"
    Generators return the generated graph in edge list format so nodes with no edges are omitted.
    Nodes will be assigned sequential IDs starting from 0 to n-1.

---

## Erdős-Rényi Random Graphs

This function creates a random graph where each possible edge exists with probability p.
This is the simplest random graph model and is useful as a null model or for testing.

Parameters:

- `n`: Number of nodes
- `p`: Edge probability (0 to 1)
- `seed`: Optional random seed for reproducibility

```sql
-- Create a random graph with 100 nodes, 10% edge probability
select src, dst
from onager_gen_erdos_renyi(100, 0.1, seed := 42);
```

| Column | Type   | Description            |
|--------|--------|------------------------|
| src    | bigint | Source node (0 to n-1) |
| dst    | bigint | Destination node       |

The expected number of edges is approximately n×(n-1)×p/2 for undirected graphs.

```sql
-- Count edges in different random graphs
select (select count(*) from onager_gen_erdos_renyi(50, 0.1, seed := 1)) as sparse,
       (select count(*) from onager_gen_erdos_renyi(50, 0.3, seed := 1)) as medium,
       (select count(*) from onager_gen_erdos_renyi(50, 0.5, seed := 1)) as dense;
```

---

## Barabási-Albert Scale-Free Graphs

This function creates a graph with a power-law degree distribution (a "scale-free" network) where new nodes
attach preferentially to existing high-degree nodes.
This models many real networks like the web, social networks, and citation networks very well.

Parameters:

- `n`: Number of nodes
- `m`: Number of edges each new node creates
- `seed`: Optional random seed

```sql
-- Create a scale-free network with 100 nodes
select src, dst
from onager_gen_barabasi_albert(100, 2, seed := 42);
```

The resulting graph will have hub nodes with many connections and many nodes with few connections:

```sql
-- Verify power-law degree distribution
with edges as (select src, dst
               from onager_gen_barabasi_albert(500, 2, seed := 42)),
     degrees as (select node_id, in_degree + out_degree as degree
                 from onager_ctr_degree((select src, dst from edges)))
select degree, count(*) as num_nodes
from degrees
group by degree
order by degree;
```

---

## Watts-Strogatz Small-World Graphs

This function creates a network with high clustering and short average path length (a "small-world" network).
The generation process starts with a ring lattice and randomly rewires edges.

Parameters:

- `n`: Number of nodes
- `k`: Number of nearest neighbors in the ring
- `beta`: Rewiring probability (0 = regular lattice, 1 = random)
- `seed`: Optional random seed

```sql
-- Create a small-world network
select src, dst
from onager_gen_watts_strogatz(100, 4, 0.3, seed := 42);
```

Compare clustering at different rewiring probabilities:

```sql
-- Low beta: high clustering, long paths
select round(avg_clustering, 3) as clustering
from onager_mtr_avg_clustering((select src, dst
                                from onager_gen_watts_strogatz(50, 4, 0.1, seed := 42)));

-- High beta: lower clustering, shorter paths
select round(avg_clustering, 3) as clustering
from onager_mtr_avg_clustering((select src, dst
                                from onager_gen_watts_strogatz(50, 4, 0.9, seed := 42)));
```

---

## Complete Example: Comparing Network Models

Generate and analyze different network types:

```sql
-- Create tables for each type
create table random_net as
select src, dst
from onager_gen_erdos_renyi(100, 0.05, seed := 42);

create table scalefree_net as
select src, dst
from onager_gen_barabasi_albert(100, 2, seed := 42);

create table smallworld_net as
select src, dst
from onager_gen_watts_strogatz(100, 4, 0.3, seed := 42);

-- Compare structural properties
select 'Random'                                                                                     as model,
       (select count(*) from random_net)                                                            as edges,
       (select round(avg_clustering, 3) from onager_mtr_avg_clustering((select * from random_net))) as clustering,
       (select diameter from onager_mtr_diameter((select * from random_net)))                       as diameter
union all
select 'Scale-free',
       (select count(*) from scalefree_net),
       (select round(avg_clustering, 3) from onager_mtr_avg_clustering((select * from scalefree_net))),
       (select diameter from onager_mtr_diameter((select * from scalefree_net)))
union all
select 'Small-world',
       (select count(*) from smallworld_net),
       (select round(avg_clustering, 3) from onager_mtr_avg_clustering((select * from smallworld_net))),
       (select diameter from onager_mtr_diameter((select * from smallworld_net)));
```
