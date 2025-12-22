//! Link prediction algorithms module.
//!
//! Jaccard, Adamic-Adar, Preferential Attachment, Resource Allocation.

use graphina::core::types::{Graph, NodeId};
use graphina::links::allocation::resource_allocation_index;
use graphina::links::attachment::preferential_attachment;
use graphina::links::similarity::{adamic_adar_index, jaccard_coefficient};

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Result of link prediction computation.
pub struct LinkPredictionResult {
    pub node1: Vec<i64>,
    pub node2: Vec<i64>,
    pub scores: Vec<f64>,
}

// Re-export as JaccardResult for backward compatibility
pub type JaccardResult = LinkPredictionResult;

/// Compute Jaccard coefficient for all node pairs.
pub fn compute_jaccard(src: &[i64], dst: &[i64]) -> Result<LinkPredictionResult> {
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

    let results = jaccard_coefficient(&graph, None);
    let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();

    let mut node1 = Vec::with_capacity(results.len());
    let mut node2 = Vec::with_capacity(results.len());
    let mut scores = Vec::with_capacity(results.len());
    for ((u, v), coef) in results {
        if let (Some(&ext_u), Some(&ext_v)) = (reverse_map.get(&u), reverse_map.get(&v)) {
            node1.push(ext_u);
            node2.push(ext_v);
            scores.push(coef);
        }
    }
    Ok(LinkPredictionResult {
        node1,
        node2,
        scores,
    })
}

/// Compute Adamic-Adar index for all node pairs.
pub fn compute_adamic_adar(src: &[i64], dst: &[i64]) -> Result<LinkPredictionResult> {
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

    let results = adamic_adar_index(&graph, None);
    let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();

    let mut node1 = Vec::with_capacity(results.len());
    let mut node2 = Vec::with_capacity(results.len());
    let mut scores = Vec::with_capacity(results.len());
    for ((u, v), coef) in results {
        if let (Some(&ext_u), Some(&ext_v)) = (reverse_map.get(&u), reverse_map.get(&v)) {
            node1.push(ext_u);
            node2.push(ext_v);
            scores.push(coef);
        }
    }
    Ok(LinkPredictionResult {
        node1,
        node2,
        scores,
    })
}

/// Compute preferential attachment for all node pairs.
pub fn compute_preferential_attachment(src: &[i64], dst: &[i64]) -> Result<LinkPredictionResult> {
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

    let results = preferential_attachment(&graph, None);
    let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();

    let mut node1 = Vec::with_capacity(results.len());
    let mut node2 = Vec::with_capacity(results.len());
    let mut scores = Vec::with_capacity(results.len());
    for ((u, v), coef) in results {
        if let (Some(&ext_u), Some(&ext_v)) = (reverse_map.get(&u), reverse_map.get(&v)) {
            node1.push(ext_u);
            node2.push(ext_v);
            scores.push(coef);
        }
    }
    Ok(LinkPredictionResult {
        node1,
        node2,
        scores,
    })
}

/// Compute resource allocation index for all node pairs.
pub fn compute_resource_allocation(src: &[i64], dst: &[i64]) -> Result<LinkPredictionResult> {
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

    let results = resource_allocation_index(&graph, None);
    let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();

    let mut node1 = Vec::with_capacity(results.len());
    let mut node2 = Vec::with_capacity(results.len());
    let mut scores = Vec::with_capacity(results.len());
    for ((u, v), coef) in results {
        if let (Some(&ext_u), Some(&ext_v)) = (reverse_map.get(&u), reverse_map.get(&v)) {
            node1.push(ext_u);
            node2.push(ext_v);
            scores.push(coef);
        }
    }
    Ok(LinkPredictionResult {
        node1,
        node2,
        scores,
    })
}
