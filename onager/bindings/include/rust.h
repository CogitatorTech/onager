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
 * Compute Maximum Clique Approximation.
 */

int64_t onager_compute_max_clique(const int64_t *src_ptr,
                                  const int64_t *dst_ptr,
                                  uintptr_t edge_count,
                                  int64_t *out_nodes);

/**
 * Compute Maximum Independent Set Approximation.
 */

int64_t onager_compute_independent_set(const int64_t *src_ptr,
                                       const int64_t *dst_ptr,
                                       uintptr_t edge_count,
                                       int64_t *out_nodes);

/**
 * Compute Minimum Vertex Cover Approximation.
 */

int64_t onager_compute_vertex_cover(const int64_t *src_ptr,
                                    const int64_t *dst_ptr,
                                    uintptr_t edge_count,
                                    int64_t *out_nodes);

/**
 * Compute PageRank on edge arrays.
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

/**
 * Compute degree centrality on edge arrays.
 */

int64_t onager_compute_degree(const int64_t *src_ptr,
                              const int64_t *dst_ptr,
                              uintptr_t edge_count,
                              bool directed,
                              int64_t *out_nodes,
                              double *out_in_degree,
                              double *out_out_degree);

/**
 * Compute in-degree of a single node (scalar).
 */

int64_t onager_compute_node_in_degree(const int64_t *src_ptr,
                                      const int64_t *dst_ptr,
                                      uintptr_t edge_count,
                                      int64_t node);

/**
 * Compute out-degree of a single node (scalar).
 */

int64_t onager_compute_node_out_degree(const int64_t *src_ptr,
                                       const int64_t *dst_ptr,
                                       uintptr_t edge_count,
                                       int64_t node);

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
 * Compute closeness centrality.
 */

int64_t onager_compute_closeness(const int64_t *src_ptr,
                                 const int64_t *dst_ptr,
                                 uintptr_t edge_count,
                                 int64_t *out_nodes,
                                 double *out_centralities);

/**
 * Compute eigenvector centrality.
 */

int64_t onager_compute_eigenvector(const int64_t *src_ptr,
                                   const int64_t *dst_ptr,
                                   uintptr_t edge_count,
                                   uintptr_t max_iter,
                                   double tolerance,
                                   int64_t *out_nodes,
                                   double *out_centralities);

/**
 * Compute Katz centrality.
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
 * Compute harmonic centrality.
 */

int64_t onager_compute_harmonic(const int64_t *src_ptr,
                                const int64_t *dst_ptr,
                                uintptr_t edge_count,
                                int64_t *out_nodes,
                                double *out_centralities);

/**
 * Compute VoteRank for influential spreaders.
 */

int64_t onager_compute_voterank(const int64_t *src_ptr,
                                const int64_t *dst_ptr,
                                uintptr_t edge_count,
                                uintptr_t num_seeds,
                                int64_t *out_nodes);

/**
 * Returns the last error message, or null if no error is set.
 */
 const char *onager_last_error(void);

/**
 * Frees a string allocated by Onager.
 * # Safety
 * The pointer must have been allocated by Onager (via CString::into_raw).
 */
 void onager_free(char *ptr);

/**
 * Returns the version string, caller must free with onager_free.
 */
 char *onager_get_version(void);

/**
 * Creates a new graph with the given name.
 * # Safety
 * The name pointer must be a valid null-terminated C string.
 */
 int32_t onager_create_graph(const char *name, bool directed);

/**
 * Drops a graph with the given name.
 * # Safety
 * The name pointer must be a valid null-terminated C string.
 */
 int32_t onager_drop_graph(const char *name);

/**
 * Returns a JSON array of all graph names.
 */
 char *onager_list_graphs(void);

/**
 * Adds a node to the specified graph.
 * # Safety
 * The graph_name pointer must be a valid null-terminated C string.
 */
 int32_t onager_add_node(const char *graph_name, int64_t node_id);

/**
 * Adds an edge to the specified graph.
 * # Safety
 * The graph_name pointer must be a valid null-terminated C string.
 */
 int32_t onager_add_edge(const char *graph_name, int64_t src, int64_t dst, double weight);

/**
 * Returns the number of nodes in the graph.
 * # Safety
 * The graph_name pointer must be a valid null-terminated C string.
 */
 int64_t onager_node_count(const char *graph_name);

