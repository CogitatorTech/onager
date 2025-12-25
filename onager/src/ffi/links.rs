//! Link prediction FFI exports.
//!
//! Jaccard, Adamic-Adar, Preferential Attachment, Resource Allocation.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute Jaccard coefficient.
#[no_mangle]
pub extern "C" fn onager_compute_jaccard(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_node1: *mut i64,
    out_node2: *mut i64,
    out_coefficients: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_jaccard(src, dst) {
        Ok(result) => {
            let n = result.node1.len();
            if !out_node1.is_null() && !out_node2.is_null() && !out_coefficients.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node1, n) }
                    .copy_from_slice(&result.node1);
                unsafe { std::slice::from_raw_parts_mut(out_node2, n) }
                    .copy_from_slice(&result.node2);
                unsafe { std::slice::from_raw_parts_mut(out_coefficients, n) }
                    .copy_from_slice(&result.scores);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute Adamic-Adar index.
#[no_mangle]
pub extern "C" fn onager_compute_adamic_adar(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_node1: *mut i64,
    out_node2: *mut i64,
    out_scores: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_adamic_adar(src, dst) {
        Ok(result) => {
            let n = result.node1.len();
            if !out_node1.is_null() && !out_node2.is_null() && !out_scores.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node1, n) }
                    .copy_from_slice(&result.node1);
                unsafe { std::slice::from_raw_parts_mut(out_node2, n) }
                    .copy_from_slice(&result.node2);
                unsafe { std::slice::from_raw_parts_mut(out_scores, n) }
                    .copy_from_slice(&result.scores);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute preferential attachment.
#[no_mangle]
pub extern "C" fn onager_compute_preferential_attachment(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_node1: *mut i64,
    out_node2: *mut i64,
    out_scores: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_preferential_attachment(src, dst) {
        Ok(result) => {
            let n = result.node1.len();
            if !out_node1.is_null() && !out_node2.is_null() && !out_scores.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node1, n) }
                    .copy_from_slice(&result.node1);
                unsafe { std::slice::from_raw_parts_mut(out_node2, n) }
                    .copy_from_slice(&result.node2);
                unsafe { std::slice::from_raw_parts_mut(out_scores, n) }
                    .copy_from_slice(&result.scores);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute resource allocation index.
#[no_mangle]
pub extern "C" fn onager_compute_resource_allocation(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_node1: *mut i64,
    out_node2: *mut i64,
    out_scores: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_resource_allocation(src, dst) {
        Ok(result) => {
            let n = result.node1.len();
            if !out_node1.is_null() && !out_node2.is_null() && !out_scores.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node1, n) }
                    .copy_from_slice(&result.node1);
                unsafe { std::slice::from_raw_parts_mut(out_node2, n) }
                    .copy_from_slice(&result.node2);
                unsafe { std::slice::from_raw_parts_mut(out_scores, n) }
                    .copy_from_slice(&result.scores);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute common neighbors count.
#[no_mangle]
pub extern "C" fn onager_compute_common_neighbors(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_node1: *mut i64,
    out_node2: *mut i64,
    out_counts: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_common_neighbors(src, dst) {
        Ok(result) => {
            let n = result.node1.len();
            if !out_node1.is_null() && !out_node2.is_null() && !out_counts.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node1, n) }
                    .copy_from_slice(&result.node1);
                unsafe { std::slice::from_raw_parts_mut(out_node2, n) }
                    .copy_from_slice(&result.node2);
                unsafe { std::slice::from_raw_parts_mut(out_counts, n) }
                    .copy_from_slice(&result.counts);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
