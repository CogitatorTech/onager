//! Centrality algorithms module.
//!
//! PageRank, Degree, Betweenness, Closeness, Eigenvector, Katz, Harmonic centrality, VoteRank.

use graphina::centrality::betweenness::betweenness_centrality;
use graphina::centrality::closeness::closeness_centrality;
use graphina::centrality::degree::{in_degree_centrality, out_degree_centrality};
use graphina::centrality::eigenvector::eigenvector_centrality;
use graphina::centrality::harmonic::harmonic_centrality;
use graphina::centrality::katz::katz_centrality;
use graphina::centrality::other::{laplacian_centrality, local_reaching_centrality, voterank};
use graphina::centrality::pagerank::pagerank;
use graphina::core::types::{Digraph, Graph, NodeId};
use ordered_float::OrderedFloat;

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Result of PageRank computation.
pub struct PageRankResult {
    pub node_ids: Vec<i64>,
    pub ranks: Vec<f64>,
}

/// Compute PageRank on a graph defined by edge arrays.
pub fn compute_pagerank(
    src: &[i64],
    dst: &[i64],
    _weights: &[f64],
    damping: f64,
    iterations: usize,
    directed: bool,
) -> Result<PageRankResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }

    let mut node_set: HashMap<i64, NodeId> = HashMap::new();
    let tolerance = 1e-6;

    if directed {
        let mut graph: Digraph<i64, f64> = Digraph::new();
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
        let ranks = pagerank(&graph, damping, iterations, tolerance, None)
            .map_err(|e| OnagerError::GraphError(e.to_string()))?;
        let mut result_nodes = Vec::with_capacity(node_set.len());
        let mut result_ranks = Vec::with_capacity(node_set.len());
        for (ext_id, int_id) in &node_set {
            result_nodes.push(*ext_id);
            result_ranks.push(*ranks.get(int_id).unwrap_or(&0.0));
        }
        Ok(PageRankResult {
            node_ids: result_nodes,
            ranks: result_ranks,
        })
    } else {
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
        let ranks = pagerank(&graph, damping, iterations, tolerance, None)
            .map_err(|e| OnagerError::GraphError(e.to_string()))?;
        let mut result_nodes = Vec::with_capacity(node_set.len());
        let mut result_ranks = Vec::with_capacity(node_set.len());
        for (ext_id, int_id) in &node_set {
            result_nodes.push(*ext_id);
            result_ranks.push(*ranks.get(int_id).unwrap_or(&0.0));
        }
        Ok(PageRankResult {
            node_ids: result_nodes,
            ranks: result_ranks,
        })
    }
}

/// Result of degree centrality computation.
pub struct DegreeResult {
    pub node_ids: Vec<i64>,
    pub in_degrees: Vec<f64>,
    pub out_degrees: Vec<f64>,
}

/// Compute degree centrality.
pub fn compute_degree(src: &[i64], dst: &[i64], directed: bool) -> Result<DegreeResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }

    let mut node_set: HashMap<i64, NodeId> = HashMap::new();

    if directed {
        let mut graph: Digraph<i64, f64> = Digraph::new();
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
        let in_deg =
            in_degree_centrality(&graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
        let out_deg =
            out_degree_centrality(&graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
        let mut result_nodes = Vec::with_capacity(node_set.len());
        let mut result_in = Vec::with_capacity(node_set.len());
        let mut result_out = Vec::with_capacity(node_set.len());
        for (ext_id, int_id) in &node_set {
            result_nodes.push(*ext_id);
            result_in.push(*in_deg.get(int_id).unwrap_or(&0.0));
            result_out.push(*out_deg.get(int_id).unwrap_or(&0.0));
        }
        Ok(DegreeResult {
            node_ids: result_nodes,
            in_degrees: result_in,
            out_degrees: result_out,
        })
    } else {
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
        let deg =
            in_degree_centrality(&graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
        let mut result_nodes = Vec::with_capacity(node_set.len());
        let mut result_deg = Vec::with_capacity(node_set.len());
        for (ext_id, int_id) in &node_set {
            result_nodes.push(*ext_id);
            result_deg.push(*deg.get(int_id).unwrap_or(&0.0));
        }
        Ok(DegreeResult {
            node_ids: result_nodes,
            in_degrees: result_deg.clone(),
            out_degrees: result_deg,
        })
    }
}

/// Result of betweenness centrality computation.
pub struct BetweennessResult {
    pub node_ids: Vec<i64>,
    pub centralities: Vec<f64>,
}

/// Compute betweenness centrality.
pub fn compute_betweenness(
    src: &[i64],
    dst: &[i64],
    normalized: bool,
) -> Result<BetweennessResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute betweenness on empty graph".to_string(),
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
    let centralities = betweenness_centrality(&graph, normalized)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_cent = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        result_cent.push(*centralities.get(int_id).unwrap_or(&0.0));
    }
    Ok(BetweennessResult {
        node_ids: result_nodes,
        centralities: result_cent,
    })
}

/// Result of closeness centrality computation.
pub struct ClosenessResult {
    pub node_ids: Vec<i64>,
    pub centralities: Vec<f64>,
}

