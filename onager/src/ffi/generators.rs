//! Graph generators FFI exports.
//!
//! Erdős-Rényi, Barabási-Albert, Watts-Strogatz.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Generate Erdős-Rényi random graph.
#[no_mangle]
pub extern "C" fn onager_generate_erdos_renyi(
    n: usize,
    p: f64,
    seed: u64,
    out_src: *mut i64,
    out_dst: *mut i64,
) -> i64 {
    clear_last_error();
    match algorithms::generate_erdos_renyi(n, p, seed) {
        Ok(result) => {
            let edge_count = result.src.len();
            if !out_src.is_null() && !out_dst.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, edge_count) }
                    .copy_from_slice(&result.src);
                unsafe { std::slice::from_raw_parts_mut(out_dst, edge_count) }
                    .copy_from_slice(&result.dst);
            }
            edge_count as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Generate Barabási-Albert graph.
#[no_mangle]
pub extern "C" fn onager_generate_barabasi_albert(
    n: usize,
    m: usize,
    seed: u64,
    out_src: *mut i64,
    out_dst: *mut i64,
) -> i64 {
    clear_last_error();
    match algorithms::generate_barabasi_albert(n, m, seed) {
        Ok(result) => {
            let edge_count = result.src.len();
            if !out_src.is_null() && !out_dst.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, edge_count) }
                    .copy_from_slice(&result.src);
                unsafe { std::slice::from_raw_parts_mut(out_dst, edge_count) }
                    .copy_from_slice(&result.dst);
            }
            edge_count as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Generate Watts-Strogatz graph.
#[no_mangle]
pub extern "C" fn onager_generate_watts_strogatz(
    n: usize,
    k: usize,
    beta: f64,
    seed: u64,
    out_src: *mut i64,
    out_dst: *mut i64,
) -> i64 {
    clear_last_error();
    match algorithms::generate_watts_strogatz(n, k, beta, seed) {
        Ok(result) => {
            let edge_count = result.src.len();
            if !out_src.is_null() && !out_dst.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, edge_count) }
                    .copy_from_slice(&result.src);
                unsafe { std::slice::from_raw_parts_mut(out_dst, edge_count) }
                    .copy_from_slice(&result.dst);
            }
            edge_count as i64
        }
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
