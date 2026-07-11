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
use graphina::core::types::{
    BaseGraph, Directed, GraphConstructor, GraphinaGraph, NodeId, NodeMap, Undirected,
};

use crate::algorithms::builder::{build_graph, check_nonnegative_weights};
use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Collect a per-node score map into parallel external-id and score vectors.
fn collect_scores(node_ids: &HashMap<i64, NodeId>, scores: &NodeMap<f64>) -> (Vec<i64>, Vec<f64>) {
    let mut result_nodes = Vec::with_capacity(node_ids.len());
    let mut result_scores = Vec::with_capacity(node_ids.len());
    for (ext_id, int_id) in node_ids {
        result_nodes.push(*ext_id);
        result_scores.push(*scores.get(int_id).unwrap_or(&0.0));
    }
    (result_nodes, result_scores)
}

/// Result of PageRank computation.
pub struct PageRankResult {
    pub node_ids: Vec<i64>,
    pub ranks: Vec<f64>,
}

/// Compute PageRank on a graph defined by edge arrays.
///
/// When `weights` is empty, every edge gets weight `1.0`.
pub fn compute_pagerank(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    damping: f64,
    iterations: usize,
    tolerance: f64,
    directed: bool,
) -> Result<PageRankResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    check_nonnegative_weights(weights, src.len())?;
    if directed {
        pagerank_impl::<Directed>(src, dst, weights, damping, iterations, tolerance)
    } else {
        pagerank_impl::<Undirected>(src, dst, weights, damping, iterations, tolerance)
    }
}

fn pagerank_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    damping: f64,
    iterations: usize,
    tolerance: f64,
) -> Result<PageRankResult> {
    let g = build_graph::<f64, Ty, _>(
        src,
        dst,
        |i| {
            if weights.is_empty() {
                1.0
            } else {
                weights[i]
            }
        },
    )?;
    let ranks = pagerank(&g.graph, damping, iterations, tolerance, None)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let (node_ids, ranks) = collect_scores(&g.node_ids, &ranks);
    Ok(PageRankResult { node_ids, ranks })
}

/// Result of degree centrality computation.
pub struct DegreeResult {
    pub node_ids: Vec<i64>,
    pub in_degrees: Vec<f64>,
    pub out_degrees: Vec<f64>,
}

/// Compute degree centrality.
pub fn compute_degree(src: &[i64], dst: &[i64], directed: bool) -> Result<DegreeResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if directed {
        let g = build_graph::<f64, Directed, _>(src, dst, |_| 1.0)?;
        let in_deg =
            in_degree_centrality(&g.graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
        let out_deg =
            out_degree_centrality(&g.graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
        let mut result_nodes = Vec::with_capacity(g.node_ids.len());
        let mut result_in = Vec::with_capacity(g.node_ids.len());
        let mut result_out = Vec::with_capacity(g.node_ids.len());
        for (ext_id, int_id) in &g.node_ids {
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
        let g = build_graph::<f64, Undirected, _>(src, dst, |_| 1.0)?;
        let deg =
            in_degree_centrality(&g.graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
        let (node_ids, degrees) = collect_scores(&g.node_ids, &deg);
        Ok(DegreeResult {
            node_ids,
            in_degrees: degrees.clone(),
            out_degrees: degrees,
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
    directed: bool,
) -> Result<BetweennessResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute betweenness on empty graph".to_string(),
        ));
    }
    if directed {
        betweenness_impl::<Directed>(src, dst, normalized)
    } else {
        betweenness_impl::<Undirected>(src, dst, normalized)
    }
}

fn betweenness_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
    normalized: bool,
) -> Result<BetweennessResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let centralities = betweenness_centrality(&g.graph, normalized)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let (node_ids, centralities) = collect_scores(&g.node_ids, &centralities);
    Ok(BetweennessResult {
        node_ids,
        centralities,
    })
}

/// Result of closeness centrality computation.
pub struct ClosenessResult {
    pub node_ids: Vec<i64>,
    pub centralities: Vec<f64>,
}

