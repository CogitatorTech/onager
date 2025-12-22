//! Graph generators module.
//!
//! Erdős-Rényi, Barabási-Albert, Watts-Strogatz.

use graphina::core::generators::{barabasi_albert_graph, erdos_renyi_graph, watts_strogatz_graph};
use graphina::core::types::{Graph, Undirected};

use crate::error::{OnagerError, Result};

/// Result of graph generation.
pub struct GeneratorResult {
    pub src: Vec<i64>,
    pub dst: Vec<i64>,
}

/// Generate an Erdős-Rényi random graph.
pub fn generate_erdos_renyi(n: usize, p: f64, seed: u64) -> Result<GeneratorResult> {
    if n == 0 {
        return Err(OnagerError::InvalidArgument("n must be > 0".to_string()));
    }
    if !(0.0..=1.0).contains(&p) {
        return Err(OnagerError::InvalidArgument(
            "p must be in [0, 1]".to_string(),
        ));
    }

    let graph: Graph<u32, f32> = erdos_renyi_graph::<Undirected>(n, p, seed)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut src = Vec::new();
    let mut dst = Vec::new();
    for (u, v, _) in graph.edges() {
        src.push(u.index() as i64);
        dst.push(v.index() as i64);
    }
    Ok(GeneratorResult { src, dst })
}

/// Generate a Barabási-Albert preferential attachment graph.
pub fn generate_barabasi_albert(n: usize, m: usize, seed: u64) -> Result<GeneratorResult> {
    if n == 0 || m == 0 {
        return Err(OnagerError::InvalidArgument(
            "n and m must be > 0".to_string(),
        ));
    }
    if m > n {
        return Err(OnagerError::InvalidArgument("m must be <= n".to_string()));
    }

    let graph: Graph<u32, f32> = barabasi_albert_graph::<Undirected>(n, m, seed)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut src = Vec::new();
    let mut dst = Vec::new();
    for (u, v, _) in graph.edges() {
        src.push(u.index() as i64);
        dst.push(v.index() as i64);
    }
    Ok(GeneratorResult { src, dst })
}

/// Generate a Watts-Strogatz small-world graph.
pub fn generate_watts_strogatz(
    n: usize,
    k: usize,
    beta: f64,
    seed: u64,
) -> Result<GeneratorResult> {
    if n == 0 {
        return Err(OnagerError::InvalidArgument("n must be > 0".to_string()));
    }
    if !k.is_multiple_of(2) || k >= n {
        return Err(OnagerError::InvalidArgument(
            "k must be even and < n".to_string(),
        ));
    }
    if !(0.0..=1.0).contains(&beta) {
        return Err(OnagerError::InvalidArgument(
            "beta must be in [0, 1]".to_string(),
        ));
    }

    let graph: Graph<u32, f32> = watts_strogatz_graph::<Undirected>(n, k, beta, seed)
        .map_err(|e| OnagerError::GraphError(e.to_string()))?;

    let mut src = Vec::new();
    let mut dst = Vec::new();
    for (u, v, _) in graph.edges() {
        src.push(u.index() as i64);
        dst.push(v.index() as i64);
    }
    Ok(GeneratorResult { src, dst })
}
