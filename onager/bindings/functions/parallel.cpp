/**
 * @file parallel.cpp
 * @brief Parallel algorithm table functions for Onager DuckDB extension.
 *
 * Parallel PageRank, BFS, Shortest Paths, Connected Components, Clustering Coefficients, Triangle Count.
 */
#include "functions.hpp"
#include <mutex>

namespace duckdb {

using namespace onager;

// =============================================================================
// Parallel PageRank
// =============================================================================

struct ParallelPageRankBindData : public TableFunctionData {
  double damping = 0.85;
  int64_t iterations = 100;
  double tolerance = 1e-6;
  bool directed = false;
  bool weighted = false;
};
struct ParallelPageRankGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> weights, result_ranks;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelPageRankBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<ParallelPageRankBindData>();
  CheckInt64Input(input, "onager_par_pagerank");
  bd->weighted = input.input_table_types.size() >= 3 && input.input_table_types[2] == LogicalType::DOUBLE;
  for (auto &kv : input.named_parameters) {
    if (kv.first == "damping") bd->damping = GetRequiredParam<double>("onager_par_pagerank", "damping", kv.second);
    if (kv.first == "iterations") bd->iterations = GetNonNegativeParam("onager_par_pagerank", "iterations", kv.second);
    if (kv.first == "tolerance") bd->tolerance = GetRequiredParam<double>("onager_par_pagerank", "tolerance", kv.second);
    if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_par_pagerank", "directed", kv.second);
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("rank");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> ParallelPageRankInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelPageRankGlobalState>(); }
static OperatorResultType ParallelPageRankInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &bd = data.bind_data->Cast<ParallelPageRankBindData>();
  auto &gs = data.global_state->Cast<ParallelPageRankGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (bd.weighted) {
    AppendWeightedEdges(input, gs.src_nodes, gs.dst_nodes, gs.weights, "onager_par_pagerank");
  } else {
    AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_par_pagerank");
  }
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelPageRankFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<ParallelPageRankBindData>(); auto &gs = data.global_state->Cast<ParallelPageRankGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    const double *w = gs.weights.empty() ? nullptr : gs.weights.data();
    int64_t nc = ::onager::onager_compute_pagerank_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), w, gs.weights.size(), bd.damping, bd.iterations, bd.tolerance, bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel PageRank failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_ranks.resize(nc);
    int64_t rc = ::onager::onager_compute_pagerank_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), w, gs.weights.size(), bd.damping, bd.iterations, bd.tolerance, bd.directed, gs.result_nodes.data(), gs.result_ranks.data());
    if (rc != nc) throw InvalidInputException("Parallel PageRank failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto r = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; r[i] = gs.result_ranks[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel BFS
// =============================================================================

struct ParallelBfsBindData : public TableFunctionData { int64_t source = 0; std::vector<int64_t> sources; bool multi = false; bool directed = false; };
struct ParallelBfsGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_sources, result_order;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelBfsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<ParallelBfsBindData>();
  CheckInt64Input(input, "onager_par_bfs");
  bool has_source = false;
  for (auto &kv : input.named_parameters) {
    if (kv.first == "source") { bd->source = GetRequiredParam<int64_t>("onager_par_bfs", "source", kv.second); has_source = true; }
    else if (kv.first == "sources") {
      if (kv.second.IsNull()) throw BinderException("onager_par_bfs parameter sources cannot be NULL");
      bd->multi = true;
      for (auto &v : ListValue::GetChildren(kv.second)) {
        if (v.IsNull()) throw BinderException("onager_par_bfs: sources must not contain NULL");
        bd->sources.push_back(v.GetValue<int64_t>());
      }
      if (bd->sources.empty()) throw BinderException("onager_par_bfs: sources must contain at least one node");
    }
    else if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_par_bfs", "directed", kv.second);
  }
  if (bd->multi && has_source) throw BinderException("onager_par_bfs: pass either source or sources, not both");
  if (bd->multi) { rt.push_back(LogicalType::BIGINT); nm.push_back("source"); }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> ParallelBfsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelBfsGlobalState>(); }
