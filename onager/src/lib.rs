//! Onager -- A Graph Analytics Extension for DuckDB
//!
//! This crate provides graph analytics capabilities within DuckDB,
//! powered by the graphina library.

pub mod error;
pub mod ffi;
pub mod graph;

pub use error::OnagerError;
