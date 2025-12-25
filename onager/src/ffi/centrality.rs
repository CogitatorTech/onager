//! Centrality FFI exports.
//!
//! PageRank, Degree, Betweenness, Closeness, Eigenvector, Katz, Harmonic.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute PageRank on edge arrays.
#[no_mangle]
pub extern "C" fn onager_compute_pagerank(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    damping: f64,
    iterations: usize,
    directed: bool,
    out_nodes: *mut i64,
    out_ranks: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer for src or dst");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_pagerank(src, dst, &[], damping, iterations, directed) {
        Ok(result) => {
            let node_count = result.node_ids.len();
            if !out_nodes.is_null() && !out_ranks.is_null() {
                let out_n = unsafe { std::slice::from_raw_parts_mut(out_nodes, node_count) };
                let out_r = unsafe { std::slice::from_raw_parts_mut(out_ranks, node_count) };
                out_n.copy_from_slice(&result.node_ids);
                out_r.copy_from_slice(&result.ranks);
            }
            node_count as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute PageRank using parallel algorithm.
#[no_mangle]
pub extern "C" fn onager_compute_pagerank_parallel(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    weights_ptr: *const f64,
    weights_count: usize,
    damping: f64,
    iterations: usize,
    directed: bool,
    out_node_ids: *mut i64,
    out_ranks: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let weights = if weights_ptr.is_null() || weights_count == 0 {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(weights_ptr, weights_count) }
    };
    match algorithms::compute_pagerank_parallel(src, dst, weights, damping, iterations, directed) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_node_ids.is_null() && !out_ranks.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node_ids, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_ranks, n) }
                    .copy_from_slice(&result.ranks);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute degree centrality on edge arrays.
#[no_mangle]
pub extern "C" fn onager_compute_degree(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    directed: bool,
    out_nodes: *mut i64,
    out_in_degree: *mut f64,
    out_out_degree: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_degree(src, dst, directed) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_in_degree.is_null() && !out_out_degree.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_in_degree, n) }
                    .copy_from_slice(&result.in_degrees);
                unsafe { std::slice::from_raw_parts_mut(out_out_degree, n) }
                    .copy_from_slice(&result.out_degrees);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute in-degree of a single node (scalar).
#[no_mangle]
pub extern "C" fn onager_compute_node_in_degree(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    node: i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_node_degree(src, dst, node) {
        Ok(r) => r.in_degree,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute out-degree of a single node (scalar).
#[no_mangle]
pub extern "C" fn onager_compute_node_out_degree(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    node: i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_node_degree(src, dst, node) {
        Ok(r) => r.out_degree,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute betweenness centrality on edge arrays.
#[no_mangle]
pub extern "C" fn onager_compute_betweenness(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    normalized: bool,
    out_nodes: *mut i64,
    out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_betweenness(src, dst, normalized) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }
                    .copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute closeness centrality.
#[no_mangle]
pub extern "C" fn onager_compute_closeness(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_nodes: *mut i64,
    out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_closeness(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }
                    .copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute eigenvector centrality.
#[no_mangle]
pub extern "C" fn onager_compute_eigenvector(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    max_iter: usize,
    tolerance: f64,
    out_nodes: *mut i64,
    out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_eigenvector(src, dst, max_iter, tolerance) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }
                    .copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute Katz centrality.
#[no_mangle]
pub extern "C" fn onager_compute_katz(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    alpha: f64,
    max_iter: usize,
    tolerance: f64,
    out_nodes: *mut i64,
    out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_katz(src, dst, alpha, max_iter, tolerance) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }
                    .copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute harmonic centrality.
#[no_mangle]
pub extern "C" fn onager_compute_harmonic(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_nodes: *mut i64,
    out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_harmonic(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }
                    .copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute VoteRank for influential spreaders.
#[no_mangle]
pub extern "C" fn onager_compute_voterank(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    num_seeds: usize,
    out_nodes: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_voterank(src, dst, num_seeds) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute Local Reaching Centrality.
#[no_mangle]
pub extern "C" fn onager_compute_local_reaching(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    distance: usize,
    out_nodes: *mut i64,
    out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_local_reaching(src, dst, distance) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }
                    .copy_from_slice(&result.centrality);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute Laplacian Centrality.
#[no_mangle]
pub extern "C" fn onager_compute_laplacian(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_nodes: *mut i64,
    out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_laplacian(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }
                    .copy_from_slice(&result.centrality);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
