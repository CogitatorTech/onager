//! Algorithm FFI exports.
//!
//! All graph algorithm functions exposed to C.

use super::common::{clear_last_error, set_last_error};
use crate::algorithms;

/// Compute PageRank on edge arrays.
#[no_mangle]
pub extern "C" fn onager_compute_pagerank(
    src_ptr: *const i64,
    dst_ptr: *const i64,
    edge_count: usize,
    damping: f64,
    iterations: usize,
    directed: bool,
    out_nodes: *mut i64,
    out_ranks: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer for src or dst"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_pagerank(src, dst, &[], damping, iterations, directed) {
        Ok(result) => {
            let node_count = result.node_ids.len();
            if !out_nodes.is_null() && !out_ranks.is_null() {
                let out_n = unsafe { std::slice::from_raw_parts_mut(out_nodes, node_count) };
                let out_r = unsafe { std::slice::from_raw_parts_mut(out_ranks, node_count) };
                out_n.copy_from_slice(&result.node_ids);
                out_r.copy_from_slice(&result.ranks);
            }
            node_count as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute degree centrality on edge arrays.
#[no_mangle]
pub extern "C" fn onager_compute_degree(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize, directed: bool,
    out_nodes: *mut i64, out_in_degree: *mut f64, out_out_degree: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_degree(src, dst, directed) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_in_degree.is_null() && !out_out_degree.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_in_degree, n) }.copy_from_slice(&result.in_degrees);
                unsafe { std::slice::from_raw_parts_mut(out_out_degree, n) }.copy_from_slice(&result.out_degrees);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute betweenness centrality on edge arrays.
#[no_mangle]
pub extern "C" fn onager_compute_betweenness(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize, normalized: bool,
    out_nodes: *mut i64, out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_betweenness(src, dst, normalized) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }.copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute closeness centrality.
#[no_mangle]
pub extern "C" fn onager_compute_closeness(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize,
    out_nodes: *mut i64, out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_closeness(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }.copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute eigenvector centrality.
#[no_mangle]
pub extern "C" fn onager_compute_eigenvector(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize, max_iter: usize, tolerance: f64,
    out_nodes: *mut i64, out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_eigenvector(src, dst, max_iter, tolerance) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }.copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute Katz centrality.
#[no_mangle]
pub extern "C" fn onager_compute_katz(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize, alpha: f64, max_iter: usize, tolerance: f64,
    out_nodes: *mut i64, out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_katz(src, dst, alpha, max_iter, tolerance) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }.copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute harmonic centrality.
#[no_mangle]
pub extern "C" fn onager_compute_harmonic(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize,
    out_nodes: *mut i64, out_centralities: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_harmonic(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_centralities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_centralities, n) }.copy_from_slice(&result.centralities);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute Louvain community detection.
#[no_mangle]
pub extern "C" fn onager_compute_louvain(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize, seed: i64,
    out_nodes: *mut i64, out_communities: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let seed_opt = if seed < 0 { None } else { Some(seed as u64) };
    match algorithms::compute_louvain(src, dst, seed_opt) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_communities.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_communities, n) }.copy_from_slice(&result.community_ids);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute connected components.
#[no_mangle]
pub extern "C" fn onager_compute_connected_components(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize,
    out_nodes: *mut i64, out_components: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_connected_components(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_components.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_components, n) }.copy_from_slice(&result.component_ids);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute label propagation.
#[no_mangle]
pub extern "C" fn onager_compute_label_propagation(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize,
    out_node_ids: *mut i64, out_labels: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_label_propagation(src, dst) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_node_ids.is_null() && !out_labels.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node_ids, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_labels, n) }.copy_from_slice(&result.labels);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute Dijkstra shortest paths.
#[no_mangle]
pub extern "C" fn onager_compute_dijkstra(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize, source_node: i64,
    out_nodes: *mut i64, out_distances: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_dijkstra(src, dst, source_node) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_nodes.is_null() && !out_distances.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_nodes, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_distances, n) }.copy_from_slice(&result.distances);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute BFS traversal.
