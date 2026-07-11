//! Graph traversal and path algorithms module.
//!
//! Dijkstra, Bellman-Ford, BFS, DFS.

use graphina::core::paths::{bellman_ford, dijkstra, floyd_warshall};
use graphina::core::types::{Directed, GraphConstructor, Undirected};
use graphina::traversal::algorithms::{bfs, dfs};
use ordered_float::OrderedFloat;

use crate::algorithms::builder::{build_graph, check_edge_arrays, check_weights_no_nan};
use crate::error::{OnagerError, Result};

/// Result of Dijkstra shortest path computation.
pub struct DijkstraResult {
    pub node_ids: Vec<i64>,
    pub distances: Vec<f64>,
}

/// Validate an optional weights slice for Dijkstra-based algorithms.
///
/// Weights must be empty (unweighted) or match the edge count, and every
/// weight must be nonnegative.
fn check_dijkstra_weights(weights: &[f64], edge_count: usize) -> Result<()> {
    if !weights.is_empty() && weights.len() != edge_count {
        return Err(OnagerError::InvalidArgument(
            "weights must be empty or same length as edges".to_string(),
        ));
    }
    if weights.iter().any(|w| *w < 0.0 || w.is_nan()) {
        return Err(OnagerError::InvalidArgument(
            "Dijkstra requires nonnegative edge weights".to_string(),
        ));
    }
    Ok(())
}

/// Compute shortest distances from a source node.
///
/// When `weights` is empty, every edge gets weight `1.0`.
pub fn compute_dijkstra(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    source_node: i64,
    directed: bool,
) -> Result<DijkstraResult> {
    check_edge_arrays(src, dst)?;
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    check_dijkstra_weights(weights, src.len())?;
    if directed {
        dijkstra_impl::<Directed>(src, dst, weights, source_node)
    } else {
        dijkstra_impl::<Undirected>(src, dst, weights, source_node)
    }
}

fn dijkstra_impl<Ty: GraphConstructor<i64, OrderedFloat<f64>>>(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    source_node: i64,
) -> Result<DijkstraResult> {
    let g = build_graph::<OrderedFloat<f64>, Ty, _>(src, dst, |i| {
        OrderedFloat(if weights.is_empty() { 1.0 } else { weights[i] })
    })?;

    let source_id = g.node_ids.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;
    let distances =
        dijkstra(&g.graph, *source_id).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_nodes = Vec::with_capacity(g.node_ids.len());
    let mut result_dist = Vec::with_capacity(g.node_ids.len());
    for (ext_id, int_id) in &g.node_ids {
        result_nodes.push(*ext_id);
        let dist = distances.get(int_id).and_then(|d| *d);
        result_dist.push(dist.map(|d| d.into_inner()).unwrap_or(f64::INFINITY));
    }
    Ok(DijkstraResult {
        node_ids: result_nodes,
        distances: result_dist,
    })
}

/// Result of BFS traversal.
pub struct BfsResult {
    pub node_ids: Vec<i64>,
    pub order: Vec<i64>,
}

/// Compute BFS traversal from a source node.
pub fn compute_bfs(
    src: &[i64],
    dst: &[i64],
    source_node: i64,
    directed: bool,
) -> Result<BfsResult> {
    check_edge_arrays(src, dst)?;
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if directed {
        bfs_impl::<Directed>(src, dst, source_node)
    } else {
        bfs_impl::<Undirected>(src, dst, source_node)
    }
}

fn bfs_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
    source_node: i64,
) -> Result<BfsResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;

    let source_id = g.node_ids.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;
    let traversal = bfs(&g.graph, *source_id);

    let mut order = Vec::new();
    for internal_id in &traversal {
        if let Some(&ext_id) = g.reverse.get(internal_id) {
            order.push(ext_id);
        }
    }
    Ok(BfsResult {
        node_ids: order.clone(),
        order,
    })
}

/// Result of DFS traversal.
pub struct DfsResult {
    pub node_ids: Vec<i64>,
    pub order: Vec<i64>,
}

/// Compute DFS traversal from a source node.
pub fn compute_dfs(
    src: &[i64],
    dst: &[i64],
    source_node: i64,
    directed: bool,
) -> Result<DfsResult> {
    check_edge_arrays(src, dst)?;
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if directed {
        dfs_impl::<Directed>(src, dst, source_node)
    } else {
        dfs_impl::<Undirected>(src, dst, source_node)
    }
}

