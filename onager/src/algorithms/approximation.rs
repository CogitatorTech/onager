//! Approximation algorithms module.
//!
//! Maximum Clique, Independent Set, Vertex Cover, TSP.

use graphina::approximation::clique::max_clique;
use graphina::approximation::independent_set::maximum_independent_set;
use graphina::approximation::tsp::traveling_salesman_problem;
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

/// Result of TSP computation.
pub struct TspResult {
    pub tour: Vec<i64>,
    pub cost: f64,
}

/// Compute Traveling Salesman Problem approximation using greedy nearest-neighbor.
pub fn compute_tsp(src: &[i64], dst: &[i64], weights: &[f64]) -> Result<TspResult> {
    if src.len() != dst.len() || src.len() != weights.len() {
        return Err(OnagerError::InvalidArgument(
            "src, dst, and weights arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute TSP on empty graph".to_string(),
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
        graph.add_edge(src_id, dst_id, weights[i]);
    }

    let (tour_internal, cost) =
        traveling_salesman_problem(&graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut tour = Vec::with_capacity(tour_internal.len());
    for node_id in tour_internal {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            tour.push(ext_id);
        }
    }

    Ok(TspResult { tour, cost })
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

    #[test]
    fn test_tsp_square() {
        // Square graph (4 nodes in a cycle) - should visit all nodes
        let src = vec![1, 2, 3, 4];
        let dst = vec![2, 3, 4, 1];
        let weights = vec![1.0, 1.0, 1.0, 1.0];

        let result = compute_tsp(&src, &dst, &weights).unwrap();

        // Tour should include all 4 nodes plus return to start
        assert!(result.tour.len() >= 5);
        assert!(result.cost > 0.0);
        // First and last should be the same (return to start)
        assert_eq!(result.tour.first(), result.tour.last());
    }

    #[test]
    fn test_tsp_empty_error() {
        let result = compute_tsp(&[], &[], &[]);
        assert!(result.is_err());
    }
}
