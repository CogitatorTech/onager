//! FFI bindings for Onager extension.
//!
//! This module provides C-compatible functions for the DuckDB C++ extension.

mod common;
mod algorithms;

// Re-export all public FFI items
pub use common::*;
pub use algorithms::*;
