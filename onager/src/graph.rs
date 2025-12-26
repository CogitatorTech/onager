//! Graph storage and operations module.
//!
//! This module provides a thread-safe graph registry that stores named graphs
//! and wraps the graphina library for graph operations.

use std::collections::HashMap;
use std::sync::Arc;

use graphina::core::types::{Digraph, Graph, NodeId};
use once_cell::sync::Lazy;
use parking_lot::RwLock;

use crate::error::{OnagerError, Result};

/// Wrapper for an undirected graph with external ID mapping.
pub struct UndirectedGraphWrapper {
    graph: Graph<i64, f64>,
    /// Maps external node IDs (provided by user) to internal graphina NodeIds
    node_mapping: HashMap<i64, NodeId>,
}

/// Wrapper for a directed graph with external ID mapping.
pub struct DirectedGraphWrapper {
    graph: Digraph<i64, f64>,
    /// Maps external node IDs (provided by user) to internal graphina NodeIds
    node_mapping: HashMap<i64, NodeId>,
}

/// A graph that can be either directed or undirected.
pub enum GraphType {
    Directed(DirectedGraphWrapper),
    Undirected(UndirectedGraphWrapper),
}

impl GraphType {
    /// Creates a new graph with the specified direction.
    pub fn new(directed: bool) -> Self {
        if directed {
            GraphType::Directed(DirectedGraphWrapper {
                graph: Digraph::new(),
                node_mapping: HashMap::new(),
            })
        } else {
            GraphType::Undirected(UndirectedGraphWrapper {
                graph: Graph::new(),
                node_mapping: HashMap::new(),
            })
        }
    }

    /// Returns true if the graph is directed.
    pub fn is_directed(&self) -> bool {
        matches!(self, GraphType::Directed(_))
    }

    /// Returns the number of nodes in the graph.
    pub fn node_count(&self) -> usize {
        match self {
            GraphType::Directed(w) => w.graph.node_count(),
            GraphType::Undirected(w) => w.graph.node_count(),
        }
    }

    /// Returns the number of edges in the graph.
    pub fn edge_count(&self) -> usize {
        match self {
            GraphType::Directed(w) => w.graph.edge_count(),
            GraphType::Undirected(w) => w.graph.edge_count(),
        }
    }

    /// Adds a node with the given external ID to the graph.
    /// The node_id is used both as the key in our mapping and stored as the node's value.
    pub fn add_node(&mut self, node_id: i64) -> Result<()> {
        match self {
            GraphType::Directed(w) => {
                if w.node_mapping.contains_key(&node_id) {
                    return Err(OnagerError::InvalidArgument(format!(
                        "Node {} already exists",
                        node_id
                    )));
                }
                let internal_id = w.graph.add_node(node_id);
                w.node_mapping.insert(node_id, internal_id);
                Ok(())
            }
            GraphType::Undirected(w) => {
                if w.node_mapping.contains_key(&node_id) {
                    return Err(OnagerError::InvalidArgument(format!(
                        "Node {} already exists",
                        node_id
                    )));
                }
                let internal_id = w.graph.add_node(node_id);
                w.node_mapping.insert(node_id, internal_id);
                Ok(())
            }
        }
    }

    /// Adds an edge between two nodes with the given weight.
    pub fn add_edge(&mut self, src: i64, dst: i64, weight: f64) -> Result<()> {
        match self {
            GraphType::Directed(w) => {
                let src_id = w
                    .node_mapping
                    .get(&src)
                    .ok_or(OnagerError::NodeNotFound(src))?;
                let dst_id = w
                    .node_mapping
                    .get(&dst)
                    .ok_or(OnagerError::NodeNotFound(dst))?;
                w.graph.add_edge(*src_id, *dst_id, weight);
                Ok(())
            }
            GraphType::Undirected(w) => {
                let src_id = w
                    .node_mapping
                    .get(&src)
                    .ok_or(OnagerError::NodeNotFound(src))?;
                let dst_id = w
                    .node_mapping
                    .get(&dst)
                    .ok_or(OnagerError::NodeNotFound(dst))?;
                w.graph.add_edge(*src_id, *dst_id, weight);
                Ok(())
            }
        }
    }
}

/// Global registry of named graphs.
static GRAPH_REGISTRY: Lazy<Arc<RwLock<HashMap<String, GraphType>>>> =
    Lazy::new(|| Arc::new(RwLock::new(HashMap::new())));

/// Creates a new graph with the given name.
pub fn create_graph(name: &str, directed: bool) -> Result<()> {
    let mut registry = GRAPH_REGISTRY.write();
    if registry.contains_key(name) {
        return Err(OnagerError::GraphAlreadyExists(name.to_string()));
    }
    registry.insert(name.to_string(), GraphType::new(directed));
    Ok(())
}

