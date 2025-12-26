/**
 * @file links.cpp
 * @brief Link prediction table functions for Onager DuckDB extension.
 *
 * Jaccard, Adamic-Adar, Preferential Attachment, Resource Allocation, Common Neighbors.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Jaccard Coefficient
// =============================================================================

struct JaccardGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_n1, result_n2;
  std::vector<double> result_scores;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> JaccardBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_lnk_jaccard");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node1");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node2");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("coefficient");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> JaccardInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<JaccardGlobalState>(); }
static OperatorResultType JaccardInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<JaccardGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType JaccardFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<JaccardGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_jaccard(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Jaccard failed: " + GetOnagerError());
    gs.result_n1.resize(nc); gs.result_n2.resize(nc); gs.result_scores.resize(nc);
    ::onager::onager_compute_jaccard(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_n1.data(), gs.result_n2.data(), gs.result_scores.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_n1.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n1 = FlatVector::GetData<int64_t>(output.data[0]); auto n2 = FlatVector::GetData<int64_t>(output.data[1]); auto sc = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { n1[i] = gs.result_n1[gs.output_idx+i]; n2[i] = gs.result_n2[gs.output_idx+i]; sc[i] = gs.result_scores[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_n1.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Adamic-Adar Index
// =============================================================================

struct AdamicAdarGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_n1, result_n2;
  std::vector<double> result_scores;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> AdamicAdarBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_lnk_adamic_adar");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node1");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node2");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("score");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> AdamicAdarInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<AdamicAdarGlobalState>(); }
static OperatorResultType AdamicAdarInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AdamicAdarGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType AdamicAdarFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<AdamicAdarGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_adamic_adar(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Adamic-Adar failed: " + GetOnagerError());
    gs.result_n1.resize(nc); gs.result_n2.resize(nc); gs.result_scores.resize(nc);
    ::onager::onager_compute_adamic_adar(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_n1.data(), gs.result_n2.data(), gs.result_scores.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_n1.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n1 = FlatVector::GetData<int64_t>(output.data[0]); auto n2 = FlatVector::GetData<int64_t>(output.data[1]); auto sc = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { n1[i] = gs.result_n1[gs.output_idx+i]; n2[i] = gs.result_n2[gs.output_idx+i]; sc[i] = gs.result_scores[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_n1.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Preferential Attachment
// =============================================================================

struct PrefAttachGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_n1, result_n2;
  std::vector<double> result_scores;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> PrefAttachBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_lnk_pref_attach");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node1");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node2");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("score");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> PrefAttachInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<PrefAttachGlobalState>(); }
static OperatorResultType PrefAttachInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<PrefAttachGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType PrefAttachFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<PrefAttachGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_preferential_attachment(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Preferential Attachment failed: " + GetOnagerError());
    gs.result_n1.resize(nc); gs.result_n2.resize(nc); gs.result_scores.resize(nc);
    ::onager::onager_compute_preferential_attachment(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_n1.data(), gs.result_n2.data(), gs.result_scores.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_n1.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n1 = FlatVector::GetData<int64_t>(output.data[0]); auto n2 = FlatVector::GetData<int64_t>(output.data[1]); auto sc = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { n1[i] = gs.result_n1[gs.output_idx+i]; n2[i] = gs.result_n2[gs.output_idx+i]; sc[i] = gs.result_scores[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_n1.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Resource Allocation
// =============================================================================

struct ResourceAllocGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_n1, result_n2;
  std::vector<double> result_scores;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ResourceAllocBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_lnk_resource_alloc");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node1");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node2");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("score");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> ResourceAllocInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ResourceAllocGlobalState>(); }
static OperatorResultType ResourceAllocInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ResourceAllocGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ResourceAllocFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ResourceAllocGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_resource_allocation(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Resource Allocation failed: " + GetOnagerError());
    gs.result_n1.resize(nc); gs.result_n2.resize(nc); gs.result_scores.resize(nc);
    ::onager::onager_compute_resource_allocation(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_n1.data(), gs.result_n2.data(), gs.result_scores.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_n1.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n1 = FlatVector::GetData<int64_t>(output.data[0]); auto n2 = FlatVector::GetData<int64_t>(output.data[1]); auto sc = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { n1[i] = gs.result_n1[gs.output_idx+i]; n2[i] = gs.result_n2[gs.output_idx+i]; sc[i] = gs.result_scores[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_n1.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Common Neighbors
// =============================================================================

struct CommonNeighborsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_n1, result_n2, result_counts;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> CommonNeighborsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_lnk_common_neighbors");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node1");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node2");
  rt.push_back(LogicalType::BIGINT); nm.push_back("count");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> CommonNeighborsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<CommonNeighborsGlobalState>(); }
static OperatorResultType CommonNeighborsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<CommonNeighborsGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType CommonNeighborsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<CommonNeighborsGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_common_neighbors(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("CommonNeighbors failed: " + GetOnagerError());
    gs.result_n1.resize(nc); gs.result_n2.resize(nc); gs.result_counts.resize(nc);
    ::onager::onager_compute_common_neighbors(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_n1.data(), gs.result_n2.data(), gs.result_counts.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_n1.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n1 = FlatVector::GetData<int64_t>(output.data[0]); auto n2 = FlatVector::GetData<int64_t>(output.data[1]); auto cnt = FlatVector::GetData<int64_t>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { n1[i] = gs.result_n1[gs.output_idx+i]; n2[i] = gs.result_n2[gs.output_idx+i]; cnt[i] = gs.result_counts[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_n1.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterLinkFunctions(ExtensionLoader &loader) {
  TableFunction jaccard("onager_lnk_jaccard", {LogicalType::TABLE}, nullptr, JaccardBind, JaccardInitGlobal);
  jaccard.in_out_function = JaccardInOut;
  jaccard.in_out_function_final = JaccardFinal;
  loader.RegisterFunction(jaccard);

  TableFunction adamic_adar("onager_lnk_adamic_adar", {LogicalType::TABLE}, nullptr, AdamicAdarBind, AdamicAdarInitGlobal);
  adamic_adar.in_out_function = AdamicAdarInOut;
  adamic_adar.in_out_function_final = AdamicAdarFinal;
  loader.RegisterFunction(adamic_adar);

  TableFunction pref_attach("onager_lnk_pref_attach", {LogicalType::TABLE}, nullptr, PrefAttachBind, PrefAttachInitGlobal);
  pref_attach.in_out_function = PrefAttachInOut;
  pref_attach.in_out_function_final = PrefAttachFinal;
  loader.RegisterFunction(pref_attach);

  TableFunction resource_alloc("onager_lnk_resource_alloc", {LogicalType::TABLE}, nullptr, ResourceAllocBind, ResourceAllocInitGlobal);
  resource_alloc.in_out_function = ResourceAllocInOut;
  resource_alloc.in_out_function_final = ResourceAllocFinal;
  loader.RegisterFunction(resource_alloc);

  TableFunction common_neighbors("onager_lnk_common_neighbors", {LogicalType::TABLE}, nullptr, CommonNeighborsBind, CommonNeighborsInitGlobal);
  common_neighbors.in_out_function = CommonNeighborsInOut;
  common_neighbors.in_out_function_final = CommonNeighborsFinal;
  loader.RegisterFunction(common_neighbors);
}

} // namespace onager
} // namespace duckdb
