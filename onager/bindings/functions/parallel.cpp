/**
 * @file parallel.cpp
 * @brief Parallel algorithm table functions for Onager DuckDB extension.
 *
 * Parallel PageRank, BFS, Shortest Paths, Connected Components, Clustering Coefficients, Triangle Count.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Parallel PageRank
// =============================================================================

struct ParallelPageRankBindData : public TableFunctionData {
  double damping = 0.85;
  int64_t iterations = 100;
  bool directed = false;
};
struct ParallelPageRankGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_ranks;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelPageRankBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<ParallelPageRankBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_par_pagerank requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "damping") bd->damping = kv.second.GetValue<double>();
    if (kv.first == "iterations") bd->iterations = kv.second.GetValue<int64_t>();
    if (kv.first == "directed") bd->directed = kv.second.GetValue<bool>();
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("rank");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> ParallelPageRankInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelPageRankGlobalState>(); }
static OperatorResultType ParallelPageRankInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelPageRankGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelPageRankFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<ParallelPageRankBindData>(); auto &gs = data.global_state->Cast<ParallelPageRankGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_pagerank_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, 0, bd.damping, bd.iterations, bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel PageRank failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_ranks.resize(nc);
    ::onager::onager_compute_pagerank_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, 0, bd.damping, bd.iterations, bd.directed, gs.result_nodes.data(), gs.result_ranks.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto r = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; r[i] = gs.result_ranks[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel BFS
// =============================================================================

struct ParallelBfsBindData : public TableFunctionData { int64_t source = 0; };
struct ParallelBfsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_order;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelBfsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<ParallelBfsBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_par_bfs requires 2 columns");
  for (auto &kv : input.named_parameters) if (kv.first == "source") bd->source = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> ParallelBfsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelBfsGlobalState>(); }
static OperatorResultType ParallelBfsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelBfsGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelBfsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<ParallelBfsBindData>(); auto &gs = data.global_state->Cast<ParallelBfsGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_bfs_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel BFS failed: " + GetOnagerError());
    gs.result_order.resize(nc);
    ::onager::onager_compute_bfs_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, gs.result_order.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_order.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_order[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_order.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel Shortest Paths
// =============================================================================

struct ParallelPathsBindData : public TableFunctionData { int64_t source = 0; };
struct ParallelPathsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_distances;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelPathsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<ParallelPathsBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_par_shortest_paths requires 2 columns");
  for (auto &kv : input.named_parameters) if (kv.first == "source") bd->source = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("distance");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> ParallelPathsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelPathsGlobalState>(); }
static OperatorResultType ParallelPathsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelPathsGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelPathsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<ParallelPathsBindData>(); auto &gs = data.global_state->Cast<ParallelPathsGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_shortest_paths_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel shortest paths failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_distances.resize(nc);
    ::onager::onager_compute_shortest_paths_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, gs.result_nodes.data(), gs.result_distances.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto d = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; d[i] = gs.result_distances[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel Connected Components
// =============================================================================

struct ParallelComponentsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_components;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelComponentsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_par_components requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("component");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> ParallelComponentsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelComponentsGlobalState>(); }
static OperatorResultType ParallelComponentsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelComponentsGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelComponentsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelComponentsGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_components_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel components failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_components.resize(nc);
    ::onager::onager_compute_components_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_components.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_components[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel Clustering Coefficients
// =============================================================================

struct ParallelClusteringGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_coefficients;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelClusteringBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_par_clustering requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("coefficient");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> ParallelClusteringInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelClusteringGlobalState>(); }
static OperatorResultType ParallelClusteringInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelClusteringGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelClusteringFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelClusteringGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_clustering_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel clustering failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_coefficients.resize(nc);
    ::onager::onager_compute_clustering_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_coefficients.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_coefficients[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Parallel Triangle Count
// =============================================================================

struct ParallelTrianglesGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_counts;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ParallelTrianglesBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_par_triangles requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("triangles");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> ParallelTrianglesInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ParallelTrianglesGlobalState>(); }
static OperatorResultType ParallelTrianglesInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelTrianglesGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ParallelTrianglesFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ParallelTrianglesGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_triangles_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Parallel triangles failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_counts.resize(nc);
    ::onager::onager_compute_triangles_parallel(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_counts.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_counts[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
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
  par_pr.named_parameters["directed"] = LogicalType::BOOLEAN;
  loader.RegisterFunction(par_pr);

  TableFunction par_bfs("onager_par_bfs", {LogicalType::TABLE}, nullptr, ParallelBfsBind, ParallelBfsInitGlobal);
  par_bfs.in_out_function = ParallelBfsInOut;
  par_bfs.in_out_function_final = ParallelBfsFinal;
  par_bfs.named_parameters["source"] = LogicalType::BIGINT;
  loader.RegisterFunction(par_bfs);

  TableFunction par_paths("onager_par_shortest_paths", {LogicalType::TABLE}, nullptr, ParallelPathsBind, ParallelPathsInitGlobal);
  par_paths.in_out_function = ParallelPathsInOut;
  par_paths.in_out_function_final = ParallelPathsFinal;
  par_paths.named_parameters["source"] = LogicalType::BIGINT;
  loader.RegisterFunction(par_paths);

  TableFunction par_components("onager_par_components", {LogicalType::TABLE}, nullptr, ParallelComponentsBind, ParallelComponentsInitGlobal);
  par_components.in_out_function = ParallelComponentsInOut;
  par_components.in_out_function_final = ParallelComponentsFinal;
  loader.RegisterFunction(par_components);

  TableFunction par_clustering("onager_par_clustering", {LogicalType::TABLE}, nullptr, ParallelClusteringBind, ParallelClusteringInitGlobal);
  par_clustering.in_out_function = ParallelClusteringInOut;
  par_clustering.in_out_function_final = ParallelClusteringFinal;
  loader.RegisterFunction(par_clustering);

  TableFunction par_triangles("onager_par_triangles", {LogicalType::TABLE}, nullptr, ParallelTrianglesBind, ParallelTrianglesInitGlobal);
  par_triangles.in_out_function = ParallelTrianglesInOut;
  par_triangles.in_out_function_final = ParallelTrianglesFinal;
  loader.RegisterFunction(par_triangles);
}

} // namespace onager
} // namespace duckdb
