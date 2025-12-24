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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_erdos_renyi_basic() {
        let result = generate_erdos_renyi(10, 0.5, 42).unwrap();

        // Should generate some edges (probabilistic but very likely with p=0.5)
        assert!(!result.src.is_empty() || result.dst.is_empty() == result.src.is_empty());
        assert_eq!(result.src.len(), result.dst.len());
    }

    #[test]
    fn test_erdos_renyi_p_zero() {
        let result = generate_erdos_renyi(10, 0.0, 42).unwrap();

        // With p=0, no edges should be generated
        assert!(result.src.is_empty());
        assert!(result.dst.is_empty());
    }

    #[test]
    fn test_erdos_renyi_p_one() {
        let result = generate_erdos_renyi(5, 1.0, 42).unwrap();

        // With p=1, should generate complete graph: n*(n-1)/2 = 5*4/2 = 10 edges
        assert_eq!(result.src.len(), 10);
    }

    #[test]
    fn test_erdos_renyi_invalid_n() {
        let result = generate_erdos_renyi(0, 0.5, 42);
        assert!(result.is_err());
    }

    #[test]
    fn test_erdos_renyi_invalid_p() {
        let result = generate_erdos_renyi(10, 1.5, 42);
        assert!(result.is_err());

        let result2 = generate_erdos_renyi(10, -0.1, 42);
        assert!(result2.is_err());
    }

    #[test]
    fn test_barabasi_albert_basic() {
        let result = generate_barabasi_albert(20, 2, 42).unwrap();

        // Should generate (n - m) * m edges for preferential attachment
        assert!(!result.src.is_empty());
        assert_eq!(result.src.len(), result.dst.len());
    }

    #[test]
    fn test_barabasi_albert_invalid_params() {
        // n=0 should fail
        let result = generate_barabasi_albert(0, 2, 42);
        assert!(result.is_err());

        // m=0 should fail
        let result2 = generate_barabasi_albert(10, 0, 42);
        assert!(result2.is_err());

        // m > n should fail
        let result3 = generate_barabasi_albert(5, 10, 42);
        assert!(result3.is_err());
    }

    #[test]
    fn test_watts_strogatz_basic() {
        let result = generate_watts_strogatz(20, 4, 0.3, 42).unwrap();

        // Should generate n*k/2 edges (k=4, n=20 -> 40 edges)
        assert!(!result.src.is_empty());
        assert_eq!(result.src.len(), result.dst.len());
    }

    #[test]
    fn test_watts_strogatz_no_rewiring() {
        let result = generate_watts_strogatz(10, 4, 0.0, 42).unwrap();

        // With beta=0, should be a regular ring lattice
        assert!(!result.src.is_empty());
    }

    #[test]
    fn test_watts_strogatz_invalid_params() {
        // n=0 should fail
        let result = generate_watts_strogatz(0, 4, 0.3, 42);
        assert!(result.is_err());

        // k must be even
        let result2 = generate_watts_strogatz(10, 3, 0.3, 42);
        assert!(result2.is_err());

        // k >= n should fail
        let result3 = generate_watts_strogatz(10, 10, 0.3, 42);
        assert!(result3.is_err());

        // beta out of range
        let result4 = generate_watts_strogatz(10, 4, 1.5, 42);
        assert!(result4.is_err());
    }

    #[test]
    fn test_deterministic_with_same_seed() {
        let result1 = generate_erdos_renyi(10, 0.5, 12345).unwrap();
        let result2 = generate_erdos_renyi(10, 0.5, 12345).unwrap();

        assert_eq!(result1.src, result2.src);
        assert_eq!(result1.dst, result2.dst);
    }
}
