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
  auto count = args.size();
  UnifiedVectorFormat name_data, node_data;
  args.data[0].ToUnifiedFormat(count, name_data);
  args.data[1].ToUnifiedFormat(count, node_data);

  auto result_data = FlatVector::GetData<int64_t>(result);
  auto &result_validity = FlatVector::Validity(result);

  for (idx_t i = 0; i < count; i++) {
    auto name = ((string_t*)name_data.data)[name_data.sel->get_index(i)];
    auto node = ((int64_t*)node_data.data)[node_data.sel->get_index(i)];
    int64_t degree = ::onager::onager_graph_node_in_degree(name.GetString().c_str(), node);
    if (degree < 0) {
      result_validity.SetInvalid(i);
    } else {
      result_data[i] = degree;
    }
  }
}

static void GetNodeOutDegree(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data, node_data;
  args.data[0].ToUnifiedFormat(count, name_data);
  args.data[1].ToUnifiedFormat(count, node_data);

  auto result_data = FlatVector::GetData<int64_t>(result);
  auto &result_validity = FlatVector::Validity(result);

  for (idx_t i = 0; i < count; i++) {
    auto name = ((string_t*)name_data.data)[name_data.sel->get_index(i)];
    auto node = ((int64_t*)node_data.data)[node_data.sel->get_index(i)];
    int64_t degree = ::onager::onager_graph_node_out_degree(name.GetString().c_str(), node);
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
  UnifiedVectorFormat name_data, dir_data;
  args.data[0].ToUnifiedFormat(count, name_data);
  args.data[1].ToUnifiedFormat(count, dir_data);

  auto result_data = FlatVector::GetData<int32_t>(result);
  for (idx_t i = 0; i < count; i++) {
    auto name = ((string_t*)name_data.data)[name_data.sel->get_index(i)];
    auto dir = ((bool*)dir_data.data)[dir_data.sel->get_index(i)];
    result_data[i] = ::onager::onager_create_graph(name.GetString().c_str(), dir);
  }
}

static void DropGraph(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data;
  args.data[0].ToUnifiedFormat(count, name_data);

  auto result_data = FlatVector::GetData<int32_t>(result);
  for (idx_t i = 0; i < count; i++) {
    auto name = ((string_t*)name_data.data)[name_data.sel->get_index(i)];
    result_data[i] = ::onager::onager_drop_graph(name.GetString().c_str());
  }
}

static void AddNode(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data, node_data;
  args.data[0].ToUnifiedFormat(count, name_data);
  args.data[1].ToUnifiedFormat(count, node_data);

  auto result_data = FlatVector::GetData<int32_t>(result);
  for (idx_t i = 0; i < count; i++) {
    auto name = ((string_t*)name_data.data)[name_data.sel->get_index(i)];
    auto node = ((int64_t*)node_data.data)[node_data.sel->get_index(i)];
    result_data[i] = ::onager::onager_add_node(name.GetString().c_str(), node);
  }
}

static void AddEdge(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data, src_data, dst_data, w_data;
  args.data[0].ToUnifiedFormat(count, name_data);
  args.data[1].ToUnifiedFormat(count, src_data);
  args.data[2].ToUnifiedFormat(count, dst_data);
  args.data[3].ToUnifiedFormat(count, w_data);

  auto result_data = FlatVector::GetData<int32_t>(result);
  for (idx_t i = 0; i < count; i++) {
    auto name = ((string_t*)name_data.data)[name_data.sel->get_index(i)];
    auto src = ((int64_t*)src_data.data)[src_data.sel->get_index(i)];
    auto dst = ((int64_t*)dst_data.data)[dst_data.sel->get_index(i)];
    auto w = ((double*)w_data.data)[w_data.sel->get_index(i)];
    result_data[i] = ::onager::onager_add_edge(name.GetString().c_str(), src, dst, w);
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
  UnifiedVectorFormat name_data;
  args.data[0].ToUnifiedFormat(count, name_data);

  auto result_data = FlatVector::GetData<int64_t>(result);
  auto &result_validity = FlatVector::Validity(result);
  for (idx_t i = 0; i < count; i++) {
    auto name = ((string_t*)name_data.data)[name_data.sel->get_index(i)];
    int64_t node_count = ::onager::onager_node_count(name.GetString().c_str());
    if (node_count < 0) {
      result_validity.SetInvalid(i);
    } else {
      result_data[i] = node_count;
    }
  }
}

static void GetEdgeCount(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data;
  args.data[0].ToUnifiedFormat(count, name_data);

  auto result_data = FlatVector::GetData<int64_t>(result);
  auto &result_validity = FlatVector::Validity(result);
  for (idx_t i = 0; i < count; i++) {
    auto name = ((string_t*)name_data.data)[name_data.sel->get_index(i)];
    int64_t edge_count = ::onager::onager_edge_count(name.GetString().c_str());
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
