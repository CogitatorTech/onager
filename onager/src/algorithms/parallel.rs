//! Parallel algorithms module.
//!
//! Parallel PageRank using rayon.

use graphina::core::types::{Digraph, Graph, NodeId};
use graphina::parallel::pagerank_parallel;

use crate::algorithms::centrality::PageRankResult;
use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Compute PageRank using parallel algorithm.
pub fn compute_pagerank_parallel(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    damping: f64,
    iterations: usize,
    directed: bool,
) -> Result<PageRankResult> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument("src and dst arrays must have same length".to_string()));
    }
    if !weights.is_empty() && weights.len() != src.len() {
        return Err(OnagerError::InvalidArgument("weights must be empty or same length as edges".to_string()));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument("Cannot compute on empty graph".to_string()));
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
            let weight = if weights.is_empty() { 1.0 } else { weights[i] };
            graph.add_edge(src_id, dst_id, weight);
        }

        let ranks = pagerank_parallel(&graph, damping, iterations, 1e-6, None);
        let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();

        let mut node_ids = Vec::with_capacity(ranks.len());
        let mut rank_values = Vec::with_capacity(ranks.len());
        for (node_id, rank) in ranks {
            if let Some(&ext_id) = reverse_map.get(&node_id) {
                node_ids.push(ext_id);
                rank_values.push(rank);
            }
        }
        Ok(PageRankResult { node_ids, ranks: rank_values })
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
            let weight = if weights.is_empty() { 1.0 } else { weights[i] };
            graph.add_edge(src_id, dst_id, weight);
        }

        let ranks = pagerank_parallel(&graph, damping, iterations, 1e-6, None);
        let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();

        let mut node_ids = Vec::with_capacity(ranks.len());
        let mut rank_values = Vec::with_capacity(ranks.len());
        for (node_id, rank) in ranks {
            if let Some(&ext_id) = reverse_map.get(&node_id) {
                node_ids.push(ext_id);
                rank_values.push(rank);
            }
        }
        Ok(PageRankResult { node_ids, ranks: rank_values })
    }
}
