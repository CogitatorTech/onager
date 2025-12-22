//! MST (Minimum Spanning Tree) FFI exports.
//!
//! Prim's and Kruskal's algorithms.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute Prim's MST on weighted edge arrays.
#[no_mangle]
pub extern "C" fn onager_compute_prim_mst(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    weight_ptr: *const f64,
    edge_count: usize,
    out_src: *mut i64,
    out_dst: *mut i64,
    out_weight: *mut f64,
    out_total: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() || weight_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let weights = unsafe { std::slice::from_raw_parts(weight_ptr, edge_count) };
    match algorithms::compute_prim_mst(src, dst, weights) {
        Ok(result) => {
            let n = result.src_nodes.len();
            if !out_src.is_null() && !out_dst.is_null() && !out_weight.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, n) }
                    .copy_from_slice(&result.src_nodes);
                unsafe { std::slice::from_raw_parts_mut(out_dst, n) }
                    .copy_from_slice(&result.dst_nodes);
                unsafe { std::slice::from_raw_parts_mut(out_weight, n) }
                    .copy_from_slice(&result.weights);
            }
            if !out_total.is_null() {
                unsafe {
                    *out_total = result.total_weight;
                }
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute Kruskal's MST on weighted edge arrays.
#[no_mangle]
pub extern "C" fn onager_compute_kruskal_mst(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    weight_ptr: *const f64,
    edge_count: usize,
    out_src: *mut i64,
    out_dst: *mut i64,
    out_weight: *mut f64,
    out_total: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() || weight_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let weights = unsafe { std::slice::from_raw_parts(weight_ptr, edge_count) };
    match algorithms::compute_kruskal_mst(src, dst, weights) {
        Ok(result) => {
            let n = result.src_nodes.len();
            if !out_src.is_null() && !out_dst.is_null() && !out_weight.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, n) }
                    .copy_from_slice(&result.src_nodes);
                unsafe { std::slice::from_raw_parts_mut(out_dst, n) }
                    .copy_from_slice(&result.dst_nodes);
                unsafe { std::slice::from_raw_parts_mut(out_weight, n) }
                    .copy_from_slice(&result.weights);
            }
            if !out_total.is_null() {
                unsafe {
                    *out_total = result.total_weight;
                }
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