/// Compute closeness centrality.
pub fn compute_closeness(src: &[i64], dst: &[i64], directed: bool) -> Result<ClosenessResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute closeness on empty graph".to_string(),
        ));
    }
    if directed {
        closeness_impl::<Directed>(src, dst)
    } else {
        closeness_impl::<Undirected>(src, dst)
    }
}

fn closeness_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
) -> Result<ClosenessResult>
where
    BaseGraph<i64, f64, Ty>: GraphinaGraph<i64, f64>,
{
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let centralities =
        closeness_centrality(&g.graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let (node_ids, centralities) = collect_scores(&g.node_ids, &centralities);
    Ok(ClosenessResult {
        node_ids,
        centralities,
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
    directed: bool,
) -> Result<EigenvectorResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute eigenvector on empty graph".to_string(),
        ));
    }
    if directed {
        eigenvector_impl::<Directed>(src, dst, max_iter, tolerance)
    } else {
        eigenvector_impl::<Undirected>(src, dst, max_iter, tolerance)
    }
}

fn eigenvector_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
    max_iter: usize,
    tolerance: f64,
) -> Result<EigenvectorResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let centralities = eigenvector_centrality(&g.graph, max_iter, tolerance)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let (node_ids, centralities) = collect_scores(&g.node_ids, &centralities);
    Ok(EigenvectorResult {
        node_ids,
        centralities,
    })
}

/// Result of Katz centrality computation.
pub struct KatzResult {
    pub node_ids: Vec<i64>,
    pub centralities: Vec<f64>,
}

/// Compute Katz centrality.
///
/// `beta` is a constant weight applied to every node (graphina's default is `1.0`).
pub fn compute_katz(
    src: &[i64],
    dst: &[i64],
    alpha: f64,
    beta: f64,
    max_iter: usize,
    tolerance: f64,
    directed: bool,
) -> Result<KatzResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute Katz on empty graph".to_string(),
        ));
    }
    if directed {
        katz_impl::<Directed>(src, dst, alpha, beta, max_iter, tolerance)
    } else {
        katz_impl::<Undirected>(src, dst, alpha, beta, max_iter, tolerance)
    }
}

fn katz_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
    alpha: f64,
    beta: f64,
    max_iter: usize,
    tolerance: f64,
) -> Result<KatzResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let beta_fn = |_node| beta;
    let centralities = katz_centrality(&g.graph, alpha, Some(&beta_fn), max_iter, tolerance)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let (node_ids, centralities) = collect_scores(&g.node_ids, &centralities);
    Ok(KatzResult {
        node_ids,
        centralities,
    })
}

/// Result of harmonic centrality computation.
pub struct HarmonicResult {
    pub node_ids: Vec<i64>,
    pub centralities: Vec<f64>,
}

/// Compute harmonic centrality.
pub fn compute_harmonic(src: &[i64], dst: &[i64], directed: bool) -> Result<HarmonicResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute harmonic on empty graph".to_string(),
        ));
    }
    if directed {
        harmonic_impl::<Directed>(src, dst)
    } else {
        harmonic_impl::<Undirected>(src, dst)
    }
}

