//! Minimum Spanning Tree algorithms module.
//!
//! Prim's and Kruskal's MST algorithms.

use graphina::core::types::{Graph, NodeId};
use graphina::mst::algorithms::{kruskal_mst, prim_mst};
use ordered_float::OrderedFloat;

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Result of MST computation.
pub struct MstResult {
    pub src_nodes: Vec<i64>,
    pub dst_nodes: Vec<i64>,
    pub weights: Vec<f64>,
    pub total_weight: f64,
}

/// Compute Prim's MST.
pub fn compute_prim_mst(src: &[i64], dst: &[i64], weights: &[f64]) -> Result<MstResult> {
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

    let (mst_edges, total_weight) =
        prim_mst(&graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_src = Vec::with_capacity(mst_edges.len());
    let mut result_dst = Vec::with_capacity(mst_edges.len());
    let mut result_weights = Vec::with_capacity(mst_edges.len());

    for edge in mst_edges {
        if let (Some(&s), Some(&d)) = (reverse_map.get(&edge.u), reverse_map.get(&edge.v)) {
            result_src.push(s);
            result_dst.push(d);
            result_weights.push(edge.weight.into_inner());
        }
    }

    Ok(MstResult {
        src_nodes: result_src,
        dst_nodes: result_dst,
        weights: result_weights,
        total_weight: total_weight.into_inner(),
    })
}

/// Compute Kruskal's MST.
pub fn compute_kruskal_mst(src: &[i64], dst: &[i64], weights: &[f64]) -> Result<MstResult> {
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

    let (mst_edges, total_weight) =
        kruskal_mst(&graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_src = Vec::with_capacity(mst_edges.len());
    let mut result_dst = Vec::with_capacity(mst_edges.len());
    let mut result_weights = Vec::with_capacity(mst_edges.len());

    for edge in mst_edges {
        if let (Some(&s), Some(&d)) = (reverse_map.get(&edge.u), reverse_map.get(&edge.v)) {
            result_src.push(s);
            result_dst.push(d);
            result_weights.push(edge.weight.into_inner());
        }
    }

    Ok(MstResult {
        src_nodes: result_src,
        dst_nodes: result_dst,
        weights: result_weights,
        total_weight: total_weight.into_inner(),
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_prim_mst_simple() {
        // Triangle with one extra edge: 1-2 (1.0), 2-3 (2.0), 1-3 (4.0), 3-4 (1.5)
        let src = vec![1, 2, 1, 3];
        let dst = vec![2, 3, 3, 4];
        let weights = vec![1.0, 2.0, 4.0, 1.5];

        let result = compute_prim_mst(&src, &dst, &weights).unwrap();

        // MST should have 3 edges (4 nodes - 1)
        assert_eq!(result.src_nodes.len(), 3);
        // Total weight should be 1.0 + 2.0 + 1.5 = 4.5 (excluding 4.0 edge)
        assert!((result.total_weight - 4.5).abs() < 0.01);
    }

    #[test]
    fn test_kruskal_mst_simple() {
        let src = vec![1, 2, 1, 3];
        let dst = vec![2, 3, 3, 4];
        let weights = vec![1.0, 2.0, 4.0, 1.5];

        let result = compute_kruskal_mst(&src, &dst, &weights).unwrap();

        assert_eq!(result.src_nodes.len(), 3);
        assert!((result.total_weight - 4.5).abs() < 0.01);
    }

    #[test]
    fn test_mst_empty_graph() {
        let src: Vec<i64> = vec![];
        let dst: Vec<i64> = vec![];
        let weights: Vec<f64> = vec![];

        let result = compute_prim_mst(&src, &dst, &weights);
        assert!(result.is_err());
    }

    #[test]
    fn test_mst_mismatched_arrays() {
        let src = vec![1, 2];
        let dst = vec![2];
        let weights = vec![1.0, 2.0];

        let result = compute_prim_mst(&src, &dst, &weights);
        assert!(result.is_err());
    }
}
