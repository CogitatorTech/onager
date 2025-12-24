//! Subgraph operations module.
//!
//! Ego graph, k-hop neighbors, induced subgraph.

use graphina::core::types::{Graph, NodeId};
use graphina::subgraphs::SubgraphOps;

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Result of ego graph extraction.
pub struct EgoGraphResult {
    pub src: Vec<i64>,
    pub dst: Vec<i64>,
}

/// Extract an ego graph centered on a node with a given radius.
/// Returns edges of the subgraph containing all nodes within `radius` hops of `center`.
pub fn compute_ego_graph(
    src: &[i64],
    dst: &[i64],
    center: i64,
    radius: usize,
) -> Result<EgoGraphResult> {
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

    let center_id = node_set
        .get(&center)
        .ok_or(OnagerError::NodeNotFound(center))?;

    let ego = graph
        .ego_graph(*center_id, radius)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_src = Vec::new();
    let mut result_dst = Vec::new();
    for (u, v, _) in ego.edges() {
        if let (Some(&ext_u), Some(&ext_v)) = (reverse_map.get(&u), reverse_map.get(&v)) {
            result_src.push(ext_u);
            result_dst.push(ext_v);
        }
    }
    Ok(EgoGraphResult {
        src: result_src,
        dst: result_dst,
    })
}

/// Result of k-hop neighbor computation.
pub struct KHopNeighborsResult {
    pub node_ids: Vec<i64>,
}

/// Find all nodes within k hops of a start node.
pub fn compute_k_hop_neighbors(
    src: &[i64],
    dst: &[i64],
    start: i64,
    k: usize,
) -> Result<KHopNeighborsResult> {
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

    let start_id = node_set
        .get(&start)
        .ok_or(OnagerError::NodeNotFound(start))?;

    let neighbors = graph.k_hop_neighbors(*start_id, k);
    let node_ids: Vec<i64> = neighbors
        .into_iter()
        .filter_map(|id| reverse_map.get(&id).copied())
        .collect();

    Ok(KHopNeighborsResult { node_ids })
}

/// Result of induced subgraph extraction.
pub struct InducedSubgraphResult {
    pub src: Vec<i64>,
    pub dst: Vec<i64>,
}

