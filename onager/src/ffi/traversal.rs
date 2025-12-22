//! Traversal and path FFI exports.
//!
//! Dijkstra, BFS, DFS, Bellman-Ford, Floyd-Warshall, Shortest Distance.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute Dijkstra shortest paths.
#[no_mangle]
pub extern "C" fn onager_compute_dijkstra(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    source_node: i64,
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
    match algorithms::compute_dijkstra(src, dst, source_node) {
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

/// Compute BFS traversal.
#[no_mangle]
pub extern "C" fn onager_compute_bfs(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    source_node: i64,
    out_order: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_bfs(src, dst, source_node) {
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

/// Compute DFS traversal.
#[no_mangle]
pub extern "C" fn onager_compute_dfs(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    source_node: i64,
    out_order: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_dfs(src, dst, source_node) {
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

/// Compute Bellman-Ford shortest paths on weighted edge arrays.
#[no_mangle]
pub extern "C" fn onager_compute_bellman_ford(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    weight_ptr: *const f64,
    edge_count: usize,
    source: i64,
    out_nodes: *mut i64,
    out_distances: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() || weight_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let weights = unsafe { std::slice::from_raw_parts(weight_ptr, edge_count) };
    match algorithms::compute_bellman_ford(src, dst, weights, source) {
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

/// Compute Floyd-Warshall all-pairs shortest paths.
#[no_mangle]
pub extern "C" fn onager_compute_floyd_warshall(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    weight_ptr: *const f64,
    edge_count: usize,
    out_src: *mut i64,
    out_dst: *mut i64,
    out_distances: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() || weight_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let weights = unsafe { std::slice::from_raw_parts(weight_ptr, edge_count) };
    match algorithms::compute_floyd_warshall(src, dst, weights) {
        Ok(result) => {
            let n = result.src_nodes.len();
            if !out_src.is_null() && !out_dst.is_null() && !out_distances.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, n) }
                    .copy_from_slice(&result.src_nodes);
                unsafe { std::slice::from_raw_parts_mut(out_dst, n) }
                    .copy_from_slice(&result.dst_nodes);
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

/// Compute shortest distance between two nodes (scalar).
#[no_mangle]
pub extern "C" fn onager_compute_shortest_distance(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    source_node: i64,
    target_node: i64,
) -> f64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return f64::NAN;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_shortest_distance(src, dst, source_node, target_node) {
        Ok(d) => d,
        Err(e) => {
            set_last_error(&e.to_string());
            f64::NAN
        }
    }
}
