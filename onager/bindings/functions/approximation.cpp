/**
 * @file approximation.cpp
 * @brief Approximation algorithm table functions for Onager DuckDB extension.
 *
 * Maximum Clique, Independent Set, Vertex Cover approximations.
 */
#include "functions.hpp"
#include <mutex>
#include <unordered_set>

namespace duckdb {

using namespace onager;

// =============================================================================
// Maximum Clique Approximation
// =============================================================================

struct MaxCliqueGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> MaxCliqueBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_apx_max_clique");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> MaxCliqueInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<MaxCliqueGlobalState>(); }
static OperatorResultType MaxCliqueInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<MaxCliqueGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_apx_max_clique");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType MaxCliqueFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<MaxCliqueGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    // One FFI call with a buffer sized to the unique node count. The result
    // set can never exceed it, and a separate sizing call could return a
    // different count when the algorithm breaks ties nondeterministically.
    std::unordered_set<int64_t> unique_nodes(gs.src_nodes.begin(), gs.src_nodes.end());
    unique_nodes.insert(gs.dst_nodes.begin(), gs.dst_nodes.end());
    gs.result_nodes.resize(unique_nodes.size());
    int64_t nc = ::onager::onager_compute_max_clique(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data());
    if (nc < 0) throw InvalidInputException("Max clique failed: " + GetOnagerError());
    gs.result_nodes.resize(nc);
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Independent Set Approximation
// =============================================================================

struct IndependentSetGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> IndependentSetBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_apx_independent_set");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> IndependentSetInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<IndependentSetGlobalState>(); }
static OperatorResultType IndependentSetInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<IndependentSetGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_apx_independent_set");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType IndependentSetFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<IndependentSetGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    // One FFI call with a buffer sized to the unique node count. The result
    // set can never exceed it, and a separate sizing call could return a
    // different count when the algorithm breaks ties nondeterministically.
    std::unordered_set<int64_t> unique_nodes(gs.src_nodes.begin(), gs.src_nodes.end());
    unique_nodes.insert(gs.dst_nodes.begin(), gs.dst_nodes.end());
    gs.result_nodes.resize(unique_nodes.size());
    int64_t nc = ::onager::onager_compute_independent_set(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data());
    if (nc < 0) throw InvalidInputException("Independent set failed: " + GetOnagerError());
    gs.result_nodes.resize(nc);
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Vertex Cover Approximation
// =============================================================================

struct VertexCoverGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> VertexCoverBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_apx_vertex_cover");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> VertexCoverInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<VertexCoverGlobalState>(); }
static OperatorResultType VertexCoverInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<VertexCoverGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_apx_vertex_cover");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType VertexCoverFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<VertexCoverGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    // One FFI call with a buffer sized to the unique node count. The result
    // set can never exceed it, and a separate sizing call could return a
    // different count when the algorithm breaks ties nondeterministically.
    std::unordered_set<int64_t> unique_nodes(gs.src_nodes.begin(), gs.src_nodes.end());
    unique_nodes.insert(gs.dst_nodes.begin(), gs.dst_nodes.end());
    gs.result_nodes.resize(unique_nodes.size());
    int64_t nc = ::onager::onager_compute_vertex_cover(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data());
    if (nc < 0) throw InvalidInputException("Vertex cover failed: " + GetOnagerError());
    gs.result_nodes.resize(nc);
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// TSP Approximation
// =============================================================================

struct TspGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_tour;
  std::vector<double> weights;
  double result_cost = 0.0;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

struct TspBindData : public TableFunctionData { int64_t start = 0; bool has_start = false; };

static unique_ptr<FunctionData> TspBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<TspBindData>();
  CheckInt64Input(input, "onager_apx_tsp", 3);
  CheckColumnType(input, "onager_apx_tsp", 2, LogicalType::DOUBLE);
  for (auto &kv : input.named_parameters) {
    if (kv.first == "start") { bd->start = GetRequiredParam<int64_t>("onager_apx_tsp", kv.first, kv.second); bd->has_start = true; }
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("order");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> TspInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<TspGlobalState>(); }
static OperatorResultType TspInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<TspGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendWeightedEdges(input, gs.src_nodes, gs.dst_nodes, gs.weights, "onager_apx_tsp");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType TspFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<TspBindData>();
  auto &gs = data.global_state->Cast<TspGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    // One FFI call with a buffer sized to the unique node count plus one:
    // a tour visits each node at most once and may close back to the start.
    std::unordered_set<int64_t> unique_nodes(gs.src_nodes.begin(), gs.src_nodes.end());
    unique_nodes.insert(gs.dst_nodes.begin(), gs.dst_nodes.end());
    gs.result_tour.resize(unique_nodes.size() + 1);
    int64_t nc = ::onager::onager_compute_tsp(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), bd.start, bd.has_start, gs.result_tour.data(), &gs.result_cost);
    if (nc < 0) throw InvalidInputException("TSP failed: " + GetOnagerError());
    gs.result_tour.resize(nc);
    gs.computed = true;
  }
  idx_t rem = gs.result_tour.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto ord = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto n = GetFlatVectorDataWritable<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { ord[i] = static_cast<int64_t>(gs.output_idx + i); n[i] = gs.result_tour[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_tour.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterApproximationFunctions(ExtensionLoader &loader) {
  TableFunction max_clique("onager_apx_max_clique", {LogicalType::TABLE}, nullptr, MaxCliqueBind, MaxCliqueInitGlobal);
  max_clique.in_out_function = MaxCliqueInOut;
  max_clique.in_out_function_final = MaxCliqueFinal;
  ONAGER_SET_NO_ORDER(max_clique);
  loader.RegisterFunction(max_clique);

  TableFunction independent_set("onager_apx_independent_set", {LogicalType::TABLE}, nullptr, IndependentSetBind, IndependentSetInitGlobal);
  independent_set.in_out_function = IndependentSetInOut;
  independent_set.in_out_function_final = IndependentSetFinal;
  ONAGER_SET_NO_ORDER(independent_set);
  loader.RegisterFunction(independent_set);

  TableFunction vertex_cover("onager_apx_vertex_cover", {LogicalType::TABLE}, nullptr, VertexCoverBind, VertexCoverInitGlobal);
  vertex_cover.in_out_function = VertexCoverInOut;
  vertex_cover.in_out_function_final = VertexCoverFinal;
  ONAGER_SET_NO_ORDER(vertex_cover);
  loader.RegisterFunction(vertex_cover);

  TableFunction tsp("onager_apx_tsp", {LogicalType::TABLE}, nullptr, TspBind, TspInitGlobal);
  tsp.in_out_function = TspInOut;
  tsp.in_out_function_final = TspFinal;
  tsp.named_parameters["start"] = LogicalType::BIGINT;
  ONAGER_SET_NO_ORDER(tsp);
  loader.RegisterFunction(tsp);
}

} // namespace onager
} // namespace duckdb