/// Extract an induced subgraph containing only the specified nodes and edges between them.
pub fn compute_induced_subgraph(
    src: &[i64],
    dst: &[i64],
    node_ids: &[i64],
) -> Result<InducedSubgraphResult> {
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
    if node_ids.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "node_ids must not be empty".to_string(),
        ));
    }

    let mut node_map: HashMap<i64, NodeId> = HashMap::new();
    let mut reverse_map: HashMap<NodeId, i64> = HashMap::new();
    let mut graph: Graph<i64, f64> = Graph::new();

    for &node in src.iter().chain(dst.iter()) {
        if !node_map.contains_key(&node) {
            let id = graph.add_node(node);
            node_map.insert(node, id);
            reverse_map.insert(id, node);
        }
    }
    for i in 0..src.len() {
        let src_id = node_map[&src[i]];
        let dst_id = node_map[&dst[i]];
        graph.add_edge(src_id, dst_id, 1.0);
    }

    // Convert external node IDs to internal NodeIds
    let selected_nodes: std::collections::HashSet<NodeId> = node_ids
        .iter()
        .filter_map(|&ext_id| node_map.get(&ext_id).copied())
        .collect();

    let subgraph = graph
        .induced_subgraph(&selected_nodes)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_src = Vec::new();
    let mut result_dst = Vec::new();
    for (u, v, _) in subgraph.edges() {
        if let (Some(&ext_u), Some(&ext_v)) = (reverse_map.get(&u), reverse_map.get(&v)) {
            result_src.push(ext_u);
            result_dst.push(ext_v);
        }
    }
    Ok(InducedSubgraphResult {
        src: result_src,
        dst: result_dst,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn star_graph() -> (Vec<i64>, Vec<i64>) {
        // Star: hub 1 connected to 2, 3, 4
        (vec![1, 1, 1], vec![2, 3, 4])
    }

    fn path_graph() -> (Vec<i64>, Vec<i64>) {
        // Path: 1-2-3-4-5
        (vec![1, 2, 3, 4], vec![2, 3, 4, 5])
    }

    #[test]
    fn test_ego_graph_star_center() {
        let (src, dst) = star_graph();
        let result = compute_ego_graph(&src, &dst, 1, 1).unwrap();

        // Ego graph of hub with radius 1 should include all edges
        assert!(!result.src.is_empty());
        assert_eq!(result.src.len(), result.dst.len());
    }

    #[test]
    fn test_ego_graph_star_leaf() {
        let (src, dst) = star_graph();
        let result = compute_ego_graph(&src, &dst, 2, 1).unwrap();

        // Ego graph of leaf with radius 1 should include edge to hub
        assert!(!result.src.is_empty());
    }

    #[test]
    fn test_ego_graph_radius_zero() {
        let (src, dst) = star_graph();
        let result = compute_ego_graph(&src, &dst, 1, 0).unwrap();

        // Radius 0 should return just the center node (no edges)
        assert!(result.src.is_empty());
    }

    #[test]
    fn test_ego_graph_invalid_center() {
        let (src, dst) = star_graph();
        let result = compute_ego_graph(&src, &dst, 999, 1);
        assert!(result.is_err());
    }

    #[test]
    fn test_k_hop_neighbors_star_hub() {
        let (src, dst) = star_graph();
        let result = compute_k_hop_neighbors(&src, &dst, 1, 1).unwrap();

        // Hub's 1-hop neighbors are all leaves (2, 3, 4)
        // Note: typically includes the start node too
        assert!(!result.node_ids.is_empty());
    }

    #[test]
    fn test_k_hop_neighbors_path() {
        let (src, dst) = path_graph();
        let result = compute_k_hop_neighbors(&src, &dst, 1, 2).unwrap();

        // From node 1, 2-hop should reach nodes 1, 2, 3
        assert!(!result.node_ids.is_empty());
    }

    #[test]
    fn test_k_hop_neighbors_zero() {
        let (src, dst) = star_graph();
        let result = compute_k_hop_neighbors(&src, &dst, 1, 0).unwrap();

        // 0-hop should return just the start node
        assert_eq!(result.node_ids.len(), 1);
        assert!(result.node_ids.contains(&1));
    }

    #[test]
    fn test_k_hop_neighbors_invalid_start() {
        let (src, dst) = star_graph();
        let result = compute_k_hop_neighbors(&src, &dst, 999, 1);
        assert!(result.is_err());
    }

    #[test]
    fn test_induced_subgraph_basic() {
        let (src, dst) = path_graph();
        // Select nodes 2, 3, 4 from path 1-2-3-4-5
        let node_ids = vec![2, 3, 4];
        let result = compute_induced_subgraph(&src, &dst, &node_ids).unwrap();

        // Should include edges 2-3 and 3-4
        assert!(!result.src.is_empty());
        assert_eq!(result.src.len(), result.dst.len());
    }

    #[test]
    fn test_induced_subgraph_single_node() {
        let (src, dst) = star_graph();
        let node_ids = vec![1];
        let result = compute_induced_subgraph(&src, &dst, &node_ids).unwrap();

        // Single node has no edges to itself
        assert!(result.src.is_empty());
    }

    #[test]
    fn test_induced_subgraph_disconnected_nodes() {
        let (src, dst) = star_graph();
        // Select only leaves (no edges between them)
        let node_ids = vec![2, 3, 4];
        let result = compute_induced_subgraph(&src, &dst, &node_ids).unwrap();

        // Leaves in star graph are not connected to each other
        assert!(result.src.is_empty());
    }

    #[test]
    fn test_induced_subgraph_empty_nodes_error() {
        let (src, dst) = star_graph();
        let result = compute_induced_subgraph(&src, &dst, &[]);
        assert!(result.is_err());
    }

    #[test]
    fn test_empty_graph_errors() {
        assert!(compute_ego_graph(&[], &[], 1, 1).is_err());
        assert!(compute_k_hop_neighbors(&[], &[], 1, 1).is_err());
        assert!(compute_induced_subgraph(&[], &[], &[1]).is_err());
    }

    #[test]
    fn test_mismatched_arrays_error() {
        assert!(compute_ego_graph(&[1, 2], &[2], 1, 1).is_err());
        assert!(compute_k_hop_neighbors(&[1, 2], &[2], 1, 1).is_err());
        assert!(compute_induced_subgraph(&[1, 2], &[2], &[1, 2]).is_err());
    }
}
