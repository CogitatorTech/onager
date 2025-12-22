# Graph Registry

Onager provides an in-memory graph registry that stores named graphs for efficient repeated queries.

## When to Use the Registry

- **Multiple queries on same data**: Store once, query many times
- **Scalar node queries**: Get degree, check existence
- **Session-persistent graphs**: Graphs persist for database session lifetime

## Creating Graphs

```sql
-- Create a directed graph
select onager_create_graph('social', true);

-- Create an undirected graph
select onager_create_graph('network', false);
```

## Adding Data

```sql
-- Add nodes
select onager_add_node('social', 1);
select onager_add_node('social', 2);
select onager_add_node('social', 3);

-- Add edges (with weights)
select onager_add_edge('social', 1, 2, 1.0);
select onager_add_edge('social', 1, 3, 0.5);
select onager_add_edge('social', 2, 3, 1.0);
```

## Querying Graphs

```sql
-- Get graph statistics
select onager_node_count('social');  -- 3
select onager_edge_count('social');  -- 3

-- Query node degrees
select onager_node_in_degree('social', 3);   -- 2 (from nodes 1 and 2)
select onager_node_out_degree('social', 1);  -- 2 (to nodes 2 and 3)
```

## Managing Graphs

```sql
-- List all graphs
select onager_list_graphs();  -- '["social", "network"]'

-- Delete a graph
select onager_drop_graph('social');
```

## Graph Registry vs Table Functions

| Feature      | Graph Registry                  | Table Functions              |
|--------------|---------------------------------|------------------------------|
| Data storage | In-memory                       | Per-query                    |
| Query type   | Scalar (single values)          | Table (result sets)          |
| Best for     | Point queries, repeated access  | One-time analysis            |
| Example      | `onager_node_in_degree('g', 1)` | `onager_ctr_pagerank(edges)` |

## Example: Social Network Analysis

```sql
-- Setup
select onager_create_graph('friends', true);

-- Bulk load from table
select onager_add_node('friends', user_id) from users;
select onager_add_edge('friends', follower_id, followed_id, 1.0) from follows;

-- Query influential users (high out-degree)
select user_id, onager_node_out_degree('friends', user_id) as followers
from users
order by followers desc
limit 10;

-- Cleanup
select onager_drop_graph('friends');
```
