//! Parallel algorithms module.
//!
//! Parallel PageRank, BFS, shortest paths, connected components, clustering, triangles.

use graphina::core::types::{Directed, Graph, GraphConstructor, NodeId, Undirected};
use graphina::parallel::{
    bfs_parallel, clustering_coefficients_parallel, connected_components_parallel,
    pagerank_parallel, shortest_paths_parallel, triangles_parallel,
};

use crate::algorithms::builder::{build_graph, check_edge_arrays};
use crate::algorithms::centrality::PageRankResult;
use crate::algorithms::community::ConnectedComponentsResult;
use crate::algorithms::metrics::TriangleResult;
use crate::algorithms::traversal::BfsResult;
use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Compute PageRank using parallel algorithm.
///
/// Graphina's `pagerank_parallel` ignores edge weights, so this function
/// rejects a non-empty `weights` slice instead of silently returning
/// unweighted ranks. Use [`super::centrality::compute_pagerank`] for
/// weighted PageRank.
pub fn compute_pagerank_parallel(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    damping: f64,
    iterations: usize,
    tolerance: f64,
    directed: bool,
) -> Result<PageRankResult> {
    check_edge_arrays(src, dst)?;
    if !weights.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Parallel PageRank does not support edge weights; use onager_ctr_pagerank instead"
                .to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if directed {
        pagerank_parallel_impl::<Directed>(src, dst, damping, iterations, tolerance)
    } else {
        pagerank_parallel_impl::<Undirected>(src, dst, damping, iterations, tolerance)
    }
}

fn pagerank_parallel_impl<Ty: GraphConstructor<i64, f64> + Sync>(
    src: &[i64],
    dst: &[i64],
    damping: f64,
    iterations: usize,
    tolerance: f64,
) -> Result<PageRankResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let ranks = pagerank_parallel(&g.graph, damping, iterations, tolerance, None);
    let mut node_ids = Vec::with_capacity(ranks.len());
    let mut rank_values = Vec::with_capacity(ranks.len());
    for (node_id, rank) in ranks {
        if let Some(&ext_id) = g.reverse.get(&node_id) {
            node_ids.push(ext_id);
            rank_values.push(rank);
        }
    }
    Ok(PageRankResult {
        node_ids,
        ranks: rank_values,
    })
}

/// Result of multi-source parallel BFS, flattened to one row per visited node.
pub struct MultiBfsResult {
    pub sources: Vec<i64>,
    pub nodes: Vec<i64>,
}

/// Compute parallel BFS traversals from multiple source nodes in one batch.
pub fn compute_bfs_parallel_multi(
    src: &[i64],
    dst: &[i64],
    sources: &[i64],
    directed: bool,
) -> Result<MultiBfsResult> {
    check_edge_arrays(src, dst)?;
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if sources.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "At least one source node is required".to_string(),
        ));
    }
    if directed {
        bfs_parallel_multi_impl::<Directed>(src, dst, sources)
    } else {
        bfs_parallel_multi_impl::<Undirected>(src, dst, sources)
    }
}

fn bfs_parallel_multi_impl<Ty: GraphConstructor<i64, f64> + Sync>(
    src: &[i64],
    dst: &[i64],
    sources: &[i64],
) -> Result<MultiBfsResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let source_ids = sources
        .iter()
        .map(|s| {
            g.node_ids
                .get(s)
                .copied()
                .ok_or(OnagerError::NodeNotFound(*s))
        })
        .collect::<Result<Vec<NodeId>>>()?;

    let results = bfs_parallel(&g.graph, &source_ids);

    let mut out_sources = Vec::new();
    let mut out_nodes = Vec::new();
    for (i, visit_order) in results.into_iter().enumerate() {
        for node_id in visit_order {
            if let Some(&ext_id) = g.reverse.get(&node_id) {
                out_sources.push(sources[i]);
                out_nodes.push(ext_id);
            }
        }
    }
    Ok(MultiBfsResult {
        sources: out_sources,
        nodes: out_nodes,
    })
}

/// Compute parallel BFS traversal from a single source.
pub fn compute_bfs_parallel(
    src: &[i64],
    dst: &[i64],
    source: i64,
    directed: bool,
) -> Result<BfsResult> {
    let multi = compute_bfs_parallel_multi(src, dst, &[source], directed)?;
    Ok(BfsResult {
        node_ids: multi.nodes.clone(),
        order: multi.nodes,
    })
}

/// Result of parallel shortest paths.
pub struct ShortestPathsParallelResult {
    pub node_ids: Vec<i64>,
    pub distances: Vec<f64>,
}