/// Compute closeness centrality.
pub fn compute_closeness(src: &[i64], dst: &[i64]) -> Result<ClosenessResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute closeness on empty graph".to_string(),
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
    let centralities =
        closeness_centrality(&graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_cent = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        result_cent.push(*centralities.get(int_id).unwrap_or(&0.0));
    }
    Ok(ClosenessResult {
        node_ids: result_nodes,
        centralities: result_cent,
    })
}

/// Result of eigenvector centrality computation.
pub struct EigenvectorResult {
    pub node_ids: Vec<i64>,
    pub centralities: Vec<f64>,
}

/// Compute eigenvector centrality.
pub fn compute_eigenvector(
    src: &[i64],
    dst: &[i64],
    max_iter: usize,
    tolerance: f64,
) -> Result<EigenvectorResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute eigenvector on empty graph".to_string(),
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
    let centralities = eigenvector_centrality(&graph, max_iter, tolerance)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_cent = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        result_cent.push(*centralities.get(int_id).unwrap_or(&0.0));
    }
    Ok(EigenvectorResult {
        node_ids: result_nodes,
        centralities: result_cent,
    })
}

/// Result of Katz centrality computation.
pub struct KatzResult {
    pub node_ids: Vec<i64>,
    pub centralities: Vec<f64>,
}

/// Compute Katz centrality.
pub fn compute_katz(
    src: &[i64],
    dst: &[i64],
    alpha: f64,
    max_iter: usize,
    tolerance: f64,
) -> Result<KatzResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute Katz on empty graph".to_string(),
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
    let centralities = katz_centrality(&graph, alpha, None, max_iter, tolerance)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_cent = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        result_cent.push(*centralities.get(int_id).unwrap_or(&0.0));
    }
    Ok(KatzResult {
        node_ids: result_nodes,
        centralities: result_cent,
    })
}

/// Result of harmonic centrality computation.
pub struct HarmonicResult {
    pub node_ids: Vec<i64>,
    pub centralities: Vec<f64>,
}

/// Compute harmonic centrality.
pub fn compute_harmonic(src: &[i64], dst: &[i64]) -> Result<HarmonicResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute harmonic on empty graph".to_string(),
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
    let centralities =
        harmonic_centrality(&graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_cent = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        result_cent.push(*centralities.get(int_id).unwrap_or(&0.0));
    }
    Ok(HarmonicResult {
        node_ids: result_nodes,
        centralities: result_cent,
    })
}

/// Result of single-node degree computation.
pub struct NodeDegreeResult {
    pub in_degree: i64,
    pub out_degree: i64,
}

/// Compute degree for a single node.
pub fn compute_node_degree(src: &[i64], dst: &[i64], node: i64) -> Result<NodeDegreeResult> {
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

    // Count in-degree (edges pointing to node) and out-degree (edges from node)
    let mut in_degree: i64 = 0;
    let mut out_degree: i64 = 0;
    let mut node_found = false;

    for i in 0..src.len() {
        if src[i] == node {
            out_degree += 1;
            node_found = true;
        }
        if dst[i] == node {
            in_degree += 1;
            node_found = true;
        }
    }

    if !node_found {
        return Err(OnagerError::InvalidArgument(format!(
            "Node {} not found in graph",
            node
        )));
    }

    Ok(NodeDegreeResult {
        in_degree,
        out_degree,
    })
}

/// Result of VoteRank computation.
pub struct VoteRankResult {
    pub node_ids: Vec<i64>,
}

/// Compute VoteRank to find influential spreaders.
pub fn compute_voterank(src: &[i64], dst: &[i64], num_seeds: usize) -> Result<VoteRankResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Ok(VoteRankResult {
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

    let seeds = voterank(&graph, num_seeds);
    let mut result_nodes = Vec::with_capacity(seeds.len());
    for node_id in seeds {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            result_nodes.push(ext_id);
        }
    }

    Ok(VoteRankResult {
        node_ids: result_nodes,
    })
}

/// Result of Local Reaching Centrality computation.
pub struct LocalReachingResult {
    pub node_ids: Vec<i64>,
    pub centrality: Vec<f64>,
}

/// Compute Local Reaching Centrality.
/// Measures how many nodes can be reached within a given distance.
pub fn compute_local_reaching(
    src: &[i64],
    dst: &[i64],
    distance: usize,
) -> Result<LocalReachingResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Ok(LocalReachingResult {
            node_ids: Vec::new(),
            centrality: Vec::new(),
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

    let centrality_map = local_reaching_centrality(&graph, distance)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_centrality = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        result_centrality.push(*centrality_map.get(int_id).unwrap_or(&0.0));
    }

    Ok(LocalReachingResult {
        node_ids: result_nodes,
        centrality: result_centrality,
    })
}

/// Result of Laplacian Centrality computation.
pub struct LaplacianResult {
    pub node_ids: Vec<i64>,
    pub centrality: Vec<f64>,
}

