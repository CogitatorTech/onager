/**
 * @file traversal.cpp
 * @brief Traversal and path table functions for Onager DuckDB extension.
 *
 * Dijkstra, BFS, DFS, Bellman-Ford, Floyd-Warshall.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Dijkstra Shortest Paths
// =============================================================================

struct DijkstraBindData : public TableFunctionData { int64_t source = 0; };
struct DijkstraGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_distances;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DijkstraBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<DijkstraBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_dijkstra requires 2 columns");
  for (auto &kv : input.named_parameters) if (kv.first == "source") bd->source = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("distance");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> DijkstraInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<DijkstraGlobalState>(); }
static OperatorResultType DijkstraInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<DijkstraGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType DijkstraFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<DijkstraBindData>(); auto &gs = data.global_state->Cast<DijkstraGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_dijkstra(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Dijkstra failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_distances.resize(nc);
    ::onager::onager_compute_dijkstra(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, gs.result_nodes.data(), gs.result_distances.data());
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
// BFS Traversal
// =============================================================================

struct BfsBindData : public TableFunctionData { int64_t source = 0; };
struct BfsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_order;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> BfsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<BfsBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_bfs requires 2 columns");
  for (auto &kv : input.named_parameters) if (kv.first == "source") bd->source = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> BfsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<BfsGlobalState>(); }
static OperatorResultType BfsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<BfsGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType BfsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<BfsBindData>(); auto &gs = data.global_state->Cast<BfsGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_bfs(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, nullptr);
    if (nc < 0) throw InvalidInputException("BFS failed: " + GetOnagerError());
    gs.result_order.resize(nc);
    ::onager::onager_compute_bfs(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, gs.result_order.data());
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
// DFS Traversal
// =============================================================================

struct DfsBindData : public TableFunctionData { int64_t source = 0; };
struct DfsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_order;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DfsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<DfsBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_dfs requires 2 columns");
  for (auto &kv : input.named_parameters) if (kv.first == "source") bd->source = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> DfsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<DfsGlobalState>(); }
static OperatorResultType DfsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<DfsGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType DfsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<DfsBindData>(); auto &gs = data.global_state->Cast<DfsGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_dfs(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, nullptr);
    if (nc < 0) throw InvalidInputException("DFS failed: " + GetOnagerError());
    gs.result_order.resize(nc);
    ::onager::onager_compute_dfs(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.source, gs.result_order.data());
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
// Bellman-Ford Shortest Paths (weighted)
// =============================================================================

struct BellmanFordBindData : public TableFunctionData { int64_t source = 0; };
struct BellmanFordGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> weights, result_distances;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> BellmanFordBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<BellmanFordBindData>();
  if (input.input_table_types.size() < 3) throw InvalidInputException("onager_bellman_ford requires 3 columns (src, dst, weight)");
  for (auto &kv : input.named_parameters) if (kv.first == "source") bd->source = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("distance");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> BellmanFordInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<BellmanFordGlobalState>(); }
static OperatorResultType BellmanFordInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<BellmanFordGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  auto w = FlatVector::GetData<double>(input.data[2]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); gs.weights.push_back(w[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType BellmanFordFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<BellmanFordBindData>(); auto &gs = data.global_state->Cast<BellmanFordGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_bellman_ford(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), bd.source, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Bellman-Ford failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_distances.resize(nc);
    ::onager::onager_compute_bellman_ford(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), bd.source, gs.result_nodes.data(), gs.result_distances.data());
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
// Floyd-Warshall All-Pairs Shortest Paths
// =============================================================================

struct FloydWarshallGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_src, result_dst;
  std::vector<double> weights, result_distances;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> FloydWarshallBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 3) throw InvalidInputException("onager_floyd_warshall requires 3 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("src");
  rt.push_back(LogicalType::BIGINT); nm.push_back("dst");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("distance");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> FloydWarshallInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<FloydWarshallGlobalState>(); }
static OperatorResultType FloydWarshallInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<FloydWarshallGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  auto w = FlatVector::GetData<double>(input.data[2]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); gs.weights.push_back(w[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType FloydWarshallFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<FloydWarshallGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_floyd_warshall(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Floyd-Warshall failed: " + GetOnagerError());
    gs.result_src.resize(nc); gs.result_dst.resize(nc); gs.result_distances.resize(nc);
    ::onager::onager_compute_floyd_warshall(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), gs.result_src.data(), gs.result_dst.data(), gs.result_distances.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_src.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto s = FlatVector::GetData<int64_t>(output.data[0]); auto d = FlatVector::GetData<int64_t>(output.data[1]); auto w = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { s[i] = gs.result_src[gs.output_idx+i]; d[i] = gs.result_dst[gs.output_idx+i]; w[i] = gs.result_distances[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_src.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterTraversalFunctions(ExtensionLoader &loader) {
  TableFunction dijkstra("onager_pth_dijkstra", {LogicalType::TABLE}, nullptr, DijkstraBind, DijkstraInitGlobal);
  dijkstra.in_out_function = DijkstraInOut;
  dijkstra.in_out_function_final = DijkstraFinal;
  dijkstra.named_parameters["source"] = LogicalType::BIGINT;
  loader.RegisterFunction(dijkstra);

  TableFunction bfs("onager_trv_bfs", {LogicalType::TABLE}, nullptr, BfsBind, BfsInitGlobal);
  bfs.in_out_function = BfsInOut;
  bfs.in_out_function_final = BfsFinal;
  bfs.named_parameters["source"] = LogicalType::BIGINT;
  loader.RegisterFunction(bfs);

  TableFunction dfs("onager_trv_dfs", {LogicalType::TABLE}, nullptr, DfsBind, DfsInitGlobal);
  dfs.in_out_function = DfsInOut;
  dfs.in_out_function_final = DfsFinal;
  dfs.named_parameters["source"] = LogicalType::BIGINT;
  loader.RegisterFunction(dfs);

  TableFunction bellman_ford("onager_pth_bellman_ford", {LogicalType::TABLE}, nullptr, BellmanFordBind, BellmanFordInitGlobal);
  bellman_ford.in_out_function = BellmanFordInOut;
  bellman_ford.in_out_function_final = BellmanFordFinal;
  bellman_ford.named_parameters["source"] = LogicalType::BIGINT;
  loader.RegisterFunction(bellman_ford);

  TableFunction floyd_warshall("onager_pth_floyd_warshall", {LogicalType::TABLE}, nullptr, FloydWarshallBind, FloydWarshallInitGlobal);
  floyd_warshall.in_out_function = FloydWarshallInOut;
  floyd_warshall.in_out_function_final = FloydWarshallFinal;
  loader.RegisterFunction(floyd_warshall);
}

} // namespace onager
} // namespace duckdb
