//! Graph metrics module.
//!
//! Diameter, Radius, Average Clustering, Average Path Length, Transitivity, Triangle Count, Assortativity.

use graphina::core::types::{Graph, NodeId};
use graphina::metrics::{
    assortativity, average_clustering_coefficient, average_path_length, diameter, radius,
    transitivity,
};
use graphina::parallel::triangles_parallel;
use ordered_float::OrderedFloat;

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Compute graph diameter (longest shortest path).
pub fn compute_diameter(src: &[i64], dst: &[i64]) -> Result<i64> {
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
    Ok(diameter(&graph).map(|d| d as i64).unwrap_or(-1))
}

/// Compute graph radius.
pub fn compute_radius(src: &[i64], dst: &[i64]) -> Result<i64> {
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
    Ok(radius(&graph).map(|v| v as i64).unwrap_or(-1))
}

/// Compute average clustering coefficient.
pub fn compute_avg_clustering(src: &[i64], dst: &[i64]) -> Result<f64> {
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
    let mut graph: Graph<i64, f64> = Graph::new();
    for &node in src.iter().chain(dst.iter()) {
        if !node_set.contains_key(&node) {
            let id = graph.add_node(node);
            node_set.insert(node, id);
        }
    }
    for i in 0..src.len() {
        let src_id = node_set[&src[i]];
        let dst_id = node_set[&dst[i]];
        graph.add_edge(src_id, dst_id, 1.0);
    }
    Ok(average_clustering_coefficient(&graph))
}

/// Compute average path length.
pub fn compute_avg_path_length(src: &[i64], dst: &[i64]) -> Result<f64> {
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
    Ok(average_path_length(&graph).unwrap_or(f64::NAN))
}

/// Compute transitivity (global clustering coefficient).
pub fn compute_transitivity(src: &[i64], dst: &[i64]) -> Result<f64> {
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
    let mut graph: Graph<i64, f64> = Graph::new();
    for &node in src.iter().chain(dst.iter()) {
        if !node_set.contains_key(&node) {
            let id = graph.add_node(node);
            node_set.insert(node, id);
        }
    }
    for i in 0..src.len() {
        let src_id = node_set[&src[i]];
        let dst_id = node_set[&dst[i]];
        graph.add_edge(src_id, dst_id, 1.0);
    }
    Ok(transitivity(&graph))
}

/// Result of triangle counting.
pub struct TriangleResult {
    pub node_ids: Vec<i64>,
    pub triangle_counts: Vec<i64>,
}

/// Compute triangle count for each node.
pub fn compute_triangle_count(src: &[i64], dst: &[i64]) -> Result<TriangleResult> {
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

    let triangles = triangles_parallel(&graph);

    let mut result_nodes = Vec::with_capacity(triangles.len());
    let mut result_counts = Vec::with_capacity(triangles.len());
    for (node_id, count) in triangles {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            result_nodes.push(ext_id);
            result_counts.push(count as i64);
        }
    }
    Ok(TriangleResult {
        node_ids: result_nodes,
        triangle_counts: result_counts,
    })
}

/// Compute assortativity coefficient.
/// Measures the tendency of nodes to connect to others with similar degree.
/// Returns a value between -1 (disassortative) and 1 (assortative).
pub fn compute_assortativity(src: &[i64], dst: &[i64]) -> Result<f64> {
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
    let mut graph: Graph<i64, f64> = Graph::new();
    for &node in src.iter().chain(dst.iter()) {
        if !node_set.contains_key(&node) {
            let id = graph.add_node(node);
            node_set.insert(node, id);
        }
    }
    for i in 0..src.len() {
        let src_id = node_set[&src[i]];
        let dst_id = node_set[&dst[i]];
        graph.add_edge(src_id, dst_id, 1.0);
    }
    Ok(assortativity(&graph))
}