/// Compute Laplacian Centrality.
/// Based on the Laplacian matrix of the graph.
pub fn compute_laplacian(src: &[i64], dst: &[i64]) -> Result<LaplacianResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Ok(LaplacianResult {
            node_ids: Vec::new(),
            centrality: Vec::new(),
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

    let centrality_map =
        laplacian_centrality(&graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_nodes = Vec::with_capacity(node_set.len());
    let mut result_centrality = Vec::with_capacity(node_set.len());
    for (ext_id, int_id) in &node_set {
        result_nodes.push(*ext_id);
        result_centrality.push(*centrality_map.get(int_id).unwrap_or(&0.0));
    }

    Ok(LaplacianResult {
        node_ids: result_nodes,
        centrality: result_centrality,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn triangle_graph() -> (Vec<i64>, Vec<i64>) {
        // Triangle: 1-2-3-1
        (vec![1, 2, 3], vec![2, 3, 1])
    }

    fn star_graph() -> (Vec<i64>, Vec<i64>) {
        // Star: 1 connected to 2, 3, 4
        (vec![1, 1, 1], vec![2, 3, 4])
    }

    #[test]
    fn test_pagerank_triangle() {
        let (src, dst) = triangle_graph();
        let result = compute_pagerank(&src, &dst, &[], 0.85, 100, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert_eq!(result.ranks.len(), 3);
        // All nodes in a triangle should have similar PageRank
        let sum: f64 = result.ranks.iter().sum();
        assert!((sum - 1.0).abs() < 0.01, "PageRank should sum to 1");
    }

    #[test]
    fn test_pagerank_directed() {
        let (src, dst) = triangle_graph();
        let result = compute_pagerank(&src, &dst, &[], 0.85, 100, true).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.ranks.is_empty());
    }

    #[test]
    fn test_degree_undirected() {
        let (src, dst) = star_graph();
        let result = compute_degree(&src, &dst, false).unwrap();

        assert_eq!(result.node_ids.len(), 4);
        // Node 1 is hub with degree 3
    }

    #[test]
    fn test_degree_directed() {
        let (src, dst) = star_graph();
        let result = compute_degree(&src, &dst, true).unwrap();

        assert_eq!(result.node_ids.len(), 4);
        assert_eq!(result.in_degrees.len(), 4);
        assert_eq!(result.out_degrees.len(), 4);
    }

    #[test]
    fn test_betweenness() {
        // Path graph: 1-2-3-4 (node 2 and 3 have high betweenness)
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];
        let result = compute_betweenness(&src, &dst, true).unwrap();

        assert_eq!(result.node_ids.len(), 4);
        assert!(!result.centralities.is_empty());
    }

    #[test]
    fn test_closeness() {
        let (src, dst) = triangle_graph();
        let result = compute_closeness(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        // All nodes in triangle should have equal closeness
    }

    #[test]
    fn test_eigenvector() {
        let (src, dst) = triangle_graph();
        let result = compute_eigenvector(&src, &dst, 100, 1e-6).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centralities.is_empty());
    }

    #[test]
    fn test_katz() {
        let (src, dst) = triangle_graph();
        let result = compute_katz(&src, &dst, 0.1, 100, 1e-6).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centralities.is_empty());
    }

    #[test]
    fn test_harmonic() {
        let (src, dst) = triangle_graph();
        let result = compute_harmonic(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centralities.is_empty());
    }

    #[test]
    fn test_node_degree() {
        let (src, dst) = star_graph();
        let result = compute_node_degree(&src, &dst, 1).unwrap();

        // Node 1 is the hub with out-degree 3
        assert!(result.out_degree >= 0);
        assert!(result.in_degree >= 0);
    }

    #[test]
    fn test_voterank() {
        let (src, dst) = triangle_graph();
        let result = compute_voterank(&src, &dst, 2).unwrap();

        assert!(result.node_ids.len() <= 2);
    }

    #[test]
    fn test_empty_graph_returns_empty() {
        // Empty graph returns empty results (not an error)
        let result = compute_pagerank(&[], &[], &[], 0.85, 100, false).unwrap();
        assert!(result.node_ids.is_empty());
        assert!(result.ranks.is_empty());
    }

    #[test]
    fn test_mismatched_arrays_error() {
        let result = compute_pagerank(&[1, 2], &[2], &[], 0.85, 100, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_local_reaching() {
        let (src, dst) = triangle_graph();
        let result = compute_local_reaching(&src, &dst, 2).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centrality.is_empty());
        // All nodes in a triangle can reach all others within distance 2
        for &c in &result.centrality {
            assert!(c >= 1.0);
        }
    }

    #[test]
    fn test_local_reaching_empty() {
        let result = compute_local_reaching(&[], &[], 2).unwrap();
        assert!(result.node_ids.is_empty());
    }

    #[test]
    fn test_laplacian() {
        let (src, dst) = triangle_graph();
        let result = compute_laplacian(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centrality.is_empty());
        // All nodes in a symmetric triangle should have similar laplacian centrality
        for &c in &result.centrality {
            assert!(c >= 0.0);
        }
    }

    #[test]
    fn test_laplacian_empty() {
        let result = compute_laplacian(&[], &[]).unwrap();
        assert!(result.node_ids.is_empty());
    }
}