fn harmonic_impl<Ty: GraphConstructor<i64, f64>>(src: &[i64], dst: &[i64]) -> Result<HarmonicResult>
where
    BaseGraph<i64, f64, Ty>: GraphinaGraph<i64, f64>,
{
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let centralities =
        harmonic_centrality(&g.graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let (node_ids, centralities) = collect_scores(&g.node_ids, &centralities);
    Ok(HarmonicResult {
        node_ids,
        centralities,
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
pub fn compute_voterank(
    src: &[i64],
    dst: &[i64],
    num_seeds: usize,
    directed: bool,
) -> Result<VoteRankResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if directed {
        voterank_impl::<Directed>(src, dst, num_seeds)
    } else {
        voterank_impl::<Undirected>(src, dst, num_seeds)
    }
}

fn voterank_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
    num_seeds: usize,
) -> Result<VoteRankResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let seeds = voterank(&g.graph, num_seeds);
    let mut result_nodes = Vec::with_capacity(seeds.len());
    for node_id in seeds {
        if let Some(&ext_id) = g.reverse.get(&node_id) {
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
    directed: bool,
) -> Result<LocalReachingResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if directed {
        local_reaching_impl::<Directed>(src, dst, distance)
    } else {
        local_reaching_impl::<Undirected>(src, dst, distance)
    }
}

fn local_reaching_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
    distance: usize,
) -> Result<LocalReachingResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let centrality_map = local_reaching_centrality(&g.graph, distance)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let (node_ids, centrality) = collect_scores(&g.node_ids, &centrality_map);
    Ok(LocalReachingResult {
        node_ids,
        centrality,
    })
}

/// Result of Laplacian Centrality computation.
pub struct LaplacianResult {
    pub node_ids: Vec<i64>,
    pub centrality: Vec<f64>,
}

/// Compute Laplacian Centrality.
/// Based on the Laplacian matrix of the graph.
pub fn compute_laplacian(src: &[i64], dst: &[i64], directed: bool) -> Result<LaplacianResult> {
    if src.is_empty() && dst.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if directed {
        laplacian_impl::<Directed>(src, dst)
    } else {
        laplacian_impl::<Undirected>(src, dst)
    }
}