#[no_mangle]
pub extern "C" fn onager_compute_bfs(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize, source_node: i64, out_order: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_bfs(src, dst, source_node) {
        Ok(result) => {
            let n = result.order.len();
            if !out_order.is_null() { unsafe { std::slice::from_raw_parts_mut(out_order, n) }.copy_from_slice(&result.order); }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute DFS traversal.
#[no_mangle]
pub extern "C" fn onager_compute_dfs(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize, source_node: i64, out_order: *mut i64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_dfs(src, dst, source_node) {
        Ok(result) => {
            let n = result.order.len();
            if !out_order.is_null() { unsafe { std::slice::from_raw_parts_mut(out_order, n) }.copy_from_slice(&result.order); }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Generate Erdős-Rényi random graph.
#[no_mangle]
pub extern "C" fn onager_generate_erdos_renyi(n: usize, p: f64, seed: u64, out_src: *mut i64, out_dst: *mut i64) -> i64 {
    clear_last_error();
    match algorithms::generate_erdos_renyi(n, p, seed) {
        Ok(result) => {
            let edge_count = result.src.len();
            if !out_src.is_null() && !out_dst.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, edge_count) }.copy_from_slice(&result.src);
                unsafe { std::slice::from_raw_parts_mut(out_dst, edge_count) }.copy_from_slice(&result.dst);
            }
            edge_count as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Generate Barabási-Albert graph.
#[no_mangle]
pub extern "C" fn onager_generate_barabasi_albert(n: usize, m: usize, seed: u64, out_src: *mut i64, out_dst: *mut i64) -> i64 {
    clear_last_error();
    match algorithms::generate_barabasi_albert(n, m, seed) {
        Ok(result) => {
            let edge_count = result.src.len();
            if !out_src.is_null() && !out_dst.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, edge_count) }.copy_from_slice(&result.src);
                unsafe { std::slice::from_raw_parts_mut(out_dst, edge_count) }.copy_from_slice(&result.dst);
            }
            edge_count as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Generate Watts-Strogatz graph.
#[no_mangle]
pub extern "C" fn onager_generate_watts_strogatz(n: usize, k: usize, beta: f64, seed: u64, out_src: *mut i64, out_dst: *mut i64) -> i64 {
    clear_last_error();
    match algorithms::generate_watts_strogatz(n, k, beta, seed) {
        Ok(result) => {
            let edge_count = result.src.len();
            if !out_src.is_null() && !out_dst.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_src, edge_count) }.copy_from_slice(&result.src);
                unsafe { std::slice::from_raw_parts_mut(out_dst, edge_count) }.copy_from_slice(&result.dst);
            }
            edge_count as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute Jaccard coefficient.
#[no_mangle]
pub extern "C" fn onager_compute_jaccard(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize,
    out_node1: *mut i64, out_node2: *mut i64, out_coefficients: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_jaccard(src, dst) {
        Ok(result) => {
            let n = result.node1.len();
            if !out_node1.is_null() && !out_node2.is_null() && !out_coefficients.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node1, n) }.copy_from_slice(&result.node1);
                unsafe { std::slice::from_raw_parts_mut(out_node2, n) }.copy_from_slice(&result.node2);
                unsafe { std::slice::from_raw_parts_mut(out_coefficients, n) }.copy_from_slice(&result.scores);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute Adamic-Adar index.
#[no_mangle]
pub extern "C" fn onager_compute_adamic_adar(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize,
    out_node1: *mut i64, out_node2: *mut i64, out_scores: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_adamic_adar(src, dst) {
        Ok(result) => {
            let n = result.node1.len();
            if !out_node1.is_null() && !out_node2.is_null() && !out_scores.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node1, n) }.copy_from_slice(&result.node1);
                unsafe { std::slice::from_raw_parts_mut(out_node2, n) }.copy_from_slice(&result.node2);
                unsafe { std::slice::from_raw_parts_mut(out_scores, n) }.copy_from_slice(&result.scores);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute preferential attachment.
#[no_mangle]
pub extern "C" fn onager_compute_preferential_attachment(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize,
    out_node1: *mut i64, out_node2: *mut i64, out_scores: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_preferential_attachment(src, dst) {
        Ok(result) => {
            let n = result.node1.len();
            if !out_node1.is_null() && !out_node2.is_null() && !out_scores.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node1, n) }.copy_from_slice(&result.node1);
                unsafe { std::slice::from_raw_parts_mut(out_node2, n) }.copy_from_slice(&result.node2);
                unsafe { std::slice::from_raw_parts_mut(out_scores, n) }.copy_from_slice(&result.scores);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute resource allocation index.
#[no_mangle]
pub extern "C" fn onager_compute_resource_allocation(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize,
    out_node1: *mut i64, out_node2: *mut i64, out_scores: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_resource_allocation(src, dst) {
        Ok(result) => {
            let n = result.node1.len();
            if !out_node1.is_null() && !out_node2.is_null() && !out_scores.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node1, n) }.copy_from_slice(&result.node1);
                unsafe { std::slice::from_raw_parts_mut(out_node2, n) }.copy_from_slice(&result.node2);
                unsafe { std::slice::from_raw_parts_mut(out_scores, n) }.copy_from_slice(&result.scores);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute graph diameter.
#[no_mangle]
pub extern "C" fn onager_compute_diameter(src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_diameter(src, dst) {
        Ok(d) => d,
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute graph radius.
#[no_mangle]
pub extern "C" fn onager_compute_radius(src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_radius(src, dst) {
        Ok(r) => r,
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}

/// Compute average clustering coefficient.
#[no_mangle]
pub extern "C" fn onager_compute_avg_clustering(src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize) -> f64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return f64::NAN; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_avg_clustering(src, dst) {
        Ok(v) => v,
        Err(e) => { set_last_error(&e.to_string()); f64::NAN }
    }
}

/// Compute average path length.
#[no_mangle]
pub extern "C" fn onager_compute_avg_path_length(src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize) -> f64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return f64::NAN; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_avg_path_length(src, dst) {
        Ok(v) => v,
        Err(e) => { set_last_error(&e.to_string()); f64::NAN }
    }
}

/// Compute transitivity.
#[no_mangle]
pub extern "C" fn onager_compute_transitivity(src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize) -> f64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return f64::NAN; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    match algorithms::compute_transitivity(src, dst) {
        Ok(v) => v,
        Err(e) => { set_last_error(&e.to_string()); f64::NAN }
    }
}

/// Compute PageRank using parallel algorithm.
#[no_mangle]
pub extern "C" fn onager_compute_pagerank_parallel(
    src_ptr: *const i64, dst_ptr: *const i64, edge_count: usize,
    weights_ptr: *const f64, weights_count: usize,
    damping: f64, iterations: usize, directed: bool,
    out_node_ids: *mut i64, out_ranks: *mut f64,
) -> i64 {
    clear_last_error();
    if src_ptr.is_null() || dst_ptr.is_null() { set_last_error("Null pointer"); return -1; }
    let src = unsafe { std::slice::from_raw_parts(src_ptr, edge_count) };
    let dst = unsafe { std::slice::from_raw_parts(dst_ptr, edge_count) };
    let weights = if weights_ptr.is_null() || weights_count == 0 { &[] } else { unsafe { std::slice::from_raw_parts(weights_ptr, weights_count) } };
    match algorithms::compute_pagerank_parallel(src, dst, weights, damping, iterations, directed) {
        Ok(result) => {
            let n = result.node_ids.len();
            if !out_node_ids.is_null() && !out_ranks.is_null() {
                unsafe { std::slice::from_raw_parts_mut(out_node_ids, n) }.copy_from_slice(&result.node_ids);
                unsafe { std::slice::from_raw_parts_mut(out_ranks, n) }.copy_from_slice(&result.ranks);
            }
            n as i64
        }
        Err(e) => { set_last_error(&e.to_string()); -1 }
    }
}
