/**
 * @file onager_extension.cpp
 * @brief This file contains the implementation of the Onager DuckDB extension.
 *
 * Onager is a graph data analytics extension for DuckDB, powered by the
 * graphina library. It provides SQL functions for graph creation, manipulation,
 * and analysis including centrality measures, community detection, and path finding.
 */
#define DUCKDB_EXTENSION_MAIN

#include "include/onager_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/types/data_chunk.hpp"
#include "duckdb/common/types/value.hpp"
#include "duckdb/common/types/vector.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rust.h"

namespace duckdb {

using namespace onager;

/**
 * @brief Retrieves the last error message from the Onager Rust core.
 * @return A string containing the error message, or "unknown error" if not set.
 */
static std::string GetOnagerError() {
  const char *err = onager_last_error();
  return err ? std::string(err) : std::string("unknown error");
}

/**
 * @brief Implements the `onager_version()` SQL function.
 * Returns the version string of the Onager extension.
 */
static void GetVersion(DataChunk &args, ExpressionState &state,
                       Vector &result) {
  char *version_c = onager_get_version();
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<string_t>(result)[0] =
      StringVector::AddString(result, version_c);
  ConstantVector::SetNull(result, false);
  onager_free(version_c);
}

/**
 * @brief Implements the `onager_last_error()` SQL function.
 * Returns the last error message string or NULL if no error is set.
 */
static void GetLastError(DataChunk &args, ExpressionState &state,
                         Vector &result) {
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  const char *err = onager_last_error();
  if (!err) {
    ConstantVector::SetNull(result, true);
    return;
  }
  ConstantVector::GetData<string_t>(result)[0] =
      StringVector::AddString(result, err);
  ConstantVector::SetNull(result, false);
}

/**
 * @brief Implements the `onager_create_graph(name, directed)` SQL function.
 * Creates a new in-memory graph with the given name.
 */
static void CreateGraph(DataChunk &args, ExpressionState &state,
                        Vector &result) {
  if (args.ColumnCount() < 1 || args.ColumnCount() > 2) {
    throw InvalidInputException(
        "onager_create_graph(name[, directed]) expects 1 or 2 arguments");
  }
  if (args.size() == 0) {
    return;
  }

  auto name_val = args.data[0].GetValue(0);
  if (name_val.IsNull()) {
    throw InvalidInputException("Graph name cannot be NULL");
  }

  std::string name = name_val.ToString();
  bool directed = false;
  if (args.ColumnCount() == 2) {
    auto dir_val = args.data[1].GetValue(0);
    if (!dir_val.IsNull()) {
      directed = dir_val.GetValue<bool>();
    }
  }

  int rc = onager_create_graph(name.c_str(), directed);
  if (rc != 0) {
    throw InvalidInputException("Failed to create graph: " + GetOnagerError());
  }

  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<bool>(result)[0] = true;
  ConstantVector::SetNull(result, false);
}

/**
 * @brief Implements the `onager_drop_graph(name)` SQL function.
 * Drops an existing in-memory graph.
 */
static void DropGraph(DataChunk &args, ExpressionState &state,
                      Vector &result) {
  if (args.ColumnCount() != 1) {
    throw InvalidInputException(
        "onager_drop_graph(name) expects exactly 1 argument");
  }
  if (args.size() == 0) {
    return;
  }

  auto name_val = args.data[0].GetValue(0);
  if (name_val.IsNull()) {
    throw InvalidInputException("Graph name cannot be NULL");
  }

  std::string name = name_val.ToString();
  int rc = onager_drop_graph(name.c_str());
  if (rc != 0) {
    throw InvalidInputException("Failed to drop graph: " + GetOnagerError());
  }

  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<bool>(result)[0] = true;
  ConstantVector::SetNull(result, false);
}

/**
 * @brief Implements the `onager_add_node(graph, node_id)` SQL function.
 * Adds a node to the specified graph.
 */
static void AddNode(DataChunk &args, ExpressionState &state, Vector &result) {
  if (args.ColumnCount() != 2) {
    throw InvalidInputException(
        "onager_add_node(graph, node_id) expects exactly 2 arguments");
  }
  if (args.size() == 0) {
    return;
  }

  auto graph_val = args.data[0].GetValue(0);
  auto node_val = args.data[1].GetValue(0);

  if (graph_val.IsNull() || node_val.IsNull()) {
    throw InvalidInputException("Graph name and node_id cannot be NULL");
  }

  std::string graph = graph_val.ToString();
  int64_t node_id = node_val.GetValue<int64_t>();

  int rc = onager_add_node(graph.c_str(), node_id);
  if (rc != 0) {
    throw InvalidInputException("Failed to add node: " + GetOnagerError());
  }

  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<bool>(result)[0] = true;
  ConstantVector::SetNull(result, false);
}

/**
 * @brief Implements the `onager_add_edge(graph, src, dst, weight)` SQL function.
 * Adds an edge to the specified graph.
 */
static void AddEdge(DataChunk &args, ExpressionState &state, Vector &result) {
  if (args.ColumnCount() < 3 || args.ColumnCount() > 4) {
    throw InvalidInputException(
        "onager_add_edge(graph, src, dst[, weight]) expects 3 or 4 arguments");
  }
  if (args.size() == 0) {
    return;
  }

  auto graph_val = args.data[0].GetValue(0);
  auto src_val = args.data[1].GetValue(0);
  auto dst_val = args.data[2].GetValue(0);

  if (graph_val.IsNull() || src_val.IsNull() || dst_val.IsNull()) {
    throw InvalidInputException("Graph name, src, and dst cannot be NULL");
  }

  std::string graph = graph_val.ToString();
  int64_t src = src_val.GetValue<int64_t>();
  int64_t dst = dst_val.GetValue<int64_t>();
  double weight = 1.0;

  if (args.ColumnCount() == 4) {
    auto weight_val = args.data[3].GetValue(0);
    if (!weight_val.IsNull()) {
      weight = weight_val.GetValue<double>();
    }
  }

  int rc = onager_add_edge(graph.c_str(), src, dst, weight);
  if (rc != 0) {
    throw InvalidInputException("Failed to add edge: " + GetOnagerError());
  }

  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<bool>(result)[0] = true;
  ConstantVector::SetNull(result, false);
}

/**
 * @brief Implements the `onager_node_count(graph)` SQL function.
 * Returns the number of nodes in the graph.
 */
static void NodeCount(DataChunk &args, ExpressionState &state, Vector &result) {
  if (args.ColumnCount() != 1) {
    throw InvalidInputException(
        "onager_node_count(graph) expects exactly 1 argument");
  }
  if (args.size() == 0) {
    return;
  }

  auto graph_val = args.data[0].GetValue(0);
  if (graph_val.IsNull()) {
    throw InvalidInputException("Graph name cannot be NULL");
  }

  std::string graph = graph_val.ToString();
  int64_t count = onager_node_count(graph.c_str());
  if (count < 0) {
    throw InvalidInputException("Failed to get node count: " + GetOnagerError());
  }

  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<int64_t>(result)[0] = count;
  ConstantVector::SetNull(result, false);
}

/**
 * @brief Implements the `onager_edge_count(graph)` SQL function.
 * Returns the number of edges in the graph.
 */
static void EdgeCount(DataChunk &args, ExpressionState &state, Vector &result) {
  if (args.ColumnCount() != 1) {
    throw InvalidInputException(
        "onager_edge_count(graph) expects exactly 1 argument");
  }
  if (args.size() == 0) {
    return;
  }

  auto graph_val = args.data[0].GetValue(0);
  if (graph_val.IsNull()) {
    throw InvalidInputException("Graph name cannot be NULL");
  }

  std::string graph = graph_val.ToString();
  int64_t count = onager_edge_count(graph.c_str());
  if (count < 0) {
    throw InvalidInputException("Failed to get edge count: " + GetOnagerError());
  }

  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<int64_t>(result)[0] = count;
  ConstantVector::SetNull(result, false);
}

/**
 * @brief Implements the `onager_list_graphs()` SQL function.
 * Returns a JSON array of all graph names.
 */
static void ListGraphs(DataChunk &args, ExpressionState &state, Vector &result) {
  char *graphs_json = onager_list_graphs();
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<string_t>(result)[0] =
      StringVector::AddString(result, graphs_json);
  ConstantVector::SetNull(result, false);
  onager_free(graphs_json);
}

// Implementation of OnagerExtension class methods
void OnagerExtension::Load(ExtensionLoader &loader) {
  loader.SetDescription("Onager: Graph Data Analytics Extension");
  
  // Version function
  loader.RegisterFunction(
      ScalarFunction("onager_version", {}, LogicalType::VARCHAR, GetVersion));

  // Last error function
  loader.RegisterFunction(
      ScalarFunction("onager_last_error", {}, LogicalType::VARCHAR, GetLastError));

  // Graph management functions
  loader.RegisterFunction(
      ScalarFunction("onager_create_graph",
                     {LogicalType::VARCHAR},
                     LogicalType::BOOLEAN, CreateGraph));
  loader.RegisterFunction(
      ScalarFunction("onager_create_graph",
                     {LogicalType::VARCHAR, LogicalType::BOOLEAN},
                     LogicalType::BOOLEAN, CreateGraph));

  loader.RegisterFunction(
      ScalarFunction("onager_drop_graph",
                     {LogicalType::VARCHAR},
                     LogicalType::BOOLEAN, DropGraph));

  loader.RegisterFunction(
      ScalarFunction("onager_list_graphs", {}, LogicalType::VARCHAR, ListGraphs));

  // Node and edge functions
  loader.RegisterFunction(
      ScalarFunction("onager_add_node",
                     {LogicalType::VARCHAR, LogicalType::BIGINT},
                     LogicalType::BOOLEAN, AddNode));

  loader.RegisterFunction(
      ScalarFunction("onager_add_edge",
                     {LogicalType::VARCHAR, LogicalType::BIGINT, LogicalType::BIGINT},
                     LogicalType::BOOLEAN, AddEdge));
  loader.RegisterFunction(
      ScalarFunction("onager_add_edge",
                     {LogicalType::VARCHAR, LogicalType::BIGINT, LogicalType::BIGINT, LogicalType::DOUBLE},
                     LogicalType::BOOLEAN, AddEdge));

  // Graph info functions
  loader.RegisterFunction(
      ScalarFunction("onager_node_count",
                     {LogicalType::VARCHAR},
                     LogicalType::BIGINT, NodeCount));

  loader.RegisterFunction(
      ScalarFunction("onager_edge_count",
                     {LogicalType::VARCHAR},
                     LogicalType::BIGINT, EdgeCount));
}

std::string OnagerExtension::Name() {
  return "onager";
}

std::string OnagerExtension::Version() const {
#ifdef EXT_VERSION_ONAGER
  return EXT_VERSION_ONAGER;
#else
  return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void onager_init(duckdb::DatabaseInstance &db) {
  duckdb::DuckDB db_wrapper(db);
  db_wrapper.LoadStaticExtension<duckdb::OnagerExtension>();
}

DUCKDB_EXTENSION_API const char *onager_version() {
  return duckdb::DuckDB::LibraryVersion();
}

}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
