//! Graph metrics FFI exports.
//!
//! Diameter, Radius, Average Clustering, Average Path Length, Transitivity, Triangle Count, Assortativity.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute graph diameter.
#[no_mangle]
pub extern "C" fn onager_compute_diameter(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_diameter(src, dst) {
        Ok(d) => d,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute graph radius.
#[no_mangle]
pub extern "C" fn onager_compute_radius(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_radius(src, dst) {
        Ok(r) => r,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute average clustering coefficient.
#[no_mangle]
pub extern "C" fn onager_compute_avg_clustering(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
) -> f64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return f64::NAN;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_avg_clustering(src, dst) {
        Ok(v) => v,
        Err(e) => {
            set_last_error(&e.to_string());
            f64::NAN
        }
    }
}

/// Compute average path length.
#[no_mangle]
pub extern "C" fn onager_compute_avg_path_length(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
) -> f64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return f64::NAN;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_avg_path_length(src, dst) {
        Ok(v) => v,
        Err(e) => {
            set_last_error(&e.to_string());
            f64::NAN
        }
    }
}

/// Compute transitivity.
#[no_mangle]
pub extern "C" fn onager_compute_transitivity(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
) -> f64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return f64::NAN;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_transitivity(src, dst) {
        Ok(v) => v,
        Err(e) => {
            set_last_error(&e.to_string());
            f64::NAN
        }
    }
}

/// Compute triangle count for each node.
#[no_mangle]
pub extern "C" fn onager_compute_triangle_count(
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
    match algorithms::compute_triangle_count(src, dst) {
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

/// Compute assortativity coefficient.
#[no_mangle]
pub extern "C" fn onager_compute_assortativity(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
) -> f64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return f64::NAN;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_assortativity(src, dst) {
        Ok(v) => v,
        Err(e) => {
            set_last_error(&e.to_string());
            f64::NAN
        }
    }
}

/// Compute graph density.
/// For directed=true: density = edges / (nodes * (nodes - 1))
/// For directed=false: density = 2 * edges / (nodes * (nodes - 1))
#[no_mangle]
pub extern "C" fn onager_compute_graph_density(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    directed: bool,
) -> f64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return f64::NAN;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_graph_density(src, dst, directed) {
        Ok(v) => v,
        Err(e) => {
            set_last_error(&e.to_string());
            f64::NAN
        }
    }
}
