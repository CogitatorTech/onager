/**
 * @file scalar_functions.cpp
 * @brief Scalar functions for the Onager DuckDB extension.
 *
 * This file contains utility and graph management scalar functions.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

namespace {

/**
 * @brief Frees a Rust-allocated string when leaving scope, even on exception.
 */
struct OnagerStringGuard {
  char *ptr;
  explicit OnagerStringGuard(char *p) : ptr(p) {}
  ~OnagerStringGuard() {
    if (ptr) ::onager::onager_free(ptr);
  }
  OnagerStringGuard(const OnagerStringGuard &) = delete;
  OnagerStringGuard &operator=(const OnagerStringGuard &) = delete;
};

} // namespace

// =============================================================================
// Utility Scalar Functions
// =============================================================================

static void GetVersion(DataChunk &args, ExpressionState &state, Vector &result) {
  char *version_c = ::onager::onager_get_version();
  OnagerStringGuard guard(version_c);
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  if (!version_c) {
    ConstantVector::SetNull(result, true);
    return;
  }
  GetConstantVectorDataWritable<string_t>(result)[0] = StringVector::AddString(result, version_c);
  ConstantVector::SetNull(result, false);
}

static void GetLastError(DataChunk &args, ExpressionState &state, Vector &result) {
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  const char *err = ::onager::onager_last_error();
  if (!err) {
    ConstantVector::SetNull(result, true);
    return;
  }
  GetConstantVectorDataWritable<string_t>(result)[0] = StringVector::AddString(result, err);
  ConstantVector::SetNull(result, false);
}

