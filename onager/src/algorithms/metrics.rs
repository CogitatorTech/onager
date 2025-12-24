//! Graph metrics module.
//!
//! Diameter, Radius, Average Clustering, Average Path Length, Transitivity, Triangle Count, Assortativity.

use graphina::core::types::{Graph, NodeId};
use graphina::metrics::{
    assortativity, average_clustering_coefficient, average_path_length, diameter, radius,
    transitivity,
};
use graphina::parallel::triangles_parallel;
use ordered_float::OrderedFloat;

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Compute graph diameter (longest shortest path).
pub fn compute_diameter(src: &[i64], dst: &[i64]) -> Result<i64> {
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
    Ok(diameter(&graph).map(|d| d as i64).unwrap_or(-1))
}

/// Compute graph radius.
pub fn compute_radius(src: &[i64], dst: &[i64]) -> Result<i64> {
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
    Ok(radius(&graph).map(|v| v as i64).unwrap_or(-1))
}

/// Compute average clustering coefficient.
pub fn compute_avg_clustering(src: &[i64], dst: &[i64]) -> Result<f64> {
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
    Ok(average_clustering_coefficient(&graph))
}

/// Compute average path length.
pub fn compute_avg_path_length(src: &[i64], dst: &[i64]) -> Result<f64> {
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
    Ok(average_path_length(&graph).unwrap_or(f64::NAN))
}

/// Compute transitivity (global clustering coefficient).
pub fn compute_transitivity(src: &[i64], dst: &[i64]) -> Result<f64> {
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
    Ok(transitivity(&graph))
}

/// Result of triangle counting.
pub struct TriangleResult {
    pub node_ids: Vec<i64>,
    pub triangle_counts: Vec<i64>,
}

/// Compute triangle count for each node.
pub fn compute_triangle_count(src: &[i64], dst: &[i64]) -> Result<TriangleResult> {
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

    let mut result_nodes = Vec::with_capacity(triangles.len());
    let mut result_counts = Vec::with_capacity(triangles.len());
    for (node_id, count) in triangles {
        if let Some(&ext_id) = reverse_map.get(&node_id) {
            result_nodes.push(ext_id);
            result_counts.push(count as i64);
        }
    }
    Ok(TriangleResult {
        node_ids: result_nodes,
        triangle_counts: result_counts,
    })
}

/// Compute assortativity coefficient.
/// Measures the tendency of nodes to connect to others with similar degree.
/// Returns a value between -1 (disassortative) and 1 (assortative).
pub fn compute_assortativity(src: &[i64], dst: &[i64]) -> Result<f64> {
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
    Ok(assortativity(&graph))
}

