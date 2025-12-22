//! Graph algorithms module.
//!
//! This module provides graph algorithm implementations organized into submodules.
//!
//! Note: We use contains_key + insert pattern intentionally here because we need
//! to call graph.add_node() to get the internal NodeId before inserting into the map.
#![allow(clippy::map_entry)]

pub mod approximation;
pub mod centrality;
pub mod community;
pub mod generators;
pub mod links;
pub mod metrics;
pub mod mst;
pub mod parallel;
pub mod personalized;
pub mod subgraphs;
pub mod traversal;

// Re-export all public items for backward compatibility
pub use approximation::*;
pub use centrality::*;
pub use community::*;
pub use generators::*;
pub use links::*;
pub use metrics::*;
pub use mst::*;
pub use parallel::*;
pub use personalized::*;
pub use subgraphs::*;
pub use traversal::*;

// Backward compatibility alias
pub use links::LinkPredictionResult as JaccardResult;
