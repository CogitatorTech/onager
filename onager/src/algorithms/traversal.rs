//! Graph traversal and path algorithms module.
//!
//! Dijkstra, Bellman-Ford, BFS, DFS.

use graphina::core::paths::{bellman_ford, dijkstra, floyd_warshall};
use graphina::core::types::{Graph, NodeId};
use graphina::traversal::algorithms::{bfs, dfs};
use ordered_float::OrderedFloat;

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Result of Dijkstra shortest path computation.
pub struct DijkstraResult {
    pub node_ids: Vec<i64>,
    pub distances: Vec<f64>,
}

/// Compute shortest distances from a source node.
pub fn compute_dijkstra(src: &[i64], dst: &[i64], source_node: i64) -> Result<DijkstraResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }

    let mut node_set: HashMap<i64, NodeId> = HashMap::new();
    let mut graph: Graph<i64, OrderedFloat<f64>> = Graph::new();
    for &node in src.iter().chain(dst.iter()) {
        if !node_set.contains_key(&node) {
            let id = graph.add_node(node);
            node_set.insert(node, id);
        }
    }
    for i in 0..src.len() {
        let src_id = node_set[&src[i]];
        let dst_id = node_set[&dst[i]];
        graph.add_edge(src_id, dst_id, OrderedFloat(1.0));
    }

    let source_id = node_set.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;
    let distances =
        dijkstra(&graph, *source_id).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_dist = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        let dist = distances.get(int_id).and_then(|d| *d);
        result_dist.push(dist.map(|d| d.into_inner()).unwrap_or(f64::INFINITY));
    }
    Ok(DijkstraResult {
        node_ids: result_nodes,
        distances: result_dist,
    })
}

/// Result of BFS traversal.
pub struct BfsResult {
    pub node_ids: Vec<i64>,
    pub order: Vec<i64>,
}

/// Compute BFS traversal from a source node.
pub fn compute_bfs(src: &[i64], dst: &[i64], source_node: i64) -> Result<BfsResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }

    let mut node_set: HashMap<i64, NodeId> = HashMap::new();
    let mut reverse_map: HashMap<NodeId, i64> = HashMap::new();
    let mut graph: Graph<i64, f64> = Graph::new();
    for &node in src.iter().chain(dst.iter()) {
        if !node_set.contains_key(&node) {
            let id = graph.add_node(node);
            node_set.insert(node, id);
            reverse_map.insert(id, node);
        }
    }
    for i in 0..src.len() {
        let src_id = node_set[&src[i]];
        let dst_id = node_set[&dst[i]];
        graph.add_edge(src_id, dst_id, 1.0);
    }

    let source_id = node_set.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;
    let traversal = bfs(&graph, *source_id);

    let mut order = Vec::new();
    for internal_id in &traversal {
        if let Some(&ext_id) = reverse_map.get(internal_id) {
            order.push(ext_id);
        }
    }
    Ok(BfsResult {
        node_ids: order.clone(),
        order,
    })
}

/// Result of DFS traversal.
pub struct DfsResult {
    pub node_ids: Vec<i64>,
    pub order: Vec<i64>,
}

/// Compute DFS traversal from a source node.
pub fn compute_dfs(src: &[i64], dst: &[i64], source_node: i64) -> Result<DfsResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }

    let mut node_set: HashMap<i64, NodeId> = HashMap::new();
    let mut reverse_map: HashMap<NodeId, i64> = HashMap::new();
    let mut graph: Graph<i64, f64> = Graph::new();
    for &node in src.iter().chain(dst.iter()) {
        if !node_set.contains_key(&node) {
            let id = graph.add_node(node);
            node_set.insert(node, id);
            reverse_map.insert(id, node);
        }
    }
    for i in 0..src.len() {
        let src_id = node_set[&src[i]];
        let dst_id = node_set[&dst[i]];
        graph.add_edge(src_id, dst_id, 1.0);
    }

    let source_id = node_set.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;
    let traversal = dfs(&graph, *source_id);

    let mut order = Vec::new();
    for internal_id in &traversal {
        if let Some(&ext_id) = reverse_map.get(internal_id) {
            order.push(ext_id);
        }
    }
    Ok(DfsResult {
        node_ids: order.clone(),
        order,
    })
}

/// Compute shortest distance between two specific nodes.
/// Returns f64::INFINITY if unreachable, or the distance if reachable.
pub fn compute_shortest_distance(
    src: &[i64],
    dst: &[i64],
    source_node: i64,
    target_node: i64,
) -> Result<f64> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }

    let mut node_set: HashMap<i64, NodeId> = HashMap::new();
    let mut graph: Graph<i64, OrderedFloat<f64>> = Graph::new();
    for &node in src.iter().chain(dst.iter()) {
        if !node_set.contains_key(&node) {
            let id = graph.add_node(node);
            node_set.insert(node, id);
        }
    }
    for i in 0..src.len() {
        let src_id = node_set[&src[i]];
        let dst_id = node_set[&dst[i]];
        graph.add_edge(src_id, dst_id, OrderedFloat(1.0));
    }

    let source_id = node_set.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;
    let target_id = node_set.get(&target_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Target node {} not found", target_node))
    })?;

    let distances =
        dijkstra(&graph, *source_id).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let dist = distances.get(target_id).and_then(|d| *d);
    Ok(dist.map(|d| d.into_inner()).unwrap_or(f64::INFINITY))
}