/// Compute graph density.
///
/// For undirected graphs: density = 2 * edges / (nodes * (nodes - 1))
/// For directed graphs: density = edges / (nodes * (nodes - 1))
///
/// Returns a value between 0.0 (sparse) and 1.0 (complete graph).
pub fn compute_graph_density(src: &[i64], dst: &[i64], directed: bool) -> Result<f64> {
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

    // Count unique nodes
    let mut node_set: std::collections::HashSet<i64> = std::collections::HashSet::new();
    for &node in src.iter().chain(dst.iter()) {
        node_set.insert(node);
    }
    let n = node_set.len() as f64;

    if n < 2.0 {
        return Ok(0.0); // Single node graph has no edges possible
    }

    let edge_count = src.len() as f64;
    let max_edges = n * (n - 1.0);

    if directed {
        Ok(edge_count / max_edges)
    } else {
        // For undirected, each edge is counted once in src/dst
        Ok((2.0 * edge_count) / max_edges)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn triangle_graph() -> (Vec<i64>, Vec<i64>) {
        // Triangle: 1-2-3-1
        (vec![1, 2, 3], vec![2, 3, 1])
    }

    fn path_graph() -> (Vec<i64>, Vec<i64>) {
        // Path: 1-2-3-4
        (vec![1, 2, 3], vec![2, 3, 4])
    }

    #[test]
    fn test_diameter_triangle() {
        let (src, dst) = triangle_graph();
        let result = compute_diameter(&src, &dst).unwrap();

        // In a triangle, diameter is 1 (direct connection between all pairs)
        assert_eq!(result, 1);
    }

    #[test]
    fn test_diameter_path() {
        let (src, dst) = path_graph();
        let result = compute_diameter(&src, &dst).unwrap();

        // In path 1-2-3-4, diameter is 3 (from 1 to 4)
        assert_eq!(result, 3);
    }

    #[test]
    fn test_radius_triangle() {
        let (src, dst) = triangle_graph();
        let result = compute_radius(&src, &dst).unwrap();

        // In a triangle, radius is 1
        assert_eq!(result, 1);
    }

    #[test]
    fn test_avg_clustering_triangle() {
        let (src, dst) = triangle_graph();
        let result = compute_avg_clustering(&src, &dst).unwrap();

        // A triangle has perfect clustering (1.0)
        assert!((result - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_avg_clustering_path() {
        let (src, dst) = path_graph();
        let result = compute_avg_clustering(&src, &dst).unwrap();

        // A path has no triangles, so clustering is 0
        assert!(result < 0.01);
    }

    #[test]
    fn test_avg_path_length_triangle() {
        let (src, dst) = triangle_graph();
        let result = compute_avg_path_length(&src, &dst).unwrap();

        // In a triangle, average path length is 1 (all nodes directly connected)
        assert!((result - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_transitivity_triangle() {
        let (src, dst) = triangle_graph();
        let result = compute_transitivity(&src, &dst).unwrap();

        // A triangle has perfect transitivity (1.0)
        assert!((result - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_triangle_count() {
        let (src, dst) = triangle_graph();
        let result = compute_triangle_count(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        // Each node in a triangle participates in 1 triangle
        let total_triangles: i64 = result.triangle_counts.iter().sum();
        assert!(total_triangles > 0);
    }

    #[test]
    fn test_assortativity() {
        let (src, dst) = triangle_graph();
        let result = compute_assortativity(&src, &dst).unwrap();

        // Assortativity should be in [-1, 1]
        assert!(result >= -1.0 && result <= 1.0);
    }

    #[test]
    fn test_empty_graph_errors() {
        assert!(compute_diameter(&[], &[]).is_err());
        assert!(compute_radius(&[], &[]).is_err());
        assert!(compute_avg_clustering(&[], &[]).is_err());
        assert!(compute_transitivity(&[], &[]).is_err());
        assert!(compute_triangle_count(&[], &[]).is_err());
    }

    #[test]
    fn test_mismatched_arrays_error() {
        assert!(compute_diameter(&[1, 2], &[2]).is_err());
    }

    #[test]
    fn test_density_complete_undirected() {
        // Complete triangle: 3 nodes, 3 edges = density 1.0
        let (src, dst) = triangle_graph();
        let result = compute_graph_density(&src, &dst, false).unwrap();
        assert!((result - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_density_complete_directed() {
        // Triangle with directed edges: 3 nodes, 3 edges
        // Max directed edges = 3 * 2 = 6, so density = 3/6 = 0.5
        let (src, dst) = triangle_graph();
        let result = compute_graph_density(&src, &dst, true).unwrap();
        assert!((result - 0.5).abs() < 0.01);
    }

    #[test]
    fn test_density_sparse() {
        // Path: 4 nodes, 3 edges
        // Max undirected = 4*3/2 = 6, density = 3/6 = 0.5
        let (src, dst) = path_graph();
        let result = compute_graph_density(&src, &dst, false).unwrap();
        assert!((result - 0.5).abs() < 0.01);
    }

    #[test]
    fn test_density_empty_error() {
        assert!(compute_graph_density(&[], &[], false).is_err());
    }
}
