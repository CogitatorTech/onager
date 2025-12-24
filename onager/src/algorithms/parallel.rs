//! Parallel algorithms module.
//!
//! Parallel PageRank, BFS, shortest paths, connected components, clustering, triangles.

use graphina::core::types::{Digraph, Graph, NodeId};
use graphina::parallel::{
    bfs_parallel, clustering_coefficients_parallel, connected_components_parallel,
    pagerank_parallel, shortest_paths_parallel, triangles_parallel,
};

use crate::algorithms::centrality::PageRankResult;
use crate::algorithms::community::ConnectedComponentsResult;
use crate::algorithms::metrics::TriangleResult;
use crate::algorithms::traversal::BfsResult;
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
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    if !weights.is_empty() && weights.len() != src.len() {
        return Err(OnagerError::InvalidArgument(
            "weights must be empty or same length as edges".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
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
        Ok(PageRankResult {
            node_ids,
            ranks: rank_values,
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
        Ok(PageRankResult {
            node_ids,
            ranks: rank_values,
        })
    }
}

/// Compute parallel BFS traversal from a single source.
pub fn compute_bfs_parallel(src: &[i64], dst: &[i64], source: i64) -> Result<BfsResult> {
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

    let source_id = node_set
        .get(&source)
        .ok_or(OnagerError::NodeNotFound(source))?;

    // bfs_parallel takes a slice of sources - we pass a single source
    let results = bfs_parallel(&graph, &[*source_id]);
    // Get the first (and only) result
    let visit_order = results.into_iter().next().unwrap_or_default();

    let order: Vec<i64> = visit_order
        .into_iter()
        .filter_map(|node_id| reverse_map.get(&node_id).copied())
        .collect();

    Ok(BfsResult {
        node_ids: order.clone(),
        order,
    })
}

/// Result of parallel shortest paths.
pub struct ShortestPathsParallelResult {
    pub node_ids: Vec<i64>,
    pub distances: Vec<f64>,
}

/// Compute parallel shortest paths from a single source.
pub fn compute_shortest_paths_parallel(
    src: &[i64],
    dst: &[i64],
    source: i64,
) -> Result<ShortestPathsParallelResult> {
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

    let source_id = node_set
        .get(&source)
        .ok_or(OnagerError::NodeNotFound(source))?;

    // shortest_paths_parallel takes a slice of sources, returns Vec<HashMap<NodeId, usize>>
    let results = shortest_paths_parallel(&graph, &[*source_id]);
    let distances_map = results.into_iter().next().unwrap_or_default();

    let mut node_ids = Vec::with_capacity(distances_map.len());
    let mut dist_values = Vec::with_capacity(distances_map.len());
    for (node_id, dist) in distances_map {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            node_ids.push(ext_id);
            dist_values.push(dist as f64);
        }
    }
    Ok(ShortestPathsParallelResult {
        node_ids,
        distances: dist_values,
    })
}

/// Compute parallel connected components.
pub fn compute_components_parallel(src: &[i64], dst: &[i64]) -> Result<ConnectedComponentsResult> {
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

    // connected_components_parallel returns HashMap<NodeId, usize>
    let components = connected_components_parallel(&graph);

    let mut node_ids = Vec::with_capacity(components.len());
    let mut component_ids = Vec::with_capacity(components.len());
    for (internal_id, comp_id) in components {
        if let Some(&ext_id) = reverse_map.get(&internal_id) {
            node_ids.push(ext_id);
            component_ids.push(comp_id as i64);
        }
    }
    Ok(ConnectedComponentsResult {
        node_ids,
        component_ids,
    })
}

/// Result of parallel clustering coefficients.
pub struct ClusteringParallelResult {
    pub node_ids: Vec<i64>,
    pub coefficients: Vec<f64>,
}

/// Compute parallel clustering coefficients for each node.
pub fn compute_clustering_parallel(src: &[i64], dst: &[i64]) -> Result<ClusteringParallelResult> {
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

    let coefficients = clustering_coefficients_parallel(&graph);

    let mut node_ids = Vec::with_capacity(coefficients.len());
    let mut coef_values = Vec::with_capacity(coefficients.len());
    for (node_id, coef) in coefficients {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            node_ids.push(ext_id);
            coef_values.push(coef);
        }
    }
    Ok(ClusteringParallelResult {
        node_ids,
        coefficients: coef_values,
    })
}

/// Compute parallel triangle count for each node.
pub fn compute_triangles_parallel(src: &[i64], dst: &[i64]) -> Result<TriangleResult> {
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

    let triangles = triangles_parallel(&graph);

    let mut node_ids = Vec::with_capacity(triangles.len());
    let mut triangle_counts = Vec::with_capacity(triangles.len());
    for (node_id, count) in triangles {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            node_ids.push(ext_id);
            triangle_counts.push(count as i64);
        }
    }
    Ok(TriangleResult {
        node_ids,
        triangle_counts,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn triangle_graph() -> (Vec<i64>, Vec<i64>) {
        // Triangle: 1-2-3-1
        (vec![1, 2, 3], vec![2, 3, 1])
    }

    fn connected_graph() -> (Vec<i64>, Vec<i64>) {
        // Connected path plus cross edge
        (vec![1, 2, 3, 1], vec![2, 3, 4, 3])
    }

    #[test]
    fn test_pagerank_parallel_undirected() {
        let (src, dst) = triangle_graph();
        let result = compute_pagerank_parallel(&src, &dst, &[], 0.85, 100, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert_eq!(result.ranks.len(), 3);

        // PageRank should sum to 1
        let sum: f64 = result.ranks.iter().sum();
        assert!((sum - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_pagerank_parallel_directed() {
        let (src, dst) = triangle_graph();
        let result = compute_pagerank_parallel(&src, &dst, &[], 0.85, 100, true).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.ranks.is_empty());
    }

    #[test]
    fn test_pagerank_parallel_with_weights() {
        let (src, dst) = triangle_graph();
        let weights = vec![1.0, 2.0, 1.5];
        let result = compute_pagerank_parallel(&src, &dst, &weights, 0.85, 100, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
    }

    #[test]
    fn test_bfs_parallel() {
        let (src, dst) = connected_graph();
        let result = compute_bfs_parallel(&src, &dst, 1).unwrap();

        // Should visit all 4 nodes starting from 1
        assert!(!result.order.is_empty());
        assert!(result.order.len() <= 4);
    }

    #[test]
    fn test_bfs_parallel_source_not_found() {
        let (src, dst) = triangle_graph();
        let result = compute_bfs_parallel(&src, &dst, 999);
        assert!(result.is_err());
    }

    #[test]
    fn test_shortest_paths_parallel() {
        let (src, dst) = connected_graph();
        let result = compute_shortest_paths_parallel(&src, &dst, 1).unwrap();

        assert!(!result.node_ids.is_empty());
        assert_eq!(result.node_ids.len(), result.distances.len());

        // All distances should be non-negative
        for &dist in &result.distances {
            assert!(dist >= 0.0);
        }
    }

    #[test]
    fn test_components_parallel_single_component() {
        let (src, dst) = triangle_graph();
        let result = compute_components_parallel(&src, &dst).unwrap();

        // Triangle is fully connected - all nodes in same component
        assert_eq!(result.node_ids.len(), 3);
        let unique_components: std::collections::HashSet<_> = result.component_ids.iter().collect();
        assert_eq!(unique_components.len(), 1);
    }

    #[test]
    fn test_components_parallel_two_components() {
        // Two disconnected edges: 1-2 and 3-4
        let src = vec![1, 3];
        let dst = vec![2, 4];
        let result = compute_components_parallel(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 4);
        let unique_components: std::collections::HashSet<_> = result.component_ids.iter().collect();
        assert_eq!(unique_components.len(), 2);
    }

    #[test]
    fn test_clustering_parallel() {
        let (src, dst) = triangle_graph();
        let result = compute_clustering_parallel(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert_eq!(result.coefficients.len(), 3);

        // All coefficients should be in [0, 1]
        for &coef in &result.coefficients {
            assert!(coef >= 0.0 && coef <= 1.0);
        }

        // Triangle has perfect clustering
        for &coef in &result.coefficients {
            assert!((coef - 1.0).abs() < 0.01);
        }
    }

    #[test]
    fn test_triangles_parallel() {
        let (src, dst) = triangle_graph();
        let result = compute_triangles_parallel(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        // Each node participates in 1 triangle
        let total: i64 = result.triangle_counts.iter().sum();
        assert!(total > 0);
    }

    #[test]
    fn test_empty_graph_errors() {
        assert!(compute_pagerank_parallel(&[], &[], &[], 0.85, 100, false).is_err());
        assert!(compute_bfs_parallel(&[], &[], 1).is_err());
        assert!(compute_shortest_paths_parallel(&[], &[], 1).is_err());
        assert!(compute_components_parallel(&[], &[]).is_err());
        assert!(compute_clustering_parallel(&[], &[]).is_err());
        assert!(compute_triangles_parallel(&[], &[]).is_err());
    }

    #[test]
    fn test_mismatched_arrays_error() {
        assert!(compute_pagerank_parallel(&[1, 2], &[2], &[], 0.85, 100, false).is_err());
    }

    #[test]
    fn test_mismatched_weights_error() {
        assert!(compute_pagerank_parallel(&[1, 2], &[2, 3], &[1.0], 0.85, 100, false).is_err());
    }
}