/// Result of multi-source parallel shortest paths, flattened to one row per reachable node.
pub struct MultiShortestPathsResult {
    pub sources: Vec<i64>,
    pub nodes: Vec<i64>,
    pub distances: Vec<f64>,
}

/// Compute parallel shortest paths from multiple source nodes in one batch.
pub fn compute_shortest_paths_parallel_multi(
    src: &[i64],
    dst: &[i64],
    sources: &[i64],
    directed: bool,
) -> Result<MultiShortestPathsResult> {
    check_edge_arrays(src, dst)?;
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if sources.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "At least one source node is required".to_string(),
        ));
    }
    if directed {
        shortest_paths_parallel_multi_impl::<Directed>(src, dst, sources)
    } else {
        shortest_paths_parallel_multi_impl::<Undirected>(src, dst, sources)
    }
}

fn shortest_paths_parallel_multi_impl<Ty: GraphConstructor<i64, f64> + Sync>(
    src: &[i64],
    dst: &[i64],
    sources: &[i64],
) -> Result<MultiShortestPathsResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;
    let source_ids = sources
        .iter()
        .map(|s| {
            g.node_ids
                .get(s)
                .copied()
                .ok_or(OnagerError::NodeNotFound(*s))
        })
        .collect::<Result<Vec<NodeId>>>()?;

    // shortest_paths_parallel returns one HashMap<NodeId, usize> per source
    let results = shortest_paths_parallel(&g.graph, &source_ids);

    let mut out_sources = Vec::new();
    let mut out_nodes = Vec::new();
    let mut out_distances = Vec::new();
    for (i, distances_map) in results.into_iter().enumerate() {
        for (node_id, dist) in distances_map {
            if let Some(&ext_id) = g.reverse.get(&node_id) {
                out_sources.push(sources[i]);
                out_nodes.push(ext_id);
                out_distances.push(dist as f64);
            }
        }
    }
    Ok(MultiShortestPathsResult {
        sources: out_sources,
        nodes: out_nodes,
        distances: out_distances,
    })
}

