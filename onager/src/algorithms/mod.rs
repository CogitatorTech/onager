//! Graph algorithms module.
//!
//! This module provides graph algorithm implementations organized into submodules.

pub mod centrality;
pub mod community;
pub mod generators;
pub mod links;
pub mod metrics;
pub mod parallel;
pub mod traversal;

// Re-export all public items for backward compatibility
pub use centrality::*;
pub use community::*;
pub use generators::*;
pub use links::*;
pub use metrics::*;
pub use parallel::*;
pub use traversal::*;

// Backward compatibility alias
pub use links::LinkPredictionResult as JaccardResult;
