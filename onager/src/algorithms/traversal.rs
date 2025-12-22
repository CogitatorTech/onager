//! Graph traversal and path algorithms module.
//!
//! Dijkstra, BFS, DFS.

use graphina::core::paths::dijkstra;
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
        return Err(OnagerError::InvalidArgument("src and dst arrays must have same length".to_string()));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument("Cannot compute on empty graph".to_string()));
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
    let distances = dijkstra(&graph, *source_id).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_dist = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        let dist = distances.get(int_id).and_then(|d| *d);
        result_dist.push(dist.map(|d| d.into_inner()).unwrap_or(f64::INFINITY));
    }
    Ok(DijkstraResult { node_ids: result_nodes, distances: result_dist })
}

/// Result of BFS traversal.
pub struct BfsResult {
    pub node_ids: Vec<i64>,
    pub order: Vec<i64>,
}

/// Compute BFS traversal from a source node.
pub fn compute_bfs(src: &[i64], dst: &[i64], source_node: i64) -> Result<BfsResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument("src and dst arrays must have same length".to_string()));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument("Cannot compute on empty graph".to_string()));
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
    Ok(BfsResult { node_ids: order.clone(), order })
}

/// Result of DFS traversal.
pub struct DfsResult {
    pub node_ids: Vec<i64>,
    pub order: Vec<i64>,
}

/// Compute DFS traversal from a source node.
pub fn compute_dfs(src: &[i64], dst: &[i64], source_node: i64) -> Result<DfsResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument("src and dst arrays must have same length".to_string()));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument("Cannot compute on empty graph".to_string()));
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
    Ok(DfsResult { node_ids: order.clone(), order })
}
