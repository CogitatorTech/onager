//! Subgraph operations FFI exports.
//!
//! Ego graph, k-hop neighbors, induced subgraph.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute ego graph.
#[no_mangle]
pub extern "C" fn onager_compute_ego_graph(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    center: i64,
    radius: usize,
    out_src: *mut i64,
    out_dst: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_ego_graph(src, dst, center, radius) {
        Ok(result) => {
            let n = result.src.len();
            if !out_src.is_null() && !out_dst.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, n) }.copy_from_slice(&result.src);
                unsafe { std::slice::from_raw_parts_mut(out_dst, n) }.copy_from_slice(&result.dst);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute k-hop neighbors.
#[no_mangle]
pub extern "C" fn onager_compute_k_hop_neighbors(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    start: i64,
    k: usize,
    out_nodes: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_k_hop_neighbors(src, dst, start, k) {
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

/// Compute induced subgraph.
#[no_mangle]
pub extern "C" fn onager_compute_induced_subgraph(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    node_ids_ptr: *const i64,
    node_count: usize,
    out_src: *mut i64,
    out_dst: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() || node_ids_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let node_ids = unsafe { std::slice::from_raw_parts(node_ids_ptr, node_count) };
    match algorithms::compute_induced_subgraph(src, dst, node_ids) {
        Ok(result) => {
            let n = result.src.len();
            if !out_src.is_null() && !out_dst.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, n) }.copy_from_slice(&result.src);
                unsafe { std::slice::from_raw_parts_mut(out_dst, n) }.copy_from_slice(&result.dst);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
