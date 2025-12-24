//! Parallel algorithms FFI exports.
//!
//! Parallel BFS, shortest paths, components, clustering, triangles.
//! Note: Parallel PageRank is in centrality.rs.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute parallel BFS from a single source.
#[no_mangle]
pub extern "C" fn onager_compute_bfs_parallel(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    source: i64,
    out_order: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_bfs_parallel(src, dst, source) {
        Ok(result) => {
            let n = result.order.len();
            if !out_order.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_order, n) }
                    .copy_from_slice(&result.order);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute parallel shortest paths from a single source.
#[no_mangle]
pub extern "C" fn onager_compute_shortest_paths_parallel(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    source: i64,
    out_nodes: *mut i64,
    out_distances: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_shortest_paths_parallel(src, dst, source) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_distances.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_distances, n) }
                    .copy_from_slice(&result.distances);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute parallel connected components.
#[no_mangle]
pub extern "C" fn onager_compute_components_parallel(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_nodes: *mut i64,
    out_components: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_components_parallel(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_components.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_components, n) }
                    .copy_from_slice(&result.component_ids);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute parallel clustering coefficients.
#[no_mangle]
pub extern "C" fn onager_compute_clustering_parallel(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_nodes: *mut i64,
    out_coefficients: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_clustering_parallel(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_coefficients.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_coefficients, n) }
                    .copy_from_slice(&result.coefficients);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute parallel triangle count.
#[no_mangle]
pub extern "C" fn onager_compute_triangles_parallel(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_nodes: *mut i64,
    out_counts: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_triangles_parallel(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_counts.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_counts, n) }
                    .copy_from_slice(&result.triangle_counts);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
