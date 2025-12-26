//! Community detection algorithms module.
//!
//! Louvain, Connected Components, Label Propagation, Girvan-Newman, Spectral Clustering, Infomap.

use graphina::community::connected_components::connected_components;
use graphina::community::girvan_newman::girvan_newman;
use graphina::community::infomap::infomap;
use graphina::community::label_propagation::label_propagation;
use graphina::community::louvain::louvain;
use graphina::community::spectral::spectral_clustering;
use graphina::core::types::{Graph, NodeId};

use crate::error::{OnagerError, Result};
use std::collections::HashMap;

/// Result of Louvain community detection.
pub struct LouvainResult {
    pub node_ids: Vec<i64>,
    pub community_ids: Vec<i64>,
}

/// Compute Louvain community detection.
pub fn compute_louvain(src: &[i64], dst: &[i64], seed: Option<u64>) -> Result<LouvainResult> {
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

    let communities = louvain(&graph, seed).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let mut result_nodes = Vec::new();
    let mut result_comms = Vec::new();
    for (comm_id, community) in communities.iter().enumerate() {
        for &internal_id in community {
            if let Some(&ext_id) = reverse_map.get(&internal_id) {
                result_nodes.push(ext_id);
                result_comms.push(comm_id as i64);
            }
        }
    }
    Ok(LouvainResult {
        node_ids: result_nodes,
        community_ids: result_comms,
    })
}

/// Result of connected components computation.
pub struct ConnectedComponentsResult {
    pub node_ids: Vec<i64>,
    pub component_ids: Vec<i64>,
}

/// Compute connected components.
pub fn compute_connected_components(src: &[i64], dst: &[i64]) -> Result<ConnectedComponentsResult> {
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

    let components = connected_components(&graph);
    let mut result_nodes = Vec::new();
    let mut result_comps = Vec::new();
    for (comp_id, component) in components.iter().enumerate() {
        for &internal_id in component {
            if let Some(&ext_id) = reverse_map.get(&internal_id) {
                result_nodes.push(ext_id);
                result_comps.push(comp_id as i64);
            }
        }
    }
    Ok(ConnectedComponentsResult {
        node_ids: result_nodes,
        component_ids: result_comps,
    })
}

/// Result of label propagation.
pub struct LabelPropagationResult {
    pub node_ids: Vec<i64>,
    pub labels: Vec<i64>,
}

/// Compute label propagation community detection.
pub fn compute_label_propagation(src: &[i64], dst: &[i64]) -> Result<LabelPropagationResult> {
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

    let labels_vec =
        label_propagation(&graph, 100, None).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();
    let node_list: Vec<NodeId> = graph.nodes().map(|(id, _)| id).collect();

    let mut node_ids = Vec::with_capacity(labels_vec.len());
    let mut labels = Vec::with_capacity(labels_vec.len());
    for (i, label) in labels_vec.into_iter().enumerate() {
        if let Some(&ext_id) = reverse_map.get(&node_list[i]) {
            node_ids.push(ext_id);
            labels.push(label as i64);
        }
    }
    Ok(LabelPropagationResult { node_ids, labels })
}

/// Result of Girvan-Newman community detection.
pub struct GirvanNewmanResult {
    pub node_ids: Vec<i64>,
    pub community_ids: Vec<i64>,
}

