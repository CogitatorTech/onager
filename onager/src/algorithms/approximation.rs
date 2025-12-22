//! Approximation algorithms module.
//!
//! Maximum Clique, Independent Set, Vertex Cover.

use graphina::approximation::clique::max_clique;
use graphina::approximation::independent_set::maximum_independent_set;
use graphina::approximation::vertex_cover::min_weighted_vertex_cover;
use graphina::core::types::{Graph, NodeId};

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Result of maximum clique computation.
pub struct CliqueResult {
    pub node_ids: Vec<i64>,
}

/// Compute maximum clique approximation.
pub fn compute_max_clique(src: &[i64], dst: &[i64]) -> Result<CliqueResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    // Handle empty graph gracefully - return empty clique
    if src.is_empty() {
        return Ok(CliqueResult {
            node_ids: Vec::new(),
        });
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

    // Graphina's max_clique returns HashSet<NodeId>
    let clique_nodes = max_clique(&graph);

    let mut result_nodes = Vec::with_capacity(clique_nodes.len());
    for node_id in clique_nodes {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            result_nodes.push(ext_id);
        }
    }

    // Sort for deterministic output
    result_nodes.sort();

    Ok(CliqueResult {
        node_ids: result_nodes,
    })
}

/// Result of independent set computation.
pub struct IndependentSetResult {
    pub node_ids: Vec<i64>,
}

/// Compute maximum independent set approximation.
pub fn compute_independent_set(src: &[i64], dst: &[i64]) -> Result<IndependentSetResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Ok(IndependentSetResult {
            node_ids: Vec::new(),
        });
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

    let indep_set = maximum_independent_set(&graph);

    let mut result_nodes = Vec::with_capacity(indep_set.len());
    for node_id in indep_set {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            result_nodes.push(ext_id);
        }
    }
    result_nodes.sort();

    Ok(IndependentSetResult {
        node_ids: result_nodes,
    })
}

/// Result of vertex cover computation.
pub struct VertexCoverResult {
    pub node_ids: Vec<i64>,
}

/// Compute minimum vertex cover approximation.
pub fn compute_vertex_cover(src: &[i64], dst: &[i64]) -> Result<VertexCoverResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Ok(VertexCoverResult {
            node_ids: Vec::new(),
        });
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

    let cover = min_weighted_vertex_cover(&graph, None);

    let mut result_nodes = Vec::with_capacity(cover.len());
    for node_id in cover {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            result_nodes.push(ext_id);
        }
    }
    result_nodes.sort();

    Ok(VertexCoverResult {
        node_ids: result_nodes,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_max_clique_simple() {
        // Triangle 1-2-3 (clique of size 3) + isolated node 4
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 1];

        let result = compute_max_clique(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(result.node_ids.contains(&1));
        assert!(result.node_ids.contains(&2));
        assert!(result.node_ids.contains(&3));
    }

    #[test]
    fn test_independent_set() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];
        let result = compute_independent_set(&src, &dst).unwrap();
        assert!(!result.node_ids.is_empty());
    }

    #[test]
    fn test_vertex_cover() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];
        let result = compute_vertex_cover(&src, &dst).unwrap();
        assert!(!result.node_ids.is_empty());
    }
}