fn dfs_impl<Ty: GraphConstructor<i64, f64>>(
    src: &[i64],
    dst: &[i64],
    source_node: i64,
) -> Result<DfsResult> {
    let g = build_graph::<f64, Ty, _>(src, dst, |_| 1.0)?;

    let source_id = g.node_ids.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;
    let traversal = dfs(&g.graph, *source_id);

    let mut order = Vec::new();
    for internal_id in &traversal {
        if let Some(&ext_id) = g.reverse.get(internal_id) {
            order.push(ext_id);
        }
    }
    Ok(DfsResult {
        node_ids: order.clone(),
        order,
    })
}

/// Compute shortest distance between two specific nodes.
/// Returns f64::INFINITY if unreachable, or the distance if reachable.
pub fn compute_shortest_distance(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    source_node: i64,
    target_node: i64,
    directed: bool,
) -> Result<f64> {
    check_edge_arrays(src, dst)?;
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    check_dijkstra_weights(weights, src.len())?;
    if directed {
        shortest_distance_impl::<Directed>(src, dst, weights, source_node, target_node)
    } else {
        shortest_distance_impl::<Undirected>(src, dst, weights, source_node, target_node)
    }
}

fn shortest_distance_impl<Ty: GraphConstructor<i64, OrderedFloat<f64>>>(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    source_node: i64,
    target_node: i64,
) -> Result<f64> {
    let g = build_graph::<OrderedFloat<f64>, Ty, _>(src, dst, |i| {
        OrderedFloat(if weights.is_empty() { 1.0 } else { weights[i] })
    })?;

    let source_id = g.node_ids.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;
    let target_id = g.node_ids.get(&target_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Target node {} not found", target_node))
    })?;

    let distances =
        dijkstra(&g.graph, *source_id).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let dist = distances.get(target_id).and_then(|d| *d);
    Ok(dist.map(|d| d.into_inner()).unwrap_or(f64::INFINITY))
}

/// Result of Bellman-Ford shortest path computation.
pub struct BellmanFordResult {
    pub node_ids: Vec<i64>,
    pub distances: Vec<f64>,
}

/// Compute shortest distances from a source node using Bellman-Ford.
/// Works with negative weights but detects negative cycles.
pub fn compute_bellman_ford(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    source_node: i64,
    directed: bool,
) -> Result<BellmanFordResult> {
    if src.len() != dst.len() || src.len() != weights.len() {
        return Err(OnagerError::InvalidArgument(
            "src, dst, and weights arrays must have same length".to_string(),
        ));
    }
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    check_weights_no_nan(weights, src.len())?;
    if directed {
        bellman_ford_impl::<Directed>(src, dst, weights, source_node)
    } else {
        bellman_ford_impl::<Undirected>(src, dst, weights, source_node)
    }
}

fn bellman_ford_impl<Ty: GraphConstructor<i64, OrderedFloat<f64>>>(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    source_node: i64,
) -> Result<BellmanFordResult> {
    let g = build_graph::<OrderedFloat<f64>, Ty, _>(src, dst, |i| OrderedFloat(weights[i]))?;

    let source_id = g.node_ids.get(&source_node).ok_or_else(|| {
        OnagerError::InvalidArgument(format!("Source node {} not found", source_node))
    })?;

    let distances = bellman_ford(&g.graph, *source_id)
        .ok_or_else(|| OnagerError::GraphError("Negative cycle detected".to_string()))?;

    let mut result_nodes = Vec::with_capacity(g.node_ids.len());
    let mut result_dist = Vec::with_capacity(g.node_ids.len());
    for (ext_id, int_id) in &g.node_ids {
        result_nodes.push(*ext_id);
        let dist = distances.get(int_id).and_then(|d| *d);
        result_dist.push(dist.map(|d| d.into_inner()).unwrap_or(f64::INFINITY));
    }
    Ok(BellmanFordResult {
        node_ids: result_nodes,
        distances: result_dist,
    })
}

/// Result of Floyd-Warshall all-pairs shortest paths.
pub struct FloydWarshallResult {
    pub src_nodes: Vec<i64>,
    pub dst_nodes: Vec<i64>,
    pub distances: Vec<f64>,
}