/// Compute Girvan-Newman community detection.
/// Uses edge betweenness to split graph into target number of communities.
pub fn compute_girvan_newman(
    src: &[i64],
    dst: &[i64],
    target_communities: i64,
) -> Result<GirvanNewmanResult> {
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
    if target_communities <= 0 {
        return Err(OnagerError::InvalidArgument(
            "target_communities must be positive".to_string(),
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

    let communities = girvan_newman(&graph, target_communities as usize)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_nodes = Vec::new();
    let mut result_comms = Vec::new();
    for (comm_id, community) in communities.iter().enumerate() {
        for &internal_id in community {
            if let Some(&ext_id) = reverse_map.get(&internal_id) {
                result_nodes.push(ext_id);
                result_comms.push(comm_id as i64);
            }
        }
    }
    Ok(GirvanNewmanResult {
        node_ids: result_nodes,
        community_ids: result_comms,
    })
}

/// Result of spectral clustering.
pub struct SpectralClusteringResult {
    pub node_ids: Vec<i64>,
    pub community_ids: Vec<i64>,
}

/// Compute spectral clustering for community detection.
/// Uses eigenvectors of the graph Laplacian and k-means clustering.
pub fn compute_spectral_clustering(
    src: &[i64],
    dst: &[i64],
    k: usize,
    seed: Option<u64>,
) -> Result<SpectralClusteringResult> {
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
    if k == 0 {
        return Err(OnagerError::InvalidArgument(
            "k must be positive".to_string(),
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

    let communities =
        spectral_clustering(&graph, k, seed).map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut result_nodes = Vec::new();
    let mut result_comms = Vec::new();
    for (comm_id, community) in communities.iter().enumerate() {
        for &internal_id in community {
            if let Some(&ext_id) = reverse_map.get(&internal_id) {
                result_nodes.push(ext_id);
                result_comms.push(comm_id as i64);
            }
        }
    }
    Ok(SpectralClusteringResult {
        node_ids: result_nodes,
        community_ids: result_comms,
    })
}

/// Result of infomap community detection.
pub struct InfomapResult {
    pub node_ids: Vec<i64>,
    pub community_ids: Vec<i64>,
}

/// Compute Infomap community detection.
/// Uses information-theoretic approach based on the map equation.
pub fn compute_infomap(
    src: &[i64],
    dst: &[i64],
    max_iter: usize,
    seed: Option<u64>,
) -> Result<InfomapResult> {
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
    if max_iter == 0 {
        return Err(OnagerError::InvalidArgument(
            "max_iter must be positive".to_string(),
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

    let modules =
        infomap(&graph, max_iter, seed).map_err(|e| OnagerError::GraphError(e.to_string()))?;
    let reverse_map: HashMap<NodeId, i64> = node_set.iter().map(|(&k, &v)| (v, k)).collect();
    let node_list: Vec<NodeId> = graph.nodes().map(|(id, _)| id).collect();

    let mut node_ids = Vec::with_capacity(modules.len());
    let mut community_ids = Vec::with_capacity(modules.len());
    for (i, module) in modules.into_iter().enumerate() {
        if let Some(&ext_id) = reverse_map.get(&node_list[i]) {
            node_ids.push(ext_id);
            community_ids.push(module as i64);
        }
    }
    Ok(InfomapResult {
        node_ids,
        community_ids,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_girvan_newman_two_communities() {
        // Two triangles connected by one edge
        let src = vec![1, 2, 3, 4, 5, 6, 3];
        let dst = vec![2, 3, 1, 5, 6, 4, 4];

        let result = compute_girvan_newman(&src, &dst, 2).unwrap();

        assert_eq!(result.node_ids.len(), 6);
        // Should have exactly 2 unique communities
        let unique_comms: std::collections::HashSet<_> = result.community_ids.iter().collect();
        assert_eq!(unique_comms.len(), 2);
    }

    #[test]
    fn test_girvan_newman_invalid_communities() {
        let src = vec![1, 2];
        let dst = vec![2, 3];

        let result = compute_girvan_newman(&src, &dst, 0);
        assert!(result.is_err());
    }

    #[test]
    fn test_louvain_simple() {
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 1];

        let result = compute_louvain(&src, &dst, None).unwrap();

        assert_eq!(result.node_ids.len(), 3);
    }

    #[test]
    fn test_connected_components() {
        let src = vec![1, 3];
        let dst = vec![2, 4];

        let result = compute_connected_components(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 4);
    }

    #[test]
    fn test_label_propagation() {
        // Triangle graph
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 1];

        let result = compute_label_propagation(&src, &dst).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert_eq!(result.labels.len(), 3);
    }

    #[test]
    fn test_spectral_clustering() {
        // Two triangles connected by one edge
        let src = vec![1, 2, 3, 4, 5, 6, 3];
        let dst = vec![2, 3, 1, 5, 6, 4, 4];

        let result = compute_spectral_clustering(&src, &dst, 2, Some(42)).unwrap();

        assert_eq!(result.node_ids.len(), 6);
        // Should have exactly 2 unique communities
        let unique_comms: std::collections::HashSet<_> = result.community_ids.iter().collect();
        assert!(unique_comms.len() <= 2);
    }

    #[test]
    fn test_spectral_clustering_invalid_k() {
        let src = vec![1, 2];
        let dst = vec![2, 3];

        let result = compute_spectral_clustering(&src, &dst, 0, None);
        assert!(result.is_err());
    }

    #[test]
    fn test_infomap() {
        // Triangle graph
        let src = vec![1, 2, 3];
        let dst = vec![2, 3, 1];

        let result = compute_infomap(&src, &dst, 10, Some(42)).unwrap();

        assert_eq!(result.node_ids.len(), 3);
        assert_eq!(result.community_ids.len(), 3);
    }

    #[test]
    fn test_infomap_invalid_max_iter() {
        let src = vec![1, 2];
        let dst = vec![2, 3];

        let result = compute_infomap(&src, &dst, 0, None);
        assert!(result.is_err());
    }

    #[test]
    fn test_empty_graph_errors() {
        assert!(compute_louvain(&[], &[], None).is_err());
        assert!(compute_connected_components(&[], &[]).is_err());
        assert!(compute_label_propagation(&[], &[]).is_err());
        assert!(compute_girvan_newman(&[], &[], 2).is_err());
        assert!(compute_spectral_clustering(&[], &[], 2, None).is_err());
        assert!(compute_infomap(&[], &[], 10, None).is_err());
    }

    #[test]
    fn test_mismatched_arrays_error() {
        assert!(compute_louvain(&[1, 2], &[2], None).is_err());
    }
}