static void GetNodeInDegree(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data, node_data;
  ONAGER_TO_UNIFIED_FORMAT(args.data[0], count, name_data);
  ONAGER_TO_UNIFIED_FORMAT(args.data[1], count, node_data);

  auto result_data = GetFlatVectorDataWritable<int64_t>(result);
  auto &result_validity = GetFlatVectorValidityWritable(result);

  for (idx_t i = 0; i < count; i++) {
    auto name_idx = name_data.sel->get_index(i);
    auto node_idx = node_data.sel->get_index(i);
    if (!name_data.validity.RowIsValid(name_idx) || !node_data.validity.RowIsValid(node_idx)) {
      result_validity.SetInvalid(i);
      continue;
    }
    auto name = ((string_t*)name_data.data)[name_idx];
    auto node = ((int64_t*)node_data.data)[node_idx];
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
  ONAGER_TO_UNIFIED_FORMAT(args.data[0], count, name_data);
  ONAGER_TO_UNIFIED_FORMAT(args.data[1], count, node_data);

  auto result_data = GetFlatVectorDataWritable<int64_t>(result);
  auto &result_validity = GetFlatVectorValidityWritable(result);

  for (idx_t i = 0; i < count; i++) {
    auto name_idx = name_data.sel->get_index(i);
    auto node_idx = node_data.sel->get_index(i);
    if (!name_data.validity.RowIsValid(name_idx) || !node_data.validity.RowIsValid(node_idx)) {
      result_validity.SetInvalid(i);
      continue;
    }
    auto name = ((string_t*)name_data.data)[name_idx];
    auto node = ((int64_t*)node_data.data)[node_idx];
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
  ONAGER_TO_UNIFIED_FORMAT(args.data[0], count, name_data);
  ONAGER_TO_UNIFIED_FORMAT(args.data[1], count, dir_data);

  auto result_data = GetFlatVectorDataWritable<int32_t>(result);
  auto &result_validity = GetFlatVectorValidityWritable(result);
  for (idx_t i = 0; i < count; i++) {
    auto name_idx = name_data.sel->get_index(i);
    auto dir_idx = dir_data.sel->get_index(i);
    if (!name_data.validity.RowIsValid(name_idx) || !dir_data.validity.RowIsValid(dir_idx)) {
      result_validity.SetInvalid(i);
      continue;
    }
    auto name = ((string_t*)name_data.data)[name_idx];
    auto dir = ((bool*)dir_data.data)[dir_idx];
    result_data[i] = ::onager::onager_create_graph(name.GetString().c_str(), dir);
  }
}

static void DropGraph(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data;
  ONAGER_TO_UNIFIED_FORMAT(args.data[0], count, name_data);

  auto result_data = GetFlatVectorDataWritable<int32_t>(result);
  auto &result_validity = GetFlatVectorValidityWritable(result);
  for (idx_t i = 0; i < count; i++) {
    auto name_idx = name_data.sel->get_index(i);
    if (!name_data.validity.RowIsValid(name_idx)) {
      result_validity.SetInvalid(i);
      continue;
    }
    auto name = ((string_t*)name_data.data)[name_idx];
    result_data[i] = ::onager::onager_drop_graph(name.GetString().c_str());
  }
}

static void AddNode(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data, node_data;
  ONAGER_TO_UNIFIED_FORMAT(args.data[0], count, name_data);
  ONAGER_TO_UNIFIED_FORMAT(args.data[1], count, node_data);

  auto result_data = GetFlatVectorDataWritable<int32_t>(result);
  auto &result_validity = GetFlatVectorValidityWritable(result);
  for (idx_t i = 0; i < count; i++) {
    auto name_idx = name_data.sel->get_index(i);
    auto node_idx = node_data.sel->get_index(i);
    if (!name_data.validity.RowIsValid(name_idx) || !node_data.validity.RowIsValid(node_idx)) {
      result_validity.SetInvalid(i);
      continue;
    }
    auto name = ((string_t*)name_data.data)[name_idx];
    auto node = ((int64_t*)node_data.data)[node_idx];
    result_data[i] = ::onager::onager_add_node(name.GetString().c_str(), node);
  }
}

static void AddEdge(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data, src_data, dst_data, w_data;
  ONAGER_TO_UNIFIED_FORMAT(args.data[0], count, name_data);
  ONAGER_TO_UNIFIED_FORMAT(args.data[1], count, src_data);
  ONAGER_TO_UNIFIED_FORMAT(args.data[2], count, dst_data);
  ONAGER_TO_UNIFIED_FORMAT(args.data[3], count, w_data);

  auto result_data = GetFlatVectorDataWritable<int32_t>(result);
  auto &result_validity = GetFlatVectorValidityWritable(result);
  for (idx_t i = 0; i < count; i++) {
    auto name_idx = name_data.sel->get_index(i);
    auto src_idx = src_data.sel->get_index(i);
    auto dst_idx = dst_data.sel->get_index(i);
    auto w_idx = w_data.sel->get_index(i);
    if (!name_data.validity.RowIsValid(name_idx) || !src_data.validity.RowIsValid(src_idx) ||
        !dst_data.validity.RowIsValid(dst_idx) || !w_data.validity.RowIsValid(w_idx)) {
      result_validity.SetInvalid(i);
      continue;
    }
    auto name = ((string_t*)name_data.data)[name_idx];
    auto src = ((int64_t*)src_data.data)[src_idx];
    auto dst = ((int64_t*)dst_data.data)[dst_idx];
    auto w = ((double*)w_data.data)[w_idx];
    result_data[i] = ::onager::onager_add_edge(name.GetString().c_str(), src, dst, w);
  }
}

static void ListGraphs(DataChunk &args, ExpressionState &state, Vector &result) {
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  char *json = ::onager::onager_list_graphs();
  OnagerStringGuard guard(json);
  if (!json) {
    ConstantVector::SetNull(result, true);
    return;
  }
  GetConstantVectorDataWritable<string_t>(result)[0] = StringVector::AddString(result, json);
  ConstantVector::SetNull(result, false);
}

static void GetNodeCount(DataChunk &args, ExpressionState &state, Vector &result) {
  auto count = args.size();
  UnifiedVectorFormat name_data;
  ONAGER_TO_UNIFIED_FORMAT(args.data[0], count, name_data);

  auto result_data = GetFlatVectorDataWritable<int64_t>(result);
  auto &result_validity = GetFlatVectorValidityWritable(result);
  for (idx_t i = 0; i < count; i++) {
    auto name_idx = name_data.sel->get_index(i);
    if (!name_data.validity.RowIsValid(name_idx)) {
      result_validity.SetInvalid(i);
      continue;
    }
    auto name = ((string_t*)name_data.data)[name_idx];
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
  ONAGER_TO_UNIFIED_FORMAT(args.data[0], count, name_data);

  auto result_data = GetFlatVectorDataWritable<int64_t>(result);
  auto &result_validity = GetFlatVectorValidityWritable(result);
  for (idx_t i = 0; i < count; i++) {
    auto name_idx = name_data.sel->get_index(i);
    if (!name_data.validity.RowIsValid(name_idx)) {
      result_validity.SetInvalid(i);
      continue;
    }
    auto name = ((string_t*)name_data.data)[name_idx];
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
  // Version and error functions. The last error is process state that can
  // change between (and within) queries, so it must not be constant-folded.
  loader.RegisterFunction(ScalarFunction("onager_version", {}, LogicalType::VARCHAR, GetVersion));
  ScalarFunction last_error("onager_last_error", {}, LogicalType::VARCHAR, GetLastError);
  last_error.SetVolatile();
  loader.RegisterFunction(last_error);

  // Graph management functions mutate or read the process-global graph
  // registry, so the optimizer must not cache or constant-fold them.
  ScalarFunction create_graph("onager_create_graph",
      {LogicalType::VARCHAR, LogicalType::BOOLEAN}, LogicalType::INTEGER, CreateGraph);
  create_graph.SetVolatile();
  loader.RegisterFunction(create_graph);

  ScalarFunction drop_graph("onager_drop_graph",
      {LogicalType::VARCHAR}, LogicalType::INTEGER, DropGraph);
  drop_graph.SetVolatile();
  loader.RegisterFunction(drop_graph);

  ScalarFunction add_node("onager_add_node",
      {LogicalType::VARCHAR, LogicalType::BIGINT}, LogicalType::INTEGER, AddNode);
  add_node.SetVolatile();
  loader.RegisterFunction(add_node);

  ScalarFunction add_edge("onager_add_edge",
      {LogicalType::VARCHAR, LogicalType::BIGINT, LogicalType::BIGINT, LogicalType::DOUBLE},
      LogicalType::INTEGER, AddEdge);
  add_edge.SetVolatile();
  loader.RegisterFunction(add_edge);

  ScalarFunction list_graphs("onager_list_graphs", {}, LogicalType::VARCHAR, ListGraphs);
  list_graphs.SetVolatile();
  loader.RegisterFunction(list_graphs);

  ScalarFunction node_count("onager_node_count",
      {LogicalType::VARCHAR}, LogicalType::BIGINT, GetNodeCount);
  node_count.SetVolatile();
  loader.RegisterFunction(node_count);

  ScalarFunction edge_count("onager_edge_count",
      {LogicalType::VARCHAR}, LogicalType::BIGINT, GetEdgeCount);
  edge_count.SetVolatile();
  loader.RegisterFunction(edge_count);

  // Node degree functions
  ScalarFunction node_in_degree("onager_node_in_degree",
      {LogicalType::VARCHAR, LogicalType::BIGINT}, LogicalType::BIGINT, GetNodeInDegree);
  node_in_degree.SetVolatile();
  loader.RegisterFunction(node_in_degree);

  ScalarFunction node_out_degree("onager_node_out_degree",
      {LogicalType::VARCHAR, LogicalType::BIGINT}, LogicalType::BIGINT, GetNodeOutDegree);
  node_out_degree.SetVolatile();
  loader.RegisterFunction(node_out_degree);
}

} // namespace onager
} // namespace duckdb
