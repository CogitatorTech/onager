//! Personalized PageRank module.
//!
//! Personalized PageRank for node-specific influence computation and recommendations.

use graphina::community::personalized_pagerank::personalized_page_rank;
use graphina::core::types::{Graph, NodeId};

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Result of personalized PageRank computation.
pub struct PersonalizedPageRankResult {
    pub node_ids: Vec<i64>,
    pub scores: Vec<f64>,
}

/// Compute Personalized PageRank.
///
/// # Arguments
/// * `src` - Source node IDs for edges
/// * `dst` - Destination node IDs for edges
/// * `personalization` - Map of node_id -> weight for personalization (biases random jumps)
/// * `damping` - Damping factor (typically 0.85)
/// * `max_iter` - Maximum iterations
/// * `tolerance` - Convergence tolerance
pub fn compute_personalized_pagerank(
    src: &[i64],
    dst: &[i64],
    personalization: &[(i64, f64)],
    damping: f64,
    max_iter: usize,
    tolerance: f64,
) -> Result<PersonalizedPageRankResult> {
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
    if !(0.0..1.0).contains(&damping) {
        return Err(OnagerError::InvalidArgument(
            "damping must be in (0, 1)".to_string(),
        ));
    }
    if max_iter == 0 {
        return Err(OnagerError::InvalidArgument(
            "max_iter must be positive".to_string(),
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

    // Build personalization vector aligned with node indices
    let n = graph.node_count();
    let node_list: Vec<NodeId> = graph.nodes().map(|(id, _)| id).collect();

    let personalization_vec = if personalization.is_empty() {
        None
    } else {
        // Create a personalization vector aligned with node order
        let mut p_vec = vec![0.0; n];
        for &(ext_id, weight) in personalization {
            if let Some(&node_idx) = node_set.get(&ext_id) {
                // Find position of node_idx in node_list
                if let Some(pos) = node_list.iter().position(|&id| id == node_idx) {
                    p_vec[pos] = weight;
                }
            }
        }
        Some(p_vec)
    };

    let ranks = personalized_page_rank(&graph, personalization_vec, damping, tolerance, max_iter)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();

    let mut node_ids = Vec::with_capacity(ranks.len());
    let mut scores = Vec::with_capacity(ranks.len());
    for (i, &rank) in ranks.iter().enumerate() {
        if let Some(&ext_id) = reverse_map.get(&node_list[i]) {
            node_ids.push(ext_id);
            scores.push(rank);
        }
    }
    Ok(PersonalizedPageRankResult { node_ids, scores })
}