/// Drops a graph with the given name.
pub fn drop_graph(name: &str) -> Result<()> {
    let mut registry = GRAPH_REGISTRY.write();
    if registry.remove(name).is_none() {
        return Err(OnagerError::GraphNotFound(name.to_string()));
    }
    Ok(())
}

/// Returns a list of all graph names.
pub fn list_graphs() -> Vec<String> {
    let registry = GRAPH_REGISTRY.read();
    registry.keys().cloned().collect()
}

/// Adds a node to the specified graph.
/// The node_id is used as the node identifier.
pub fn add_node(graph_name: &str, node_id: i64) -> Result<()> {
    let mut registry = GRAPH_REGISTRY.write();
    let graph = registry
        .get_mut(graph_name)
        .ok_or_else(|| OnagerError::GraphNotFound(graph_name.to_string()))?;
    graph.add_node(node_id)
}

/// Adds an edge to the specified graph.
pub fn add_edge(graph_name: &str, src: i64, dst: i64, weight: f64) -> Result<()> {
    let mut registry = GRAPH_REGISTRY.write();
    let graph = registry
        .get_mut(graph_name)
        .ok_or_else(|| OnagerError::GraphNotFound(graph_name.to_string()))?;
    graph.add_edge(src, dst, weight)
}

/// Returns the number of nodes in the graph.
pub fn node_count(graph_name: &str) -> Result<usize> {
    let registry = GRAPH_REGISTRY.read();
    let graph = registry
        .get(graph_name)
        .ok_or_else(|| OnagerError::GraphNotFound(graph_name.to_string()))?;
    Ok(graph.node_count())
}

/// Returns the number of edges in the graph.
pub fn edge_count(graph_name: &str) -> Result<usize> {
    let registry = GRAPH_REGISTRY.read();
    let graph = registry
        .get(graph_name)
        .ok_or_else(|| OnagerError::GraphNotFound(graph_name.to_string()))?;
    Ok(graph.edge_count())
}

/// Returns the in-degree of a node in the named graph.
pub fn get_node_in_degree(graph_name: &str, external_node_id: i64) -> Result<usize> {
    let registry = GRAPH_REGISTRY.read();
    let graph = registry
        .get(graph_name)
        .ok_or_else(|| OnagerError::GraphNotFound(graph_name.to_string()))?;

    match graph {
        GraphType::Directed(w) => {
            let node_id = w
                .node_mapping
                .get(&external_node_id)
                .ok_or(OnagerError::NodeNotFound(external_node_id))?;
            w.graph
                .in_degree(*node_id)
                .ok_or(OnagerError::NodeNotFound(external_node_id))
        }
        GraphType::Undirected(w) => {
            let node_id = w
                .node_mapping
                .get(&external_node_id)
                .ok_or(OnagerError::NodeNotFound(external_node_id))?;
            w.graph
                .degree(*node_id)
                .ok_or(OnagerError::NodeNotFound(external_node_id))
        }
    }
}

/// Returns the out-degree of a node in the named graph.
pub fn get_node_out_degree(graph_name: &str, external_node_id: i64) -> Result<usize> {
    let registry = GRAPH_REGISTRY.read();
    let graph = registry
        .get(graph_name)
        .ok_or_else(|| OnagerError::GraphNotFound(graph_name.to_string()))?;

    match graph {
        GraphType::Directed(w) => {
            let node_id = w
                .node_mapping
                .get(&external_node_id)
                .ok_or(OnagerError::NodeNotFound(external_node_id))?;
            w.graph
                .out_degree(*node_id)
                .ok_or(OnagerError::NodeNotFound(external_node_id))
        }
        GraphType::Undirected(w) => {
            let node_id = w
                .node_mapping
                .get(&external_node_id)
                .ok_or(OnagerError::NodeNotFound(external_node_id))?;
            w.graph
                .degree(*node_id)
                .ok_or(OnagerError::NodeNotFound(external_node_id))
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_create_and_drop_graph() {
        let name = "test_graph_1";
        assert!(create_graph(name, false).is_ok());
        assert!(create_graph(name, false).is_err()); // Already exists
        assert!(drop_graph(name).is_ok());
        assert!(drop_graph(name).is_err()); // Not found
    }

    #[test]
    fn test_add_nodes_and_edges() {
        let name = "test_graph_2";
        create_graph(name, false).unwrap();

        add_node(name, 0).unwrap();
        add_node(name, 1).unwrap();
        add_node(name, 2).unwrap();

        assert_eq!(node_count(name).unwrap(), 3);

        add_edge(name, 0, 1, 1.0).unwrap();
        add_edge(name, 1, 2, 2.0).unwrap();

        assert_eq!(edge_count(name).unwrap(), 2);

        drop_graph(name).unwrap();
    }
}
