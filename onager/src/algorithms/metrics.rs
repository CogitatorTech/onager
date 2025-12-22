//! Graph metrics module.
//!
//! Diameter, Radius, Average Clustering, Average Path Length, Transitivity.

use graphina::core::types::{Graph, NodeId};
use graphina::metrics::{average_clustering_coefficient, average_path_length, diameter, radius, transitivity};
use ordered_float::OrderedFloat;

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Compute graph diameter (longest shortest path).
pub fn compute_diameter(src: &[i64], dst: &[i64]) -> Result<i64> {
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
    Ok(diameter(&graph).map(|d| d as i64).unwrap_or(-1))
}

/// Compute graph radius.
pub fn compute_radius(src: &[i64], dst: &[i64]) -> Result<i64> {
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
    Ok(radius(&graph).map(|v| v as i64).unwrap_or(-1))
}

/// Compute average clustering coefficient.
pub fn compute_avg_clustering(src: &[i64], dst: &[i64]) -> Result<f64> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument("src and dst arrays must have same length".to_string()));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument("Cannot compute on empty graph".to_string()));
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
    Ok(average_path_length(&graph).unwrap_or(f64::NAN))
}

/// Compute transitivity (global clustering coefficient).
pub fn compute_transitivity(src: &[i64], dst: &[i64]) -> Result<f64> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument("src and dst arrays must have same length".to_string()));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument("Cannot compute on empty graph".to_string()));
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
