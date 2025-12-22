/* Generated with cbindgen */
/* DO NOT EDIT */


#ifndef ONAGER_H
#define ONAGER_H

#pragma once

/* Generated with cbindgen:0.26.0 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
namespace onager {
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Returns the last error message, or null if no error is set.
 */
 const char *onager_last_error(void);

/**
 * Frees a string allocated by Onager.
 */
 void onager_free(char *ptr);

/**
 * Returns the version string, caller must free with onager_free.
 */
 char *onager_get_version(void);

/**
 * Creates a new graph with the given name.
 * Returns 0 on success, -1 on error.
 */
 int32_t onager_create_graph(const char *name, bool directed);

/**
 * Drops a graph with the given name.
 * Returns 0 on success, -1 on error.
 */
 int32_t onager_drop_graph(const char *name);

/**
 * Returns a JSON array of all graph names.
 * Caller must free with onager_free.
 */
 char *onager_list_graphs(void);

/**
 * Adds a node to the specified graph.
 * Returns 0 on success, -1 on error.
 */
 int32_t onager_add_node(const char *graph_name, int64_t node_id);

/**
 * Adds an edge to the specified graph.
 * Returns 0 on success, -1 on error.
 */
 int32_t onager_add_edge(const char *graph_name, int64_t src, int64_t dst, double weight);

/**
 * Returns the number of nodes in the graph.
 * Returns -1 on error.
 */
 int64_t onager_node_count(const char *graph_name);

/**
 * Returns the number of edges in the graph.
 * Returns -1 on error.
 */
 int64_t onager_edge_count(const char *graph_name);

/**
 * Compute PageRank on edge arrays.
 *
 * # Arguments
 * * `src_ptr` - Pointer to source node IDs array
 * * `dst_ptr` - Pointer to destination node IDs array
 * * `edge_count` - Number of edges
 * * `damping` - Damping factor (typically 0.85)
 * * `iterations` - Maximum iterations
 * * `directed` - Whether the graph is directed
 * * `out_nodes` - Output: pointer to node IDs array (must be pre-allocated)
 * * `out_ranks` - Output: pointer to ranks array (must be pre-allocated)
 *
 * # Returns
 * Number of nodes on success, -1 on error.
 */

int64_t onager_compute_pagerank(const int64_t *src_ptr,
                                const int64_t *dst_ptr,
                                uintptr_t edge_count,
                                double damping,
                                uintptr_t iterations,
                                bool directed,
                                int64_t *out_nodes,
                                double *out_ranks);

/**
 * Compute degree centrality on edge arrays.
 *
 * # Returns
 * Number of nodes on success, -1 on error.
 */

int64_t onager_compute_degree(const int64_t *src_ptr,
                              const int64_t *dst_ptr,
                              uintptr_t edge_count,
                              bool directed,
                              int64_t *out_nodes,
                              double *out_in_degree,
                              double *out_out_degree);

/**
 * Compute betweenness centrality on edge arrays.
 */

int64_t onager_compute_betweenness(const int64_t *src_ptr,
                                   const int64_t *dst_ptr,
                                   uintptr_t edge_count,
                                   bool normalized,
                                   int64_t *out_nodes,
                                   double *out_centralities);

/**
 * Compute closeness centrality on edge arrays.
 */

int64_t onager_compute_closeness(const int64_t *src_ptr,
                                 const int64_t *dst_ptr,
                                 uintptr_t edge_count,
                                 int64_t *out_nodes,
                                 double *out_centralities);

/**
 * Compute Louvain community detection on edge arrays.
 */

int64_t onager_compute_louvain(const int64_t *src_ptr,
                               const int64_t *dst_ptr,
                               uintptr_t edge_count,
                               int64_t seed,
                               int64_t *out_nodes,
                               int64_t *out_communities);

/**
 * Compute connected components on edge arrays.
 */

int64_t onager_compute_connected_components(const int64_t *src_ptr,
                                            const int64_t *dst_ptr,
                                            uintptr_t edge_count,
                                            int64_t *out_nodes,
                                            int64_t *out_components);

/**
 * Compute eigenvector centrality on edge arrays.
 */

int64_t onager_compute_eigenvector(const int64_t *src_ptr,
                                   const int64_t *dst_ptr,
                                   uintptr_t edge_count,
                                   uintptr_t max_iter,
                                   double tolerance,
                                   int64_t *out_nodes,
                                   double *out_centralities);

/**
 * Compute Katz centrality on edge arrays.
 */

int64_t onager_compute_katz(const int64_t *src_ptr,
                            const int64_t *dst_ptr,
                            uintptr_t edge_count,
                            double alpha,
                            uintptr_t max_iter,
                            double tolerance,
                            int64_t *out_nodes,
                            double *out_centralities);

/**
 * Compute harmonic centrality on edge arrays.
 */

