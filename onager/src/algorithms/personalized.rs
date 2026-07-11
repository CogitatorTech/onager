//! Personalized PageRank module.
//!
//! Personalized PageRank for node-specific influence computation and recommendations.

use graphina::centrality::personalized_pagerank::personalized_page_rank;
use graphina::core::types::{Directed, GraphConstructor, NodeId, Undirected};

use crate::algorithms::builder::build_graph;
use crate::error::{OnagerError, Result};

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
/// * `directed` - Treat edges as one-way when true
pub fn compute_personalized_pagerank(
    src: &[i64],
    dst: &[i64],
    personalization: &[(i64, f64)],
    damping: f64,
    max_iter: usize,
    tolerance: f64,
    directed: bool,
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
    if damping <= 0.0 || damping >= 1.0 || damping.is_nan() {
        return Err(OnagerError::InvalidArgument(
            "damping must be in (0, 1)".to_string(),
        ));
    }
    if max_iter == 0 {
        return Err(OnagerError::InvalidArgument(
            "max_iter must be positive".to_string(),
        ));
    }
    for &(_, weight) in personalization {
        if weight <= 0.0 || weight.is_nan() {
            return Err(OnagerError::InvalidArgument(
                "personalization weights must be positive".to_string(),
            ));
        }
    }
    if directed {
        personalized_pagerank_impl::<Directed>(
            src,
            dst,
            personalization,
            damping,
            max_iter,
            tolerance,
        )
    } else {
        personalized_pagerank_impl::<Undirected>(
            src,
            dst,
            personalization,
            damping,
            max_iter,
            tolerance,
        )
    }
}

fn personalized_pagerank_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
    personalization: &[(i64, f64)],
    damping: f64,
    max_iter: usize,
    tolerance: f64,
) -> Result<PersonalizedPageRankResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;

    // Build personalization vector aligned with node indices
    let n = g.graph.node_count();
    let node_list: Vec<NodeId> = g.graph.nodes().map(|(id, _)| id).collect();

    let personalization_vec = if personalization.is_empty() {
        None
    } else {
        // Create a personalization vector aligned with node order
        let mut p_vec = vec![0.0; n];
        for &(ext_id, weight) in personalization {
            let node_idx = *g.node_ids.get(&ext_id).ok_or_else(|| {
                OnagerError::InvalidArgument(format!(
                    "Personalization node {} not found in graph",
                    ext_id
                ))
            })?;
            // Find position of node_idx in node_list
            if let Some(pos) = node_list.iter().position(|&id| id == node_idx) {
                p_vec[pos] = weight;
            }
        }
        Some(p_vec)
    };

    let ranks = personalized_page_rank(&g.graph, personalization_vec, damping, tolerance, max_iter)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut node_ids = Vec::with_capacity(ranks.len());
    let mut scores = Vec::with_capacity(ranks.len());
    for (i, &rank) in ranks.iter().enumerate() {
        if let Some(&ext_id) = g.reverse.get(&node_list[i]) {
            node_ids.push(ext_id);
            scores.push(rank);
        }
    }
    Ok(PersonalizedPageRankResult { node_ids, scores })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn triangle_graph() -> (Vec<i64>, Vec<i64>) {
        // Triangle: 1-2-3-1
        (vec![1, 2, 3], vec![2, 3, 1])
    }

    #[test]
    fn test_personalized_pagerank_basic() {
        let (src, dst) = triangle_graph();
        let personalization = vec![(1, 1.0)]; // Bias towards node 1
        let result =
            compute_personalized_pagerank(&src, &dst, &personalization, 0.85, 100, 1e-6, false)
                .unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert_eq!(result.scores.len(), 3);

        // Scores should sum to approximately 1
        let sum: f64 = result.scores.iter().sum();
        assert!((sum - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_personalized_pagerank_empty_personalization() {
        let (src, dst) = triangle_graph();
        let personalization: Vec<(i64, f64)> = vec![];
        let result =
            compute_personalized_pagerank(&src, &dst, &personalization, 0.85, 100, 1e-6, false)
                .unwrap();

        // With no personalization, should work like regular PageRank
        assert_eq!(result.node_ids.len(), 3);
    }

    #[test]
    fn test_personalized_pagerank_multiple_sources() {
        let (src, dst) = triangle_graph();
        let personalization = vec![(1, 0.5), (2, 0.5)]; // Split between nodes 1 and 2
        let result =
            compute_personalized_pagerank(&src, &dst, &personalization, 0.85, 100, 1e-6, false)
                .unwrap();

        assert_eq!(result.node_ids.len(), 3);
    }

    #[test]
    fn test_personalized_pagerank_star_graph() {
        // Star: hub 1 connected to 2, 3, 4
        let src = vec![1, 1, 1];
        let dst = vec![2, 3, 4];
        let personalization = vec![(1, 1.0)]; // Bias towards hub
        let result =
            compute_personalized_pagerank(&src, &dst, &personalization, 0.85, 100, 1e-6, false)
                .unwrap();

        assert_eq!(result.node_ids.len(), 4);
    }

    #[test]
    fn test_personalized_pagerank_directed() {
        // Directed star: hub 1 points to 2, 3, 4; nothing points back to the hub.
        let src = vec![1, 1, 1];
        let dst = vec![2, 3, 4];
        let result = compute_personalized_pagerank(&src, &dst, &[], 0.85, 100, 1e-6, true).unwrap();

        assert_eq!(result.node_ids.len(), 4);
        let sum: f64 = result.scores.iter().sum();
        assert!((sum - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_personalized_pagerank_empty_graph_error() {
        let result = compute_personalized_pagerank(&[], &[], &[], 0.85, 100, 1e-6, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_personalized_pagerank_invalid_damping() {
        let (src, dst) = triangle_graph();

        // damping >= 1 should fail
        let result = compute_personalized_pagerank(&src, &dst, &[], 1.0, 100, 1e-6, false);
        assert!(result.is_err());

        // damping <= 0 should fail
        let result2 = compute_personalized_pagerank(&src, &dst, &[], 0.0, 100, 1e-6, false);
        assert!(result2.is_err());
    }

    #[test]
    fn test_personalized_pagerank_invalid_max_iter() {
        let (src, dst) = triangle_graph();
        let result = compute_personalized_pagerank(&src, &dst, &[], 0.85, 0, 1e-6, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_personalized_pagerank_mismatched_arrays() {
        let result = compute_personalized_pagerank(&[1, 2], &[2], &[], 0.85, 100, 1e-6, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_personalized_pagerank_unknown_node_errors() {
        let (src, dst) = triangle_graph();
        let personalization = vec![(999, 1.0)];
        let result =
            compute_personalized_pagerank(&src, &dst, &personalization, 0.85, 100, 1e-6, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_personalized_pagerank_nonpositive_weight_errors() {
        let (src, dst) = triangle_graph();
        for weight in [0.0, -1.0, f64::NAN] {
            let personalization = vec![(1, weight)];
            let result =
                compute_personalized_pagerank(&src, &dst, &personalization, 0.85, 100, 1e-6, false);
            assert!(result.is_err());
        }
    }
}