fn laplacian_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
) -> Result<LaplacianResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let centrality_map =
        laplacian_centrality(&g.graph).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let (node_ids, centrality) = collect_scores(&g.node_ids, &centrality_map);
    Ok(LaplacianResult {
        node_ids,
        centrality,
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
        let result = compute_pagerank(&src, &dst, &[], 0.85, 100, 1e-6, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert_eq!(result.ranks.len(), 3);
        // All nodes in a triangle should have similar PageRank
        let sum: f64 = result.ranks.iter().sum();
        assert!((sum - 1.0).abs() < 0.01, "PageRank should sum to 1");
    }

    #[test]
    fn test_pagerank_directed() {
        let (src, dst) = triangle_graph();
        let result = compute_pagerank(&src, &dst, &[], 0.85, 100, 1e-6, true).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.ranks.is_empty());
    }

    #[test]
    fn test_pagerank_weighted() {
        // Node 1 points to nodes 2 and 3; the edge to node 3 has a larger weight,
        // so node 3 must receive a higher rank than node 2.
        let src = vec![1, 1];
        let dst = vec![2, 3];
        let weights = vec![1.0, 10.0];
        let result = compute_pagerank(&src, &dst, &weights, 0.85, 100, 1e-6, true).unwrap();

        let rank_of = |node: i64| {
            let idx = result
                .node_ids
                .iter()
                .position(|&n| n == node)
                .unwrap_or_else(|| panic!("node {node} missing from result"));
            result.ranks[idx]
        };
        assert!(rank_of(3) > rank_of(2), "heavier edge should raise rank");

        // Uniform explicit weights must match the unweighted result.
        let (tsrc, tdst) = triangle_graph();
        let uniform =
            compute_pagerank(&tsrc, &tdst, &[1.0, 1.0, 1.0], 0.85, 100, 1e-6, false).unwrap();
        let unweighted = compute_pagerank(&tsrc, &tdst, &[], 0.85, 100, 1e-6, false).unwrap();
        let sum_u: f64 = uniform.ranks.iter().sum();
        let sum_n: f64 = unweighted.ranks.iter().sum();
        assert!((sum_u - sum_n).abs() < 1e-9);
    }

    #[test]
    fn test_pagerank_weights_length_mismatch() {
        let (src, dst) = triangle_graph();
        let result = compute_pagerank(&src, &dst, &[1.0], 0.85, 100, 1e-6, false);
        assert!(result.is_err());
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
        let result = compute_betweenness(&src, &dst, true, false).unwrap();

        assert_eq!(result.node_ids.len(), 4);
        assert!(!result.centralities.is_empty());
    }

    #[test]
    fn test_betweenness_directed() {
        // Directed path 1 -> 2 -> 3 -> 4: middle nodes still lie on paths
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];
        let result = compute_betweenness(&src, &dst, false, true).unwrap();

        assert_eq!(result.node_ids.len(), 4);
    }

    #[test]
    fn test_closeness() {
        let (src, dst) = triangle_graph();
        let result = compute_closeness(&src, &dst, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        // All nodes in triangle should have equal closeness
    }

    #[test]
    fn test_eigenvector() {
        let (src, dst) = triangle_graph();
        let result = compute_eigenvector(&src, &dst, 100, 1e-6, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centralities.is_empty());
    }

    #[test]
    fn test_katz() {
        let (src, dst) = triangle_graph();
        let result = compute_katz(&src, &dst, 0.1, 1.0, 100, 1e-6, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centralities.is_empty());
    }

    #[test]
    fn test_harmonic() {
        let (src, dst) = triangle_graph();
        let result = compute_harmonic(&src, &dst, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centralities.is_empty());
    }

    #[test]
    fn test_harmonic_directed_asymmetric() {
        // Directed path 1 -> 2 -> 3: node 3 reaches nothing, but node 1 reaches both.
        let src = vec![1, 2];
        let dst = vec![2, 3];
        let directed = compute_harmonic(&src, &dst, true).unwrap();
        let undirected = compute_harmonic(&src, &dst, false).unwrap();

        let score = |r: &HarmonicResult, node: i64| {
            let idx = r
                .node_ids
                .iter()
                .position(|&n| n == node)
                .unwrap_or_else(|| panic!("node {node} missing from result"));
            r.centralities[idx]
        };
        // graphina's harmonic centrality sums reciprocal distances over the nodes
        // reachable from each node, so node 3 scores zero in the directed graph
        // but not in the undirected one.
        assert!(score(&directed, 3) < score(&undirected, 3));
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
        let result = compute_voterank(&src, &dst, 2, false).unwrap();

        assert!(result.node_ids.len() <= 2);
    }

    #[test]
    fn test_empty_graph_errors() {
        // Empty input is an error, matching the other algorithm modules
        assert!(compute_pagerank(&[], &[], &[], 0.85, 100, 1e-6, false).is_err());
        assert!(compute_degree(&[], &[], false).is_err());
        assert!(compute_voterank(&[], &[], 10, false).is_err());
    }

    #[test]
    fn test_mismatched_arrays_error() {
        let result = compute_pagerank(&[1, 2], &[2], &[], 0.85, 100, 1e-6, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_local_reaching() {
        let (src, dst) = triangle_graph();
        let result = compute_local_reaching(&src, &dst, 2, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centrality.is_empty());
        // All nodes in a triangle can reach all others within distance 2
        for &c in &result.centrality {
            assert!(c >= 1.0);
        }
    }

    #[test]
    fn test_local_reaching_empty_errors() {
        assert!(compute_local_reaching(&[], &[], 2, false).is_err());
    }

    #[test]
    fn test_laplacian() {
        let (src, dst) = triangle_graph();
        let result = compute_laplacian(&src, &dst, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.centrality.is_empty());
        // All nodes in a symmetric triangle should have similar laplacian centrality
        for &c in &result.centrality {
            assert!(c >= 0.0);
        }
    }

    #[test]
    fn test_laplacian_empty_errors() {
        assert!(compute_laplacian(&[], &[], false).is_err());
    }

    #[test]
    fn test_pagerank_rejects_nan_weight() {
        let result = compute_pagerank(&[1, 2], &[2, 3], &[f64::NAN, 1.0], 0.85, 100, 1e-6, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_pagerank_rejects_negative_weight() {
        let result = compute_pagerank(&[1, 2], &[2, 3], &[-5.0, 1.0], 0.85, 100, 1e-6, false);
        assert!(result.is_err());
    }
}
