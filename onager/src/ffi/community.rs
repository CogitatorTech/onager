//! Community detection FFI exports.
//!
//! Louvain, Connected Components, Label Propagation, Girvan-Newman.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute Louvain community detection.
#[no_mangle]
pub extern "C" fn onager_compute_louvain(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    seed: i64,
    out_nodes: *mut i64,
    out_communities: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let seed_opt = if seed < 0 { None } else { Some(seed as u64) };
    match algorithms::compute_louvain(src, dst, seed_opt) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_communities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_communities, n) }
                    .copy_from_slice(&result.community_ids);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute connected components.
#[no_mangle]
pub extern "C" fn onager_compute_connected_components(
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
    match algorithms::compute_connected_components(src, dst) {
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

/// Compute label propagation.
#[no_mangle]
pub extern "C" fn onager_compute_label_propagation(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    out_node_ids: *mut i64,
    out_labels: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_label_propagation(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_node_ids.is_null() && !out_labels.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node_ids, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_labels, n) }
                    .copy_from_slice(&result.labels);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute Girvan-Newman community detection.
#[no_mangle]
pub extern "C" fn onager_compute_girvan_newman(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    target_communities: i64,
    out_nodes: *mut i64,
    out_communities: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_girvan_newman(src, dst, target_communities) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_communities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_communities, n) }
                    .copy_from_slice(&result.community_ids);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute spectral clustering.
#[no_mangle]
pub extern "C" fn onager_compute_spectral_clustering(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    k: usize,
    seed: i64,
    out_nodes: *mut i64,
    out_communities: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let seed_opt = if seed < 0 { None } else { Some(seed as u64) };
    match algorithms::compute_spectral_clustering(src, dst, k, seed_opt) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_communities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_communities, n) }
                    .copy_from_slice(&result.community_ids);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Compute infomap community detection.
#[no_mangle]
pub extern "C" fn onager_compute_infomap(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    max_iter: usize,
    seed: i64,
    out_nodes: *mut i64,
    out_communities: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let seed_opt = if seed < 0 { None } else { Some(seed as u64) };
    match algorithms::compute_infomap(src, dst, max_iter, seed_opt) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_communities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_communities, n) }
                    .copy_from_slice(&result.community_ids);
            }
            n as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
