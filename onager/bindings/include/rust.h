/* Generated with cbindgen */
/* DO NOT EDIT */


#ifndef ONAGER_H
#define ONAGER_H

#pragma once

/* Generated with cbindgen:0.29.0 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
namespace onager {
#endif  // __cplusplus

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

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#ifdef __cplusplus
}  // namespace onager
#endif  // __cplusplus

#endif  /* ONAGER_H */

/* End of generated bindings */
