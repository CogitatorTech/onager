//! Approximation algorithms FFI exports.
//!
//! Maximum Clique, Independent Set, Vertex Cover.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute Maximum Clique Approximation.
#[no_mangle]
pub extern "C" fn onager_compute_max_clique(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_nodes: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_max_clique(src, dst) {
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

/// Compute Maximum Independent Set Approximation.
#[no_mangle]
pub extern "C" fn onager_compute_independent_set(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_nodes: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_independent_set(src, dst) {
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

/// Compute Minimum Vertex Cover Approximation.
#[no_mangle]
pub extern "C" fn onager_compute_vertex_cover(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_nodes: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_vertex_cover(src, dst) {
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

/// Compute TSP Approximation.
/// Returns tour length and writes tour nodes to out_tour and cost to out_cost.
#[no_mangle]
pub extern "C" fn onager_compute_tsp(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    weight_ptr: *const f64,
    edge_count: usize,
    out_tour: *mut i64,
    out_cost: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() || weight_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let weights = unsafe { std::slice::from_raw_parts(weight_ptr, edge_count) };
    match algorithms::compute_tsp(src, dst, weights) {
        Ok(result) => {
            let n = result.tour.len();
            if !out_tour.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_tour, n) }
                    .copy_from_slice(&result.tour);
            }
            if !out_cost.is_null() {
                unsafe { *out_cost = result.cost };
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
