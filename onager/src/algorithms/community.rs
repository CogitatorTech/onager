//! Community detection algorithms module.
//!
//! Louvain, Connected Components, Label Propagation.

use graphina::community::connected_components::connected_components;
use graphina::community::label_propagation::label_propagation;
use graphina::community::louvain::louvain;
use graphina::core::types::{Graph, NodeId};

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Result of Louvain community detection.
pub struct LouvainResult {
    pub node_ids: Vec<i64>,
    pub community_ids: Vec<i64>,
}

/// Compute Louvain community detection.
pub fn compute_louvain(src: &[i64], dst: &[i64], seed: Option<u64>) -> Result<LouvainResult> {
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

    let communities = louvain(&graph, seed).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let mut result_nodes = Vec::new();
    let mut result_comms = Vec::new();
    for (comm_id, community) in communities.iter().enumerate() {
        for &internal_id in community {
            if let Some(&ext_id) = reverse_map.get(&internal_id) {
                result_nodes.push(ext_id);
                result_comms.push(comm_id as i64);
            }
        }
    }
    Ok(LouvainResult { node_ids: result_nodes, community_ids: result_comms })
}

/// Result of connected components computation.
pub struct ConnectedComponentsResult {
    pub node_ids: Vec<i64>,
    pub component_ids: Vec<i64>,
}

/// Compute connected components.
pub fn compute_connected_components(src: &[i64], dst: &[i64]) -> Result<ConnectedComponentsResult> {
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

    let components = connected_components(&graph);
    let mut result_nodes = Vec::new();
    let mut result_comps = Vec::new();
    for (comp_id, component) in components.iter().enumerate() {
        for &internal_id in component {
            if let Some(&ext_id) = reverse_map.get(&internal_id) {
                result_nodes.push(ext_id);
                result_comps.push(comp_id as i64);
            }
        }
    }
    Ok(ConnectedComponentsResult { node_ids: result_nodes, component_ids: result_comps })
}

/// Result of label propagation.
pub struct LabelPropagationResult {
    pub node_ids: Vec<i64>,
    pub labels: Vec<i64>,
}

/// Compute label propagation community detection.
pub fn compute_label_propagation(src: &[i64], dst: &[i64]) -> Result<LabelPropagationResult> {
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

    let labels_vec = label_propagation(&graph, 100, None).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();
    let node_list: Vec<NodeId> = graph.nodes().map(|(id, _)| id).collect();

    let mut node_ids = Vec::with_capacity(labels_vec.len());
    let mut labels = Vec::with_capacity(labels_vec.len());
    for (i, label) in labels_vec.into_iter().enumerate() {
        if let Some(&ext_id) = reverse_map.get(&node_list[i]) {
            node_ids.push(ext_id);
            labels.push(label as i64);
        }
    }
    Ok(LabelPropagationResult { node_ids, labels })
}