/// Compute parallel shortest paths from a single source.
pub fn compute_shortest_paths_parallel(
    src: &[i64],
    dst: &[i64],
    source: i64,
    directed: bool,
) -> Result<ShortestPathsParallelResult> {
    let multi = compute_shortest_paths_parallel_multi(src, dst, &[source], directed)?;
    Ok(ShortestPathsParallelResult {
        node_ids: multi.nodes,
        distances: multi.distances,
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
        let src_id = *node_set.get(&src[i]).ok_or_else(|| {
            OnagerError::InvalidArgument(format!("Source node {} not found in graph", src[i]))
        })?;
        let dst_id = *node_set.get(&dst[i]).ok_or_else(|| {
            OnagerError::InvalidArgument(format!("Destination node {} not found in graph", dst[i]))
        })?;
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
        let src_id = *node_set.get(&src[i]).ok_or_else(|| {
            OnagerError::InvalidArgument(format!("Source node {} not found in graph", src[i]))
        })?;
        let dst_id = *node_set.get(&dst[i]).ok_or_else(|| {
            OnagerError::InvalidArgument(format!("Destination node {} not found in graph", dst[i]))
        })?;
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
        let src_id = *node_set.get(&src[i]).ok_or_else(|| {
            OnagerError::InvalidArgument(format!("Source node {} not found in graph", src[i]))
        })?;
        let dst_id = *node_set.get(&dst[i]).ok_or_else(|| {
            OnagerError::InvalidArgument(format!("Destination node {} not found in graph", dst[i]))
        })?;
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
        let result = compute_pagerank_parallel(&src, &dst, &[], 0.85, 100, 1e-6, false).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert_eq!(result.ranks.len(), 3);

        // PageRank should sum to 1
        let sum: f64 = result.ranks.iter().sum();
        assert!((sum - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_pagerank_parallel_directed() {
        let (src, dst) = triangle_graph();
        let result = compute_pagerank_parallel(&src, &dst, &[], 0.85, 100, 1e-6, true).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert!(!result.ranks.is_empty());
    }

    #[test]
    fn test_pagerank_parallel_with_weights_errors() {
        let (src, dst) = triangle_graph();
        let weights = vec![1.0, 2.0, 1.5];
        let result = compute_pagerank_parallel(&src, &dst, &weights, 0.85, 100, 1e-6, false);

        assert!(result.is_err());
    }

    #[test]
    fn test_bfs_parallel() {
        let (src, dst) = connected_graph();
        let result = compute_bfs_parallel(&src, &dst, 1, false).unwrap();

        // Should visit all 4 nodes starting from 1
        assert!(!result.order.is_empty());
        assert!(result.order.len() <= 4);
    }

    #[test]
    fn test_bfs_parallel_source_not_found() {
        let (src, dst) = triangle_graph();
        let result = compute_bfs_parallel(&src, &dst, 999, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_shortest_paths_parallel() {
        let (src, dst) = connected_graph();
        let result = compute_shortest_paths_parallel(&src, &dst, 1, false).unwrap();

        assert!(!result.node_ids.is_empty());
        assert_eq!(result.node_ids.len(), result.distances.len());

        // All distances should be non-negative
        for &dist in &result.distances {
            assert!(dist >= 0.0);
        }
    }

    #[test]
    fn test_bfs_parallel_multi() {
        let (src, dst) = connected_graph();
        let result = compute_bfs_parallel_multi(&src, &dst, &[1, 2], false).unwrap();

        assert_eq!(result.sources.len(), result.nodes.len());
        assert!(result.sources.contains(&1));
        assert!(result.sources.contains(&2));

        // Empty sources and unknown sources must error
        assert!(compute_bfs_parallel_multi(&src, &dst, &[], false).is_err());
        assert!(compute_bfs_parallel_multi(&src, &dst, &[1, 999], false).is_err());
    }

    #[test]
    fn test_bfs_parallel_multi_directed() {
        // Directed path 1 -> 2 -> 3: from source 3 only node 3 is reachable,
        // while the undirected traversal reaches all three nodes.
        let src = vec![1, 2];
        let dst = vec![2, 3];
        let directed = compute_bfs_parallel_multi(&src, &dst, &[3], true).unwrap();
        let undirected = compute_bfs_parallel_multi(&src, &dst, &[3], false).unwrap();
        assert_eq!(directed.nodes.len(), 1);
        assert_eq!(undirected.nodes.len(), 3);
    }

    #[test]
    fn test_shortest_paths_parallel_multi_directed() {
        // Directed path 1 -> 2 -> 3: from source 3 only node 3 has a distance.
        let src = vec![1, 2];
        let dst = vec![2, 3];
        let directed = compute_shortest_paths_parallel_multi(&src, &dst, &[3], true).unwrap();
        let undirected = compute_shortest_paths_parallel_multi(&src, &dst, &[3], false).unwrap();
        assert_eq!(directed.nodes.len(), 1);
        assert_eq!(undirected.nodes.len(), 3);
    }

    #[test]
    fn test_shortest_paths_parallel_multi() {
        let (src, dst) = connected_graph();
        let result = compute_shortest_paths_parallel_multi(&src, &dst, &[1, 2], false).unwrap();

        assert_eq!(result.sources.len(), result.nodes.len());
        assert_eq!(result.nodes.len(), result.distances.len());
        assert!(result.sources.contains(&1));
        assert!(result.sources.contains(&2));

        // Batch results must match the single-source results per source
        let single = compute_shortest_paths_parallel(&src, &dst, 1, false).unwrap();
        let batch_from_1 = result
            .sources
            .iter()
            .zip(&result.nodes)
            .filter(|(&s, _)| s == 1)
            .count();
        assert_eq!(batch_from_1, single.node_ids.len());
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
            assert!((0.0..=1.0).contains(&coef));
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
        assert!(compute_pagerank_parallel(&[], &[], &[], 0.85, 100, 1e-6, false).is_err());
        assert!(compute_bfs_parallel(&[], &[], 1, false).is_err());
        assert!(compute_shortest_paths_parallel(&[], &[], 1, false).is_err());
        assert!(compute_components_parallel(&[], &[]).is_err());
        assert!(compute_clustering_parallel(&[], &[]).is_err());
        assert!(compute_triangles_parallel(&[], &[]).is_err());
    }

    #[test]
    fn test_mismatched_arrays_error() {
        assert!(compute_pagerank_parallel(&[1, 2], &[2], &[], 0.85, 100, 1e-6, false).is_err());
    }

    #[test]
    fn test_mismatched_weights_error() {
        assert!(
            compute_pagerank_parallel(&[1, 2], &[2, 3], &[1.0], 0.85, 100, 1e-6, false).is_err()
        );
    }

    #[test]
    fn test_weights_rejected() {
        // Graphina's parallel PageRank ignores weights, so passing them is an error.
        let result =
            compute_pagerank_parallel(&[1, 2], &[2, 3], &[1.0, 2.0], 0.85, 100, 1e-6, false);
        assert!(result.is_err());
    }
}