static OperatorResultType ParallelBfsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelBfsGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_par_bfs");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelBfsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<ParallelBfsBindData>(); auto &gs = data.global_state->Cast<ParallelBfsGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    if (bd.multi) {
      int64_t nc = ::onager::onager_compute_bfs_parallel_multi(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.sources.data(), bd.sources.size(), bd.directed, nullptr, nullptr);
      if (nc < 0) throw InvalidInputException("Parallel BFS failed: " + GetOnagerError());
      gs.result_sources.resize(nc); gs.result_order.resize(nc);
      int64_t rc = ::onager::onager_compute_bfs_parallel_multi(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.sources.data(), bd.sources.size(), bd.directed, gs.result_sources.data(), gs.result_order.data());
      if (rc != nc) throw InvalidInputException("Parallel BFS failed: " + GetOnagerError());
    } else {
      int64_t nc = ::onager::onager_compute_bfs_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, bd.directed, nullptr);
      if (nc < 0) throw InvalidInputException("Parallel BFS failed: " + GetOnagerError());
      gs.result_order.resize(nc);
      int64_t rc = ::onager::onager_compute_bfs_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, bd.directed, gs.result_order.data());
      if (rc != nc) throw InvalidInputException("Parallel BFS failed: " + GetOnagerError());
    }
    gs.computed = true;
  }
  idx_t rem = gs.result_order.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  if (bd.multi) {
    auto s = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto n = GetFlatVectorDataWritable<int64_t>(output.data[1]);
    for (idx_t i = 0; i < to; i++) { s[i] = gs.result_sources[gs.output_idx+i]; n[i] = gs.result_order[gs.output_idx+i]; }
  } else {
    auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]);
    for (idx_t i = 0; i < to; i++) { n[i] = gs.result_order[gs.output_idx+i]; }
  }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_order.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel Shortest Paths
// =============================================================================

struct ParallelPathsBindData : public TableFunctionData { int64_t source = 0; std::vector<int64_t> sources; bool multi = false; bool directed = false; };
struct ParallelPathsGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_sources, result_nodes;
  std::vector<double> result_distances;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelPathsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<ParallelPathsBindData>();
  CheckInt64Input(input, "onager_par_shortest_paths");
  bool has_source = false;
  for (auto &kv : input.named_parameters) {
    if (kv.first == "source") { bd->source = GetRequiredParam<int64_t>("onager_par_shortest_paths", "source", kv.second); has_source = true; }
    else if (kv.first == "sources") {
      if (kv.second.IsNull()) throw BinderException("onager_par_shortest_paths parameter sources cannot be NULL");
      bd->multi = true;
      for (auto &v : ListValue::GetChildren(kv.second)) {
        if (v.IsNull()) throw BinderException("onager_par_shortest_paths: sources must not contain NULL");
        bd->sources.push_back(v.GetValue<int64_t>());
      }
      if (bd->sources.empty()) throw BinderException("onager_par_shortest_paths: sources must contain at least one node");
    }
    else if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_par_shortest_paths", "directed", kv.second);
  }
  if (bd->multi && has_source) throw BinderException("onager_par_shortest_paths: pass either source or sources, not both");
  if (bd->multi) { rt.push_back(LogicalType::BIGINT); nm.push_back("source"); }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("distance");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> ParallelPathsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelPathsGlobalState>(); }
static OperatorResultType ParallelPathsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelPathsGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_par_shortest_paths");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelPathsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<ParallelPathsBindData>(); auto &gs = data.global_state->Cast<ParallelPathsGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    if (bd.multi) {
      int64_t nc = ::onager::onager_compute_shortest_paths_parallel_multi(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.sources.data(), bd.sources.size(), bd.directed, nullptr, nullptr, nullptr);
      if (nc < 0) throw InvalidInputException("Parallel shortest paths failed: " + GetOnagerError());
      gs.result_sources.resize(nc); gs.result_nodes.resize(nc); gs.result_distances.resize(nc);
      int64_t rc = ::onager::onager_compute_shortest_paths_parallel_multi(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.sources.data(), bd.sources.size(), bd.directed, gs.result_sources.data(), gs.result_nodes.data(), gs.result_distances.data());
      if (rc != nc) throw InvalidInputException("Parallel shortest paths failed: " + GetOnagerError());
    } else {
      int64_t nc = ::onager::onager_compute_shortest_paths_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, bd.directed, nullptr, nullptr);
      if (nc < 0) throw InvalidInputException("Parallel shortest paths failed: " + GetOnagerError());
      gs.result_nodes.resize(nc); gs.result_distances.resize(nc);
      int64_t rc = ::onager::onager_compute_shortest_paths_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, bd.directed, gs.result_nodes.data(), gs.result_distances.data());
      if (rc != nc) throw InvalidInputException("Parallel shortest paths failed: " + GetOnagerError());
    }
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  if (bd.multi) {
    auto s = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto n = GetFlatVectorDataWritable<int64_t>(output.data[1]); auto d = GetFlatVectorDataWritable<double>(output.data[2]);
    for (idx_t i = 0; i < to; i++) { s[i] = gs.result_sources[gs.output_idx+i]; n[i] = gs.result_nodes[gs.output_idx+i]; d[i] = gs.result_distances[gs.output_idx+i]; }
  } else {
    auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto d = GetFlatVectorDataWritable<double>(output.data[1]);
    for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; d[i] = gs.result_distances[gs.output_idx+i]; }
  }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel Connected Components
