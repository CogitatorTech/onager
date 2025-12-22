//! FFI bindings for Onager extension.
//!
//! This module provides C-compatible functions for the DuckDB C++ extension.
//! Functions are organized by category into submodules.

mod approximation;
mod centrality;
mod common;
mod community;
mod generators;
mod links;
mod metrics;
mod mst;
mod parallel;
mod personalized;
mod subgraphs;
mod traversal;

// Re-export all public FFI items
pub use approximation::*;
pub use centrality::*;
pub use common::*;
pub use community::*;
pub use generators::*;
pub use links::*;
pub use metrics::*;
pub use mst::*;
pub use parallel::*;
pub use personalized::*;
pub use subgraphs::*;
pub use traversal::*;
