# Centrality Analysis Example

## Finding Influential Nodes in a Network

```sql
-- Create a citation network
CREATE TABLE citations AS SELECT * FROM (VALUES 
  (1::BIGINT, 2::BIGINT), (1, 3), (2, 4), (2, 5),
  (3, 4), (3, 6), (4, 7), (5, 7),
  (6, 7), (7, 8), (7, 9), (8, 10), (9, 10)
) t(paper_from, paper_to);

-- PageRank: overall importance
SELECT * FROM onager_ctr_pagerank((SELECT paper_from, paper_to FROM citations))
ORDER BY rank DESC LIMIT 5;

-- Betweenness: information brokers
SELECT * FROM onager_ctr_betweenness((SELECT paper_from, paper_to FROM citations))
ORDER BY betweenness DESC LIMIT 5;

-- Degree: most connected
SELECT * FROM onager_ctr_degree((SELECT paper_from, paper_to FROM citations))
ORDER BY in_degree + out_degree DESC LIMIT 5;
```

## Comparing Centrality Measures

```sql
WITH 
  pr AS (SELECT node_id, rank FROM onager_ctr_pagerank((SELECT paper_from, paper_to FROM citations))),
  bt AS (SELECT node_id, betweenness FROM onager_ctr_betweenness((SELECT paper_from, paper_to FROM citations))),
  dg AS (SELECT node_id, in_degree + out_degree as total_degree FROM onager_ctr_degree((SELECT paper_from, paper_to FROM citations)))
SELECT pr.node_id, pr.rank, bt.betweenness, dg.total_degree
FROM pr 
JOIN bt ON pr.node_id = bt.node_id
JOIN dg ON pr.node_id = dg.node_id
ORDER BY pr.rank DESC;
```