/**
 * Returns the number of edges in the graph.
 * # Safety
 * The graph_name pointer must be a valid null-terminated C string.
 */
 int64_t onager_edge_count(const char *graph_name);

/**
 * Returns the in-degree of a node in the named graph.
 * # Safety
 * The graph_name pointer must be a valid null-terminated C string.
 */
 int64_t onager_graph_node_in_degree(const char *graph_name, int64_t node);

/**
 * Returns the out-degree of a node in the named graph.
 * # Safety
 * The graph_name pointer must be a valid null-terminated C string.
 */
 int64_t onager_graph_node_out_degree(const char *graph_name, int64_t node);

/**
 * Compute Louvain community detection.
 */

int64_t onager_compute_louvain(const int64_t *src_ptr,
                               const int64_t *dst_ptr,
                               uintptr_t edge_count,
                               int64_t seed,
                               int64_t *out_nodes,
                               int64_t *out_communities);

/**
 * Compute connected components.
 */

int64_t onager_compute_connected_components(const int64_t *src_ptr,
                                            const int64_t *dst_ptr,
                                            uintptr_t edge_count,
                                            int64_t *out_nodes,
                                            int64_t *out_components);

/**
 * Compute label propagation.
 */

int64_t onager_compute_label_propagation(const int64_t *src_ptr,
                                         const int64_t *dst_ptr,
                                         uintptr_t edge_count,
                                         int64_t *out_node_ids,
                                         int64_t *out_labels);

/**
 * Compute Girvan-Newman community detection.
 */

int64_t onager_compute_girvan_newman(const int64_t *src_ptr,
                                     const int64_t *dst_ptr,
                                     uintptr_t edge_count,
                                     int64_t target_communities,
                                     int64_t *out_nodes,
                                     int64_t *out_communities);

/**
 * Compute spectral clustering.
 */

int64_t onager_compute_spectral_clustering(const int64_t *src_ptr,
                                           const int64_t *dst_ptr,
                                           uintptr_t edge_count,
                                           uintptr_t k,
                                           int64_t seed,
                                           int64_t *out_nodes,
                                           int64_t *out_communities);

/**
 * Compute infomap community detection.
 */

int64_t onager_compute_infomap(const int64_t *src_ptr,
                               const int64_t *dst_ptr,
                               uintptr_t edge_count,
                               uintptr_t max_iter,
                               int64_t seed,
                               int64_t *out_nodes,
                               int64_t *out_communities);

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
 * Compute graph diameter.
 */

int64_t onager_compute_diameter(const int64_t *src_ptr,
                                const int64_t *dst_ptr,
                                uintptr_t edge_count);

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
 * Compute triangle count for each node.
 */

int64_t onager_compute_triangle_count(const int64_t *src_ptr,
                                      const int64_t *dst_ptr,
                                      uintptr_t edge_count,
                                      int64_t *out_nodes,
                                      int64_t *out_counts);

/**
 * Compute assortativity coefficient.
 */

double onager_compute_assortativity(const int64_t *src_ptr,
                                    const int64_t *dst_ptr,
                                    uintptr_t edge_count);

/**
 * Compute Prim's MST on weighted edge arrays.
 */

int64_t onager_compute_prim_mst(const int64_t *src_ptr,
                                const int64_t *dst_ptr,
                                const double *weight_ptr,
                                uintptr_t edge_count,
                                int64_t *out_src,
                                int64_t *out_dst,
                                double *out_weight,
                                double *out_total);

/**
 * Compute Kruskal's MST on weighted edge arrays.
 */

int64_t onager_compute_kruskal_mst(const int64_t *src_ptr,
                                   const int64_t *dst_ptr,
                                   const double *weight_ptr,
                                   uintptr_t edge_count,
                                   int64_t *out_src,
                                   int64_t *out_dst,
                                   double *out_weight,
                                   double *out_total);

/**
 * Compute parallel BFS from a single source.
 */

int64_t onager_compute_bfs_parallel(const int64_t *src_ptr,
                                    const int64_t *dst_ptr,
                                    uintptr_t edge_count,
                                    int64_t source,
                                    int64_t *out_order);

/**
 * Compute parallel shortest paths from a single source.
 */

int64_t onager_compute_shortest_paths_parallel(const int64_t *src_ptr,
                                               const int64_t *dst_ptr,
                                               uintptr_t edge_count,
                                               int64_t source,
                                               int64_t *out_nodes,
                                               double *out_distances);

