/**
 * @file subgraphs.cpp
 * @brief Subgraph extraction table functions for Onager DuckDB extension.
 *
 * Ego Graph, K-Hop Neighbors, Induced Subgraph.
 */
#include "functions.hpp"
#include <mutex>

namespace duckdb {

using namespace onager;

// =============================================================================
// Ego Graph
// =============================================================================

struct EgoGraphBindData : public TableFunctionData { int64_t center = 0; int64_t radius = 1; };
struct EgoGraphGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_src, result_dst;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> EgoGraphBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<EgoGraphBindData>();
  CheckInt64Input(input, "onager_sub_ego_graph");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "center") bd->center = GetRequiredParam<int64_t>("onager_sub_ego_graph", kv.first, kv.second);
    if (kv.first == "radius") bd->radius = GetNonNegativeParam("onager_sub_ego_graph", kv.first, kv.second);
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("src");
  rt.push_back(LogicalType::BIGINT); nm.push_back("dst");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> EgoGraphInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<EgoGraphGlobalState>(); }
static OperatorResultType EgoGraphInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<EgoGraphGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_sub_ego_graph");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType EgoGraphFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<EgoGraphBindData>(); auto &gs = data.global_state->Cast<EgoGraphGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_ego_graph(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.center, bd.radius, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Ego graph failed: " + GetOnagerError());
    gs.result_src.resize(nc); gs.result_dst.resize(nc);
    int64_t rc = ::onager::onager_compute_ego_graph(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.center, bd.radius, gs.result_src.data(), gs.result_dst.data());
    if (rc != nc) throw InvalidInputException("Ego graph failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_src.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto s = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto d = GetFlatVectorDataWritable<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { s[i] = gs.result_src[gs.output_idx+i]; d[i] = gs.result_dst[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_src.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// K-Hop Neighbors
// =============================================================================

struct KHopBindData : public TableFunctionData { int64_t start = 0; int64_t k = 1; };
struct KHopGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> KHopBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<KHopBindData>();
  CheckInt64Input(input, "onager_sub_k_hop");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "start") bd->start = GetRequiredParam<int64_t>("onager_sub_k_hop", kv.first, kv.second);
    if (kv.first == "k") bd->k = GetNonNegativeParam("onager_sub_k_hop", kv.first, kv.second);
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> KHopInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<KHopGlobalState>(); }
static OperatorResultType KHopInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<KHopGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_sub_k_hop");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType KHopFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<KHopBindData>(); auto &gs = data.global_state->Cast<KHopGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_k_hop_neighbors(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.start, bd.k, nullptr);
    if (nc < 0) throw InvalidInputException("K-hop neighbors failed: " + GetOnagerError());
    gs.result_nodes.resize(nc);
    int64_t rc = ::onager::onager_compute_k_hop_neighbors(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.start, bd.k, gs.result_nodes.data());
    if (rc != nc) throw InvalidInputException("K-hop neighbors failed: " + GetOnagerError());
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
// Induced Subgraph
// =============================================================================

struct InducedSubgraphGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, filter_nodes, result_src, result_dst;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> InducedSubgraphBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_sub_induced", 3);
  CheckColumnType(input, "onager_sub_induced", 2, LogicalType::BIGINT);
  rt.push_back(LogicalType::BIGINT); nm.push_back("src");
  rt.push_back(LogicalType::BIGINT); nm.push_back("dst");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> InducedSubgraphInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<InducedSubgraphGlobalState>(); }
static OperatorResultType InducedSubgraphInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<InducedSubgraphGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  UnifiedVectorFormat s_data, d_data, f_data;
  ONAGER_TO_UNIFIED_FORMAT(input.data[0], input.size(), s_data);
  ONAGER_TO_UNIFIED_FORMAT(input.data[1], input.size(), d_data);
  ONAGER_TO_UNIFIED_FORMAT(input.data[2], input.size(), f_data);
  auto s = UnifiedVectorFormat::GetData<int64_t>(s_data);
  auto d = UnifiedVectorFormat::GetData<int64_t>(d_data);
  auto f = UnifiedVectorFormat::GetData<int64_t>(f_data);
  for (idx_t i = 0; i < input.size(); i++) {
    auto s_idx = s_data.sel->get_index(i);
    auto d_idx = d_data.sel->get_index(i);
    auto f_idx = f_data.sel->get_index(i);
    if (!s_data.validity.RowIsValid(s_idx) || !d_data.validity.RowIsValid(d_idx)) {
      throw InvalidInputException("onager_sub_induced does not accept NULL edge endpoints");
    }
    gs.src_nodes.push_back(s[s_idx]);
    gs.dst_nodes.push_back(d[d_idx]);
    if (f_data.validity.RowIsValid(f_idx)) {
      gs.filter_nodes.push_back(f[f_idx]);
    }
  }
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType InducedSubgraphFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<InducedSubgraphGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty() || gs.filter_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_induced_subgraph(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.filter_nodes.data(), gs.filter_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Induced subgraph failed: " + GetOnagerError());
    gs.result_src.resize(nc); gs.result_dst.resize(nc);
    int64_t rc = ::onager::onager_compute_induced_subgraph(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.filter_nodes.data(), gs.filter_nodes.size(), gs.result_src.data(), gs.result_dst.data());
    if (rc != nc) throw InvalidInputException("Induced subgraph failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_src.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto s = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto d = GetFlatVectorDataWritable<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { s[i] = gs.result_src[gs.output_idx+i]; d[i] = gs.result_dst[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_src.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterSubgraphFunctions(ExtensionLoader &loader) {
  TableFunction ego_graph("onager_sub_ego_graph", {LogicalType::TABLE}, nullptr, EgoGraphBind, EgoGraphInitGlobal);
  ego_graph.in_out_function = EgoGraphInOut;
  ego_graph.in_out_function_final = EgoGraphFinal;
  ego_graph.named_parameters["center"] = LogicalType::BIGINT;
  ego_graph.named_parameters["radius"] = LogicalType::BIGINT;
  ONAGER_SET_NO_ORDER(ego_graph);
  loader.RegisterFunction(ego_graph);

  TableFunction k_hop("onager_sub_k_hop", {LogicalType::TABLE}, nullptr, KHopBind, KHopInitGlobal);
  k_hop.in_out_function = KHopInOut;
  k_hop.in_out_function_final = KHopFinal;
  k_hop.named_parameters["start"] = LogicalType::BIGINT;
  k_hop.named_parameters["k"] = LogicalType::BIGINT;
  ONAGER_SET_NO_ORDER(k_hop);
  loader.RegisterFunction(k_hop);

  TableFunction induced("onager_sub_induced", {LogicalType::TABLE}, nullptr, InducedSubgraphBind, InducedSubgraphInitGlobal);
  induced.in_out_function = InducedSubgraphInOut;
  induced.in_out_function_final = InducedSubgraphFinal;
  ONAGER_SET_NO_ORDER(induced);
  loader.RegisterFunction(induced);
}

} // namespace onager
} // namespace duckdb
