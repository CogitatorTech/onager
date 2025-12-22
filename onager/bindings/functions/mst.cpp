/**
 * @file mst.cpp
 * @brief Minimum Spanning Tree table functions for Onager DuckDB extension.
 *
 * Kruskal's MST algorithm.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Kruskal MST
// =============================================================================

struct KruskalMstGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_src, result_dst;
  std::vector<double> weights, result_weights;
  double total_weight = 0.0;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> KruskalMstBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 3) throw InvalidInputException("onager_mst_kruskal requires 3 columns (src, dst, weight)");
  rt.push_back(LogicalType::BIGINT); nm.push_back("src");
  rt.push_back(LogicalType::BIGINT); nm.push_back("dst");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("weight");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> KruskalMstInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<KruskalMstGlobalState>(); }
static OperatorResultType KruskalMstInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<KruskalMstGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  auto w = FlatVector::GetData<double>(input.data[2]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); gs.weights.push_back(w[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType KruskalMstFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<KruskalMstGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t ec = ::onager::onager_compute_kruskal_mst(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr, nullptr);
    if (ec < 0) throw InvalidInputException("Kruskal MST failed: " + GetOnagerError());
    gs.result_src.resize(ec); gs.result_dst.resize(ec); gs.result_weights.resize(ec);
    ::onager::onager_compute_kruskal_mst(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), gs.result_src.data(), gs.result_dst.data(), gs.result_weights.data(), &gs.total_weight);
    gs.computed = true;
  }
  idx_t rem = gs.result_src.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto s = FlatVector::GetData<int64_t>(output.data[0]); auto d = FlatVector::GetData<int64_t>(output.data[1]); auto w = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { s[i] = gs.result_src[gs.output_idx+i]; d[i] = gs.result_dst[gs.output_idx+i]; w[i] = gs.result_weights[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_src.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterMstFunctions(ExtensionLoader &loader) {
  TableFunction kruskal("onager_mst_kruskal", {LogicalType::TABLE}, nullptr, KruskalMstBind, KruskalMstInitGlobal);
  kruskal.in_out_function = KruskalMstInOut;
  kruskal.in_out_function_final = KruskalMstFinal;
  loader.RegisterFunction(kruskal);
}

} // namespace onager
} // namespace duckdb