/// Compute all-pairs shortest distances using Floyd-Warshall.
pub fn compute_floyd_warshall(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
    directed: bool,
) -> Result<FloydWarshallResult> {
    if src.len() != dst.len() || src.len() != weights.len() {
        return Err(OnagerError::InvalidArgument(
            "src, dst, and weights arrays must have same length".to_string(),
        ));
    }
    check_weights_no_nan(weights, src.len())?;
    if src.is_empty() {
        return Err(OnagerError::InvalidArgument(
            "Cannot compute on empty graph".to_string(),
        ));
    }
    if directed {
        floyd_warshall_impl::<Directed>(src, dst, weights)
    } else {
        floyd_warshall_impl::<Undirected>(src, dst, weights)
    }
}

fn floyd_warshall_impl<Ty: GraphConstructor<i64, OrderedFloat<f64>>>(
    src: &[i64],
    dst: &[i64],
    weights: &[f64],
) -> Result<FloydWarshallResult> {
    let g = build_graph::<OrderedFloat<f64>, Ty, _>(src, dst, |i| OrderedFloat(weights[i]))?;

    let distances = floyd_warshall(&g.graph)
        .ok_or_else(|| OnagerError::GraphError("Negative cycle detected".to_string()))?;

    let mut result_src = Vec::new();
    let mut result_dst = Vec::new();
    let mut result_dist = Vec::new();

    for (&from_id, inner) in &distances {
        if let Some(&from_ext) = g.reverse.get(&from_id) {
            for (&to_id, &dist_opt) in inner {
                if let Some(&to_ext) = g.reverse.get(&to_id) {
                    if from_ext != to_ext {
                        result_src.push(from_ext);
                        result_dst.push(to_ext);
                        result_dist.push(dist_opt.map(|d| d.into_inner()).unwrap_or(f64::INFINITY));
                    }
                }
            }
        }
    }

    Ok(FloydWarshallResult {
        src_nodes: result_src,
        dst_nodes: result_dst,
        distances: result_dist,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_bellman_ford_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];
        let weights = vec![1.0, 2.0, 3.0];

        let result = compute_bellman_ford(&src, &dst, &weights, 1, false).unwrap();

        assert_eq!(result.node_ids.len(), 4);
    }

    #[test]
    fn test_bellman_ford_source_not_found() {
        let src = vec![1, 2];
        let dst = vec![2, 3];
        let weights = vec![1.0, 2.0];

        let result = compute_bellman_ford(&src, &dst, &weights, 99, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_dijkstra_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];

        let result = compute_dijkstra(&src, &dst, &[], 1, false).unwrap();

        assert_eq!(result.node_ids.len(), 4);
    }

    #[test]
    fn test_dijkstra_weighted() {
        // Two routes from 1 to 3: direct with weight 10, or through 2 with total weight 3.
        let src = vec![1, 1, 2];
        let dst = vec![3, 2, 3];
        let weights = vec![10.0, 1.0, 2.0];

        let result = compute_dijkstra(&src, &dst, &weights, 1, false).unwrap();
        let dist_of = |node: i64| {
            let idx = result
                .node_ids
                .iter()
                .position(|&n| n == node)
                .unwrap_or_else(|| panic!("node {node} missing from result"));
            result.distances[idx]
        };
        assert!(
            (dist_of(3) - 3.0).abs() < 1e-9,
            "shortest path is via node 2"
        );
        assert!((dist_of(2) - 1.0).abs() < 1e-9);

        let direct = compute_shortest_distance(&src, &dst, &weights, 1, 3, false).unwrap();
        assert!((direct - 3.0).abs() < 1e-9);
    }

    #[test]
    fn test_dijkstra_directed_unreachable() {
        // Directed path 1 -> 2 -> 3: node 1 is unreachable from node 3.
        let src = vec![1, 2];
        let dst = vec![2, 3];

        let result = compute_dijkstra(&src, &dst, &[], 3, true).unwrap();
        let dist_of = |node: i64| {
            let idx = result
                .node_ids
                .iter()
                .position(|&n| n == node)
                .unwrap_or_else(|| panic!("node {node} missing from result"));
            result.distances[idx]
        };
        assert!(dist_of(1).is_infinite());

        // The same query on the undirected graph reaches node 1 in two hops.
        let undirected = compute_shortest_distance(&src, &dst, &[], 3, 1, false).unwrap();
        assert!((undirected - 2.0).abs() < 1e-9);
    }

    #[test]
    fn test_dijkstra_invalid_weights() {
        let src = vec![1, 2];
        let dst = vec![2, 3];
        assert!(compute_dijkstra(&src, &dst, &[1.0], 1, false).is_err());
        assert!(compute_dijkstra(&src, &dst, &[1.0, -2.0], 1, false).is_err());
        assert!(compute_shortest_distance(&src, &dst, &[1.0, -2.0], 1, 3, false).is_err());
    }

    #[test]
    fn test_bfs_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];

        let result = compute_bfs(&src, &dst, 1, false).unwrap();

        assert!(!result.node_ids.is_empty());
    }

    #[test]
    fn test_bfs_directed() {
        // Directed path 1 -> 2 -> 3: BFS from node 3 visits only node 3,
        // while the undirected traversal reaches all three nodes.
        let src = vec![1, 2];
        let dst = vec![2, 3];

        let directed = compute_bfs(&src, &dst, 3, true).unwrap();
        assert_eq!(directed.order, vec![3]);

        let undirected = compute_bfs(&src, &dst, 3, false).unwrap();
        assert_eq!(undirected.order.len(), 3);
    }

    #[test]
    fn test_dfs_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];

        let result = compute_dfs(&src, &dst, 1, false).unwrap();

        assert!(!result.node_ids.is_empty());
    }

    #[test]
    fn test_floyd_warshall_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];
        let weights = vec![1.0, 2.0, 3.0];

        let result = compute_floyd_warshall(&src, &dst, &weights, false).unwrap();

        // Should have all-pairs distances (excluding self-loops)
        assert!(!result.src_nodes.is_empty());
        assert_eq!(result.src_nodes.len(), result.dst_nodes.len());
        assert_eq!(result.src_nodes.len(), result.distances.len());
    }

    #[test]
    fn test_floyd_warshall_triangle() {
        // Triangle: 1-2-3 with weights
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 1];
        let weights = vec![1.0, 1.0, 1.0];

        let result = compute_floyd_warshall(&src, &dst, &weights, false).unwrap();

        // All nodes can reach all others
        assert!(!result.distances.is_empty());
        // All distances should be finite
        for &d in &result.distances {
            assert!(d.is_finite());
        }
    }

    #[test]
    fn test_shortest_distance() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 4];

        let dist = compute_shortest_distance(&src, &dst, &[], 1, 4, false).unwrap();

        // Path 1 -> 2 -> 3 -> 4 = 3 hops
        assert_eq!(dist, 3.0);
    }

    #[test]
    fn test_shortest_distance_unreachable() {
        // Two disconnected components
        let src = vec![1, 3];
        let dst = vec![2, 4];

        let dist = compute_shortest_distance(&src, &dst, &[], 1, 3, false).unwrap();

        // Node 3 is unreachable from node 1
        assert!(dist.is_infinite());
    }

    #[test]
    fn test_empty_graph_errors() {
        assert!(compute_dijkstra(&[], &[], &[], 1, false).is_err());
        assert!(compute_bfs(&[], &[], 1, false).is_err());
        assert!(compute_dfs(&[], &[], 1, false).is_err());
        assert!(compute_bellman_ford(&[], &[], &[], 1, false).is_err());
        assert!(compute_floyd_warshall(&[], &[], &[], false).is_err());
        assert!(compute_shortest_distance(&[], &[], &[], 1, 2, false).is_err());
    }

    #[test]
    fn test_mismatched_arrays_error() {
        assert!(compute_dijkstra(&[1, 2], &[2], &[], 1, false).is_err());
        assert!(compute_bellman_ford(&[1, 2], &[2, 3], &[1.0], 1, false).is_err());
        assert!(compute_floyd_warshall(&[1, 2], &[2, 3], &[1.0], false).is_err());
    }

    #[test]
    fn test_nan_weights_rejected() {
        assert!(compute_bellman_ford(&[1, 2], &[2, 3], &[f64::NAN, 1.0], 1, false).is_err());
        assert!(compute_floyd_warshall(&[1, 2], &[2, 3], &[f64::NAN, 1.0], false).is_err());
    }
}
