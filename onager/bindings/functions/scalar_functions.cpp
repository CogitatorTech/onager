/**
 * @file scalar_functions.cpp
 * @brief Scalar functions for the Onager DuckDB extension.
 *
 * This file contains utility and graph management scalar functions.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Utility Scalar Functions
// =============================================================================

static void GetVersion(DataChunk &args, ExpressionState &state, Vector &result) {
  char *version_c = ::onager::onager_get_version();
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<string_t>(result)[0] = StringVector::AddString(result, version_c);
  ConstantVector::SetNull(result, false);
  ::onager::onager_free(version_c);
}

static void GetLastError(DataChunk &args, ExpressionState &state, Vector &result) {
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  const char *err = ::onager::onager_last_error();
  if (!err) {
    ConstantVector::SetNull(result, true);
    return;
  }
  ConstantVector::GetData<string_t>(result)[0] = StringVector::AddString(result, err);
  ConstantVector::SetNull(result, false);
}

static void GetNodeInDegree(DataChunk &args, ExpressionState &state, Vector &result) {
  auto &graph_name_vec = args.data[0];
  auto &node_vec = args.data[1];
  auto count = args.size();

  auto result_data = FlatVector::GetData<int64_t>(result);
  auto &result_validity = FlatVector::Validity(result);

  for (idx_t i = 0; i < count; i++) {
    auto graph_name = FlatVector::GetData<string_t>(graph_name_vec)[i].GetString();
    auto node = FlatVector::GetData<int64_t>(node_vec)[i];
    int64_t degree = ::onager::onager_graph_node_in_degree(graph_name.c_str(), node);
    if (degree < 0) {
      result_validity.SetInvalid(i);
    } else {
      result_data[i] = degree;
    }
  }
}

static void GetNodeOutDegree(DataChunk &args, ExpressionState &state, Vector &result) {
  auto &graph_name_vec = args.data[0];
  auto &node_vec = args.data[1];
  auto count = args.size();

  auto result_data = FlatVector::GetData<int64_t>(result);
  auto &result_validity = FlatVector::Validity(result);

  for (idx_t i = 0; i < count; i++) {
    auto graph_name = FlatVector::GetData<string_t>(graph_name_vec)[i].GetString();
    auto node = FlatVector::GetData<int64_t>(node_vec)[i];
    int64_t degree = ::onager::onager_graph_node_out_degree(graph_name.c_str(), node);
    if (degree < 0) {
      result_validity.SetInvalid(i);
    } else {
      result_data[i] = degree;
    }
  }
}

// =============================================================================
// Graph Management Scalar Functions
// =============================================================================

static void CreateGraph(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  auto result_data = FlatVector::GetData<int32_t>(result);
  for (idx_t i = 0; i < count; i++) {
    auto graph_name = FlatVector::GetData<string_t>(args.data[0])[i].GetString();
    auto directed = FlatVector::GetData<bool>(args.data[1])[i];
    result_data[i] = ::onager::onager_create_graph(graph_name.c_str(), directed);
  }
}

static void DropGraph(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  auto result_data = FlatVector::GetData<int32_t>(result);
  for (idx_t i = 0; i < count; i++) {
    auto graph_name = FlatVector::GetData<string_t>(args.data[0])[i].GetString();
    result_data[i] = ::onager::onager_drop_graph(graph_name.c_str());
  }
}

static void AddNode(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  auto result_data = FlatVector::GetData<int32_t>(result);
  for (idx_t i = 0; i < count; i++) {
    auto graph_name = FlatVector::GetData<string_t>(args.data[0])[i].GetString();
    auto node_id = FlatVector::GetData<int64_t>(args.data[1])[i];
    result_data[i] = ::onager::onager_add_node(graph_name.c_str(), node_id);
  }
}

static void AddEdge(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  auto result_data = FlatVector::GetData<int32_t>(result);
  for (idx_t i = 0; i < count; i++) {
    auto graph_name = FlatVector::GetData<string_t>(args.data[0])[i].GetString();
    auto src = FlatVector::GetData<int64_t>(args.data[1])[i];
    auto dst = FlatVector::GetData<int64_t>(args.data[2])[i];
    auto weight = FlatVector::GetData<double>(args.data[3])[i];
    result_data[i] = ::onager::onager_add_edge(graph_name.c_str(), src, dst, weight);
  }
}

static void ListGraphs(DataChunk &args, ExpressionState &state, Vector &result) {
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  char *json = ::onager::onager_list_graphs();
  if (!json) {
    ConstantVector::SetNull(result, true);
    return;
  }
  ConstantVector::GetData<string_t>(result)[0] = StringVector::AddString(result, json);
  ConstantVector::SetNull(result, false);
  ::onager::onager_free(json);
}

static void GetNodeCount(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  auto result_data = FlatVector::GetData<int64_t>(result);
  auto &result_validity = FlatVector::Validity(result);
  for (idx_t i = 0; i < count; i++) {
    auto graph_name = FlatVector::GetData<string_t>(args.data[0])[i].GetString();
    int64_t node_count = ::onager::onager_node_count(graph_name.c_str());
    if (node_count < 0) {
      result_validity.SetInvalid(i);
    } else {
      result_data[i] = node_count;
    }
  }
}

static void GetEdgeCount(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  auto result_data = FlatVector::GetData<int64_t>(result);
  auto &result_validity = FlatVector::Validity(result);
  for (idx_t i = 0; i < count; i++) {
    auto graph_name = FlatVector::GetData<string_t>(args.data[0])[i].GetString();
    int64_t edge_count = ::onager::onager_edge_count(graph_name.c_str());
    if (edge_count < 0) {
      result_validity.SetInvalid(i);
    } else {
      result_data[i] = edge_count;
    }
  }
}

namespace onager {

void RegisterScalarFunctions(ExtensionLoader &loader) {
  // Version and error functions
  loader.RegisterFunction(ScalarFunction("onager_version", {}, LogicalType::VARCHAR, GetVersion));
  loader.RegisterFunction(ScalarFunction("onager_last_error", {}, LogicalType::VARCHAR, GetLastError));

  // Graph management functions
  loader.RegisterFunction(ScalarFunction("onager_create_graph",
      {LogicalType::VARCHAR, LogicalType::BOOLEAN}, LogicalType::INTEGER, CreateGraph));
  loader.RegisterFunction(ScalarFunction("onager_drop_graph",
      {LogicalType::VARCHAR}, LogicalType::INTEGER, DropGraph));
  loader.RegisterFunction(ScalarFunction("onager_add_node",
      {LogicalType::VARCHAR, LogicalType::BIGINT}, LogicalType::INTEGER, AddNode));
  loader.RegisterFunction(ScalarFunction("onager_add_edge",
      {LogicalType::VARCHAR, LogicalType::BIGINT, LogicalType::BIGINT, LogicalType::DOUBLE},
      LogicalType::INTEGER, AddEdge));
  loader.RegisterFunction(ScalarFunction("onager_list_graphs", {}, LogicalType::VARCHAR, ListGraphs));
  loader.RegisterFunction(ScalarFunction("onager_node_count",
      {LogicalType::VARCHAR}, LogicalType::BIGINT, GetNodeCount));
  loader.RegisterFunction(ScalarFunction("onager_edge_count",
      {LogicalType::VARCHAR}, LogicalType::BIGINT, GetEdgeCount));

  // Node degree functions
  loader.RegisterFunction(ScalarFunction("onager_node_in_degree",
      {LogicalType::VARCHAR, LogicalType::BIGINT}, LogicalType::BIGINT, GetNodeInDegree));
  loader.RegisterFunction(ScalarFunction("onager_node_out_degree",
      {LogicalType::VARCHAR, LogicalType::BIGINT}, LogicalType::BIGINT, GetNodeOutDegree));
}

} // namespace onager
} // namespace duckdb