// =============================================================================

struct ParallelComponentsGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_components;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelComponentsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_par_components");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("component");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> ParallelComponentsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelComponentsGlobalState>(); }
static OperatorResultType ParallelComponentsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelComponentsGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_par_components");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelComponentsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelComponentsGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_components_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel components failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_components.resize(nc);
    int64_t rc = ::onager::onager_compute_components_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_components.data());
    if (rc != nc) throw InvalidInputException("Parallel components failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_components[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel Clustering Coefficients
// =============================================================================

struct ParallelClusteringGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_coefficients;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelClusteringBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_par_clustering");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("coefficient");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> ParallelClusteringInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelClusteringGlobalState>(); }
static OperatorResultType ParallelClusteringInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelClusteringGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_par_clustering");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelClusteringFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelClusteringGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_clustering_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel clustering failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_coefficients.resize(nc);
    int64_t rc = ::onager::onager_compute_clustering_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_coefficients.data());
    if (rc != nc) throw InvalidInputException("Parallel clustering failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_coefficients[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel Triangle Count
// =============================================================================

struct ParallelTrianglesGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_counts;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelTrianglesBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_par_triangles");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("triangles");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> ParallelTrianglesInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelTrianglesGlobalState>(); }
static OperatorResultType ParallelTrianglesInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelTrianglesGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_par_triangles");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelTrianglesFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelTrianglesGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_triangles_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel triangles failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_counts.resize(nc);
    int64_t rc = ::onager::onager_compute_triangles_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_counts.data());
    if (rc != nc) throw InvalidInputException("Parallel triangles failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_counts[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterParallelFunctions(ExtensionLoader &loader) {
  TableFunction par_pr("onager_par_pagerank", {LogicalType::TABLE}, nullptr, ParallelPageRankBind, ParallelPageRankInitGlobal);
  par_pr.in_out_function = ParallelPageRankInOut;
  par_pr.in_out_function_final = ParallelPageRankFinal;
  par_pr.named_parameters["damping"] = LogicalType::DOUBLE;
  par_pr.named_parameters["iterations"] = LogicalType::BIGINT;
  par_pr.named_parameters["tolerance"] = LogicalType::DOUBLE;
  par_pr.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(par_pr);
  loader.RegisterFunction(par_pr);

  TableFunction par_bfs("onager_par_bfs", {LogicalType::TABLE}, nullptr, ParallelBfsBind, ParallelBfsInitGlobal);
  par_bfs.in_out_function = ParallelBfsInOut;
  par_bfs.in_out_function_final = ParallelBfsFinal;
  par_bfs.named_parameters["source"] = LogicalType::BIGINT;
  par_bfs.named_parameters["sources"] = LogicalType::LIST(LogicalType::BIGINT);
  par_bfs.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(par_bfs);
  loader.RegisterFunction(par_bfs);

  TableFunction par_paths("onager_par_shortest_paths", {LogicalType::TABLE}, nullptr, ParallelPathsBind, ParallelPathsInitGlobal);
  par_paths.in_out_function = ParallelPathsInOut;
  par_paths.in_out_function_final = ParallelPathsFinal;
  par_paths.named_parameters["source"] = LogicalType::BIGINT;
  par_paths.named_parameters["sources"] = LogicalType::LIST(LogicalType::BIGINT);
  par_paths.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(par_paths);
  loader.RegisterFunction(par_paths);

  TableFunction par_components("onager_par_components", {LogicalType::TABLE}, nullptr, ParallelComponentsBind, ParallelComponentsInitGlobal);
  par_components.in_out_function = ParallelComponentsInOut;
  par_components.in_out_function_final = ParallelComponentsFinal;
  ONAGER_SET_NO_ORDER(par_components);
  loader.RegisterFunction(par_components);

  TableFunction par_clustering("onager_par_clustering", {LogicalType::TABLE}, nullptr, ParallelClusteringBind, ParallelClusteringInitGlobal);
  par_clustering.in_out_function = ParallelClusteringInOut;
  par_clustering.in_out_function_final = ParallelClusteringFinal;
  ONAGER_SET_NO_ORDER(par_clustering);
  loader.RegisterFunction(par_clustering);

  TableFunction par_triangles("onager_par_triangles", {LogicalType::TABLE}, nullptr, ParallelTrianglesBind, ParallelTrianglesInitGlobal);
  par_triangles.in_out_function = ParallelTrianglesInOut;
  par_triangles.in_out_function_final = ParallelTrianglesFinal;
  ONAGER_SET_NO_ORDER(par_triangles);
  loader.RegisterFunction(par_triangles);
}

} // namespace onager
} // namespace duckdb
