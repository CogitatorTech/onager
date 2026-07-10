//! Shared edge-list graph construction for algorithm wrappers.
//!
//! Algorithms that accept a `directed` flag dispatch to a generic
//! implementation parameterized over graphina's `Directed` or `Undirected`
//! marker, and this module builds the graph either way from SQL edge arrays.

use graphina::core::types::{BaseGraph, GraphConstructor, NodeId};
use std::collections::HashMap;

use crate::error::{OnagerError, Result};

/// A graph built from edge arrays plus the external-to-internal id maps.
pub(crate) struct EdgeListGraph<W, Ty: GraphConstructor<i64, W>> {
    pub graph: BaseGraph<i64, W, Ty>,
    pub node_ids: HashMap<i64, NodeId>,
    pub reverse: HashMap<NodeId, i64>,
}

/// Validate that `src` and `dst` have the same length.
pub(crate) fn check_edge_arrays(src: &[i64], dst: &[i64]) -> Result<()> {
    if src.len() != dst.len() {
        return Err(OnagerError::InvalidArgument(
            "src and dst arrays must have same length".to_string(),
        ));
    }
    Ok(())
}

/// Validate that `weights` is empty or matches the edge count.
pub(crate) fn check_weights(weights: &[f64], edge_count: usize) -> Result<()> {
    if !weights.is_empty() && weights.len() != edge_count {
        return Err(OnagerError::InvalidArgument(
            "weights must be empty or same length as edges".to_string(),
        ));
    }
    Ok(())
}

/// Build a directed or undirected graph from edge arrays.
///
/// `weight_at` supplies the edge weight for each input row, letting callers
/// use unit weights, raw `f64` weights, or `OrderedFloat` weights.
pub(crate) fn build_graph<W, Ty, F>(
    src: &[i64],
    dst: &[i64],
    mut weight_at: F,
) -> Result<EdgeListGraph<W, Ty>>
where
    Ty: GraphConstructor<i64, W>,
    F: FnMut(usize) -> W,
{
    check_edge_arrays(src, dst)?;

    let mut graph: BaseGraph<i64, W, Ty> = BaseGraph::new();
    let mut node_ids: HashMap<i64, NodeId> = HashMap::new();
    let mut reverse: HashMap<NodeId, i64> = HashMap::new();
    for &node in src.iter().chain(dst.iter()) {
        if !node_ids.contains_key(&node) {
            let id = graph.add_node(node);
            node_ids.insert(node, id);
            reverse.insert(id, node);
        }
    }
    for i in 0..src.len() {
        let src_id = *node_ids.get(&src[i]).ok_or_else(|| {
            OnagerError::InvalidArgument(format!("Source node {} not found in graph", src[i]))
        })?;
        let dst_id = *node_ids.get(&dst[i]).ok_or_else(|| {
            OnagerError::InvalidArgument(format!("Destination node {} not found in graph", dst[i]))
        })?;
        graph.add_edge(src_id, dst_id, weight_at(i));
    }
    Ok(EdgeListGraph {
        graph,
        node_ids,
        reverse,
    })
}
