/**
 * @file approximation.cpp
 * @brief Approximation algorithm table functions for Onager DuckDB extension.
 *
 * Maximum Clique, Independent Set, Vertex Cover approximations.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Maximum Clique Approximation
// =============================================================================

struct MaxCliqueGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> MaxCliqueBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_max_clique requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> MaxCliqueInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<MaxCliqueGlobalState>(); }
static OperatorResultType MaxCliqueInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<MaxCliqueGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType MaxCliqueFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<MaxCliqueGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_max_clique(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr);
    if (nc < 0) throw InvalidInputException("Max clique failed: " + GetOnagerError());
    gs.result_nodes.resize(nc);
    ::onager::onager_compute_max_clique(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Independent Set Approximation
// =============================================================================

struct IndependentSetGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> IndependentSetBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_independent_set requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> IndependentSetInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<IndependentSetGlobalState>(); }
static OperatorResultType IndependentSetInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<IndependentSetGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType IndependentSetFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<IndependentSetGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_independent_set(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr);
    if (nc < 0) throw InvalidInputException("Independent set failed: " + GetOnagerError());
    gs.result_nodes.resize(nc);
    ::onager::onager_compute_independent_set(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Vertex Cover Approximation
// =============================================================================

struct VertexCoverGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> VertexCoverBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_vertex_cover requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> VertexCoverInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<VertexCoverGlobalState>(); }
static OperatorResultType VertexCoverInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<VertexCoverGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType VertexCoverFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<VertexCoverGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_vertex_cover(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr);
    if (nc < 0) throw InvalidInputException("Vertex cover failed: " + GetOnagerError());
    gs.result_nodes.resize(nc);
    ::onager::onager_compute_vertex_cover(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// TSP Approximation
// =============================================================================

struct TspGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_tour;
  std::vector<double> weights;
  double result_cost = 0.0;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> TspBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 3) throw InvalidInputException("onager_tsp requires 3 columns: (src, dst, weight)");
  rt.push_back(LogicalType::BIGINT); nm.push_back("order");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> TspInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<TspGlobalState>(); }
static OperatorResultType TspInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<TspGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  auto w = FlatVector::GetData<double>(input.data[2]);
  for (idx_t i = 0; i < input.size(); i++) {
    gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); gs.weights.push_back(w[i]);
  }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType TspFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<TspGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_tsp(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("TSP failed: " + GetOnagerError());
    gs.result_tour.resize(nc);
    ::onager::onager_compute_tsp(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), gs.result_tour.data(), &gs.result_cost);
    gs.computed = true;
  }
  idx_t rem = gs.result_tour.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto ord = FlatVector::GetData<int64_t>(output.data[0]); auto n = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { ord[i] = static_cast<int64_t>(gs.output_idx + i); n[i] = gs.result_tour[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
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
  loader.RegisterFunction(max_clique);

  TableFunction independent_set("onager_apx_independent_set", {LogicalType::TABLE}, nullptr, IndependentSetBind, IndependentSetInitGlobal);
  independent_set.in_out_function = IndependentSetInOut;
  independent_set.in_out_function_final = IndependentSetFinal;
  loader.RegisterFunction(independent_set);

  TableFunction vertex_cover("onager_apx_vertex_cover", {LogicalType::TABLE}, nullptr, VertexCoverBind, VertexCoverInitGlobal);
  vertex_cover.in_out_function = VertexCoverInOut;
  vertex_cover.in_out_function_final = VertexCoverFinal;
  loader.RegisterFunction(vertex_cover);

  TableFunction tsp("onager_apx_tsp", {LogicalType::TABLE}, nullptr, TspBind, TspInitGlobal);
  tsp.in_out_function = TspInOut;
  tsp.in_out_function_final = TspFinal;
  loader.RegisterFunction(tsp);
}

} // namespace onager
} // namespace duckdb
