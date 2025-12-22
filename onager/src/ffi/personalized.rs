//! Personalized PageRank FFI exports.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute personalized PageRank.
#[no_mangle]
pub extern "C" fn onager_compute_personalized_pagerank(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    pers_nodes_ptr: *const i64,
    pers_weights_ptr: *const f64,
    pers_count: usize,
    damping: f64,
    max_iter: usize,
    tolerance: f64,
    out_nodes: *mut i64,
    out_scores: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() {
        set_last_error("Null pointer");
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };

    let personalization: Vec<(i64, f64)> =
        if pers_count > 0 && !pers_nodes_ptr.is_null() && !pers_weights_ptr.is_null() {
            let pers_nodes = unsafe { std::slice::from_raw_parts(pers_nodes_ptr, pers_count) };
            let pers_weights = unsafe { std::slice::from_raw_parts(pers_weights_ptr, pers_count) };
            pers_nodes
                .iter()
                .zip(pers_weights.iter())
                .map(|(&n, &w)| (n, w))
                .collect()
        } else {
            Vec::new()
        };

    match algorithms::compute_personalized_pagerank(
        src,
        dst,
        &personalization,
        damping,
        max_iter,
        tolerance,
    ) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_scores.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }
                    .copy_from_slice(&result.node_ids);
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