/**
 * Compute parallel connected components.
 */

int64_t onager_compute_components_parallel(const int64_t *src_ptr,
                                           const int64_t *dst_ptr,
                                           uintptr_t edge_count,
                                           int64_t *out_nodes,
                                           int64_t *out_components);

/**
 * Compute parallel clustering coefficients.
 */

int64_t onager_compute_clustering_parallel(const int64_t *src_ptr,
                                           const int64_t *dst_ptr,
                                           uintptr_t edge_count,
                                           int64_t *out_nodes,
                                           double *out_coefficients);

/**
 * Compute parallel triangle count.
 */

int64_t onager_compute_triangles_parallel(const int64_t *src_ptr,
                                          const int64_t *dst_ptr,
                                          uintptr_t edge_count,
                                          int64_t *out_nodes,
                                          int64_t *out_counts);

/**
 * Compute personalized PageRank.
 */

int64_t onager_compute_personalized_pagerank(const int64_t *src_ptr,
                                             const int64_t *dst_ptr,
                                             uintptr_t edge_count,
                                             const int64_t *pers_nodes_ptr,
                                             const double *pers_weights_ptr,
                                             uintptr_t pers_count,
                                             double damping,
                                             uintptr_t max_iter,
                                             double tolerance,
                                             int64_t *out_nodes,
                                             double *out_scores);

/**
 * Compute ego graph.
 */

int64_t onager_compute_ego_graph(const int64_t *src_ptr,
                                 const int64_t *dst_ptr,
                                 uintptr_t edge_count,
                                 int64_t center,
                                 uintptr_t radius,
                                 int64_t *out_src,
                                 int64_t *out_dst);

/**
 * Compute k-hop neighbors.
 */

int64_t onager_compute_k_hop_neighbors(const int64_t *src_ptr,
                                       const int64_t *dst_ptr,
                                       uintptr_t edge_count,
                                       int64_t start,
                                       uintptr_t k,
                                       int64_t *out_nodes);

/**
 * Compute induced subgraph.
 */

int64_t onager_compute_induced_subgraph(const int64_t *src_ptr,
                                        const int64_t *dst_ptr,
                                        uintptr_t edge_count,
                                        const int64_t *node_ids_ptr,
                                        uintptr_t node_count,
                                        int64_t *out_src,
                                        int64_t *out_dst);

/**
 * Compute Dijkstra shortest paths.
 */

int64_t onager_compute_dijkstra(const int64_t *src_ptr,
                                const int64_t *dst_ptr,
                                uintptr_t edge_count,
                                int64_t source_node,
                                int64_t *out_nodes,
                                double *out_distances);

/**
 * Compute BFS traversal.
 */

int64_t onager_compute_bfs(const int64_t *src_ptr,
                           const int64_t *dst_ptr,
                           uintptr_t edge_count,
                           int64_t source_node,
                           int64_t *out_order);

/**
 * Compute DFS traversal.
 */

int64_t onager_compute_dfs(const int64_t *src_ptr,
                           const int64_t *dst_ptr,
                           uintptr_t edge_count,
                           int64_t source_node,
                           int64_t *out_order);

/**
 * Compute Bellman-Ford shortest paths on weighted edge arrays.
 */

int64_t onager_compute_bellman_ford(const int64_t *src_ptr,
                                    const int64_t *dst_ptr,
                                    const double *weight_ptr,
                                    uintptr_t edge_count,
                                    int64_t source,
                                    int64_t *out_nodes,
                                    double *out_distances);

/**
 * Compute Floyd-Warshall all-pairs shortest paths.
 */

int64_t onager_compute_floyd_warshall(const int64_t *src_ptr,
                                      const int64_t *dst_ptr,
                                      const double *weight_ptr,
                                      uintptr_t edge_count,
                                      int64_t *out_src,
                                      int64_t *out_dst,
                                      double *out_distances);

/**
 * Compute shortest distance between two nodes (scalar).
 */

double onager_compute_shortest_distance(const int64_t *src_ptr,
                                        const int64_t *dst_ptr,
                                        uintptr_t edge_count,
                                        int64_t source_node,
                                        int64_t target_node);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#ifdef __cplusplus
} // namespace onager
#endif // __cplusplus

#endif /* ONAGER_H */

/* End of generated bindings */
