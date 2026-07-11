/**
 * @file mst.cpp
 * @brief Minimum Spanning Tree table functions for Onager DuckDB extension.
 *
 * Kruskal's MST algorithm.
 */
#include "functions.hpp"
#include <mutex>

namespace duckdb {

using namespace onager;

// =============================================================================
// Kruskal MST
// =============================================================================

struct KruskalMstGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_src, result_dst;
  std::vector<double> weights, result_weights;
  double total_weight = 0.0;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> KruskalMstBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_mst_kruskal", 3);
  CheckColumnType(input, "onager_mst_kruskal", 2, LogicalType::DOUBLE);
  rt.push_back(LogicalType::BIGINT); nm.push_back("src");
  rt.push_back(LogicalType::BIGINT); nm.push_back("dst");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("weight");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> KruskalMstInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<KruskalMstGlobalState>(); }
static OperatorResultType KruskalMstInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<KruskalMstGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendWeightedEdges(input, gs.src_nodes, gs.dst_nodes, gs.weights, "onager_mst_kruskal");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType KruskalMstFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<KruskalMstGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t ec = ::onager::onager_compute_kruskal_mst(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr, nullptr);
    if (ec < 0) throw InvalidInputException("Kruskal MST failed: " + GetOnagerError());
    gs.result_src.resize(ec); gs.result_dst.resize(ec); gs.result_weights.resize(ec);
    int64_t rc = ::onager::onager_compute_kruskal_mst(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), gs.result_src.data(), gs.result_dst.data(), gs.result_weights.data(), &gs.total_weight);
    if (rc != ec) throw InvalidInputException("Kruskal MST failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_src.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto s = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto d = GetFlatVectorDataWritable<int64_t>(output.data[1]); auto w = GetFlatVectorDataWritable<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { s[i] = gs.result_src[gs.output_idx+i]; d[i] = gs.result_dst[gs.output_idx+i]; w[i] = gs.result_weights[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_src.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Prim MST
// =============================================================================

struct PrimMstGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_src, result_dst;
  std::vector<double> weights, result_weights;
  double total_weight = 0.0;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> PrimMstBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_mst_prim", 3);
  CheckColumnType(input, "onager_mst_prim", 2, LogicalType::DOUBLE);
  rt.push_back(LogicalType::BIGINT); nm.push_back("src");
  rt.push_back(LogicalType::BIGINT); nm.push_back("dst");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("weight");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> PrimMstInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<PrimMstGlobalState>(); }
static OperatorResultType PrimMstInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<PrimMstGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendWeightedEdges(input, gs.src_nodes, gs.dst_nodes, gs.weights, "onager_mst_prim");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType PrimMstFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<PrimMstGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t ec = ::onager::onager_compute_prim_mst(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr, nullptr);
    if (ec < 0) throw InvalidInputException("Prim MST failed: " + GetOnagerError());
    gs.result_src.resize(ec); gs.result_dst.resize(ec); gs.result_weights.resize(ec);
    int64_t rc = ::onager::onager_compute_prim_mst(gs.src_nodes.data(), gs.dst_nodes.data(), gs.weights.data(), gs.src_nodes.size(), gs.result_src.data(), gs.result_dst.data(), gs.result_weights.data(), &gs.total_weight);
    if (rc != ec) throw InvalidInputException("Prim MST failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_src.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto s = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto d = GetFlatVectorDataWritable<int64_t>(output.data[1]); auto w = GetFlatVectorDataWritable<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { s[i] = gs.result_src[gs.output_idx+i]; d[i] = gs.result_dst[gs.output_idx+i]; w[i] = gs.result_weights[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
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
  ONAGER_SET_NO_ORDER(kruskal);
  loader.RegisterFunction(kruskal);

  TableFunction prim("onager_mst_prim", {LogicalType::TABLE}, nullptr, PrimMstBind, PrimMstInitGlobal);
  prim.in_out_function = PrimMstInOut;
  prim.in_out_function_final = PrimMstFinal;
  ONAGER_SET_NO_ORDER(prim);
  loader.RegisterFunction(prim);
}

} // namespace onager
} // namespace duckdb