/// Result of Bellman-Ford shortest path computation.
pub struct BellmanFordResult {
    pub node_ids: Vec<i64>,
    pub distances: Vec<f64>,
}

/// Compute shortest distances from a source node using Bellman-Ford.
/// Works with negative weights but detects negative cycles.
pub fn compute_bellman_ford(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    source_node: i64,
) -> Result<BellmanFordResult> {
    if src.len() != dst.len() || src.len() != weights.len() {
        return Err(OnagerError::InvalidArgument(
            "src, dst, and weights arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }

    let mut node_set: HashMap<i64, NodeId> = HashMap::new();
    let mut graph: Graph<i64, OrderedFloat<f64>> = Graph::new();
    for &node in src.iter().chain(dst.iter()) {
        if !node_set.contains_key(&node) {
            let id = graph.add_node(node);
            node_set.insert(node, id);
        }
    }
    for i in 0..src.len() {
        let src_id = node_set[&src[i]];
        let dst_id = node_set[&dst[i]];
        graph.add_edge(src_id, dst_id, OrderedFloat(weights[i]));
    }

    let source_id = node_set.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;

    let distances = bellman_ford(&graph, *source_id)
        .ok_or_else(|| OnagerError::GraphError("Negative cycle detected".to_string()))?;

    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_dist = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        let dist = distances.get(int_id).and_then(|d| *d);
        result_dist.push(dist.map(|d| d.into_inner()).unwrap_or(f64::INFINITY));
    }
    Ok(BellmanFordResult {
        node_ids: result_nodes,
        distances: result_dist,
    })
}

/// Result of Floyd-Warshall all-pairs shortest paths.
pub struct FloydWarshallResult {
    pub src_nodes: Vec<i64>,
    pub dst_nodes: Vec<i64>,
    pub distances: Vec<f64>,
}

/// Compute all-pairs shortest distances using Floyd-Warshall.
pub fn compute_floyd_warshall(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
) -> Result<FloydWarshallResult> {
    if src.len() != dst.len() || src.len() != weights.len() {
        return Err(OnagerError::InvalidArgument(
            "src, dst, and weights arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }

    let mut node_set: HashMap<i64, NodeId> = HashMap::new();
    let mut reverse_map: HashMap<NodeId, i64> = HashMap::new();
    let mut graph: Graph<i64, OrderedFloat<f64>> = Graph::new();

    for &node in src.iter().chain(dst.iter()) {
        if !node_set.contains_key(&node) {
            let id = graph.add_node(node);
            node_set.insert(node, id);
            reverse_map.insert(id, node);
        }
    }
    for i in 0..src.len() {
        let src_id = node_set[&src[i]];
        let dst_id = node_set[&dst[i]];
        graph.add_edge(src_id, dst_id, OrderedFloat(weights[i]));
    }

    let distances = floyd_warshall(&graph)
        .ok_or_else(|| OnagerError::GraphError("Negative cycle detected".to_string()))?;

    let mut result_src = Vec::new();
    let mut result_dst = Vec::new();
    let mut result_dist = Vec::new();

    for (&from_id, inner) in &distances {
        if let Some(&from_ext) = reverse_map.get(&from_id) {
            for (&to_id, &dist_opt) in inner {
                if let Some(&to_ext) = reverse_map.get(&to_id) {
                    if from_ext != to_ext {
                        result_src.push(from_ext);
                        result_dst.push(to_ext);
                        result_dist.push(dist_opt.map(|d| d.into_inner()).unwrap_or(f64::INFINITY));
                    }
                }
            }
        }
    }

    Ok(FloydWarshallResult {
        src_nodes: result_src,
        dst_nodes: result_dst,
        distances: result_dist,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_bellman_ford_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];
        let weights = vec![1.0, 2.0, 3.0];

        let result = compute_bellman_ford(&src, &dst, &weights, 1).unwrap();

        assert_eq!(result.node_ids.len(), 4);
    }

    #[test]
    fn test_bellman_ford_source_not_found() {
        let src = vec![1, 2];
        let dst = vec![2, 3];
        let weights = vec![1.0, 2.0];

        let result = compute_bellman_ford(&src, &dst, &weights, 99);
        assert!(result.is_err());
    }

    #[test]
    fn test_dijkstra_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];

        let result = compute_dijkstra(&src, &dst, 1).unwrap();

        assert_eq!(result.node_ids.len(), 4);
    }

    #[test]
    fn test_bfs_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];

        let result = compute_bfs(&src, &dst, 1).unwrap();

        assert!(!result.node_ids.is_empty());
    }

    #[test]
    fn test_dfs_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];

        let result = compute_dfs(&src, &dst, 1).unwrap();

        assert!(!result.node_ids.is_empty());
    }
}