int64_t onager_compute_harmonic(const int64_t *src_ptr,
                                const int64_t *dst_ptr,
                                uintptr_t edge_count,
                                int64_t *out_nodes,
                                double *out_centralities);

/**
 * Compute Dijkstra shortest paths on edge arrays.
 */

int64_t onager_compute_dijkstra(const int64_t *src_ptr,
                                const int64_t *dst_ptr,
                                uintptr_t edge_count,
                                int64_t source_node,
                                int64_t *out_nodes,
                                double *out_distances);

/**
 * Compute BFS traversal on edge arrays.
 */

int64_t onager_compute_bfs(const int64_t *src_ptr,
                           const int64_t *dst_ptr,
                           uintptr_t edge_count,
                           int64_t source_node,
                           int64_t *out_order);

/**
 * Compute DFS traversal on edge arrays.
 */

int64_t onager_compute_dfs(const int64_t *src_ptr,
                           const int64_t *dst_ptr,
                           uintptr_t edge_count,
                           int64_t source_node,
                           int64_t *out_order);

/**
 * Generate Erdős-Rényi random graph.
 */

int64_t onager_generate_erdos_renyi(uintptr_t n,
                                    double p,
                                    uint64_t seed,
                                    int64_t *out_src,
                                    int64_t *out_dst);

/**
 * Generate Barabási-Albert graph.
 */

int64_t onager_generate_barabasi_albert(uintptr_t n,
                                        uintptr_t m,
                                        uint64_t seed,
                                        int64_t *out_src,
                                        int64_t *out_dst);

/**
 * Generate Watts-Strogatz graph.
 */

int64_t onager_generate_watts_strogatz(uintptr_t n,
                                       uintptr_t k,
                                       double beta,
                                       uint64_t seed,
                                       int64_t *out_src,
                                       int64_t *out_dst);

/**
 * Compute Jaccard coefficient.
 */

int64_t onager_compute_jaccard(const int64_t *src_ptr,
                               const int64_t *dst_ptr,
                               uintptr_t edge_count,
                               int64_t *out_node1,
                               int64_t *out_node2,
                               double *out_coefficients);

/**
 * Compute graph diameter.
 */

int64_t onager_compute_diameter(const int64_t *src_ptr,
                                const int64_t *dst_ptr,
                                uintptr_t edge_count);

/**
 * Compute Adamic-Adar index.
 */

int64_t onager_compute_adamic_adar(const int64_t *src_ptr,
                                   const int64_t *dst_ptr,
                                   uintptr_t edge_count,
                                   int64_t *out_node1,
                                   int64_t *out_node2,
                                   double *out_scores);

/**
 * Compute preferential attachment.
 */

int64_t onager_compute_preferential_attachment(const int64_t *src_ptr,
                                               const int64_t *dst_ptr,
                                               uintptr_t edge_count,
                                               int64_t *out_node1,
                                               int64_t *out_node2,
                                               double *out_scores);

/**
 * Compute resource allocation index.
 */

int64_t onager_compute_resource_allocation(const int64_t *src_ptr,
                                           const int64_t *dst_ptr,
                                           uintptr_t edge_count,
                                           int64_t *out_node1,
                                           int64_t *out_node2,
                                           double *out_scores);

/**
 * Compute label propagation.
 */

int64_t onager_compute_label_propagation(const int64_t *src_ptr,
                                         const int64_t *dst_ptr,
                                         uintptr_t edge_count,
                                         int64_t *out_node_ids,
                                         int64_t *out_labels);

/**
 * Compute graph radius.
 */

int64_t onager_compute_radius(const int64_t *src_ptr,
                              const int64_t *dst_ptr,
                              uintptr_t edge_count);

/**
 * Compute average clustering coefficient.
 */

double onager_compute_avg_clustering(const int64_t *src_ptr,
                                     const int64_t *dst_ptr,
                                     uintptr_t edge_count);

/**
 * Compute average path length.
 */

double onager_compute_avg_path_length(const int64_t *src_ptr,
                                      const int64_t *dst_ptr,
                                      uintptr_t edge_count);

/**
 * Compute transitivity.
 */

double onager_compute_transitivity(const int64_t *src_ptr,
                                   const int64_t *dst_ptr,
                                   uintptr_t edge_count);

/**
 * Compute PageRank using parallel algorithm.
 */

int64_t onager_compute_pagerank_parallel(const int64_t *src_ptr,
                                         const int64_t *dst_ptr,
                                         uintptr_t edge_count,
                                         const double *weights_ptr,
                                         uintptr_t weights_count,
                                         double damping,
                                         uintptr_t iterations,
                                         bool directed,
                                         int64_t *out_node_ids,
                                         double *out_ranks);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#ifdef __cplusplus
} // namespace onager
#endif // __cplusplus

#endif /* ONAGER_H */

/* End of generated bindings */
