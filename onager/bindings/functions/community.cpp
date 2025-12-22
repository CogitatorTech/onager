/**
 * @file community.cpp
 * @brief Community detection table functions for Onager DuckDB extension.
 *
 * Louvain, Connected Components, Label Propagation, Girvan-Newman.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Louvain Community Detection
// =============================================================================

struct LouvainBindData : public TableFunctionData { int64_t seed = -1; };
struct LouvainGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_communities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> LouvainBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<LouvainBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_louvain requires 2 columns");
  for (auto &kv : input.named_parameters) if (kv.first == "seed") bd->seed = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("community");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> LouvainInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<LouvainGlobalState>(); }
static OperatorResultType LouvainInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<LouvainGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType LouvainFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<LouvainBindData>(); auto &gs = data.global_state->Cast<LouvainGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_louvain(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.seed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Louvain failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_communities.resize(nc);
    ::onager::onager_compute_louvain(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.seed, gs.result_nodes.data(), gs.result_communities.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_communities[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Connected Components
// =============================================================================

struct ComponentsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_components;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ComponentsBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_components requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("component");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> ComponentsInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ComponentsGlobalState>(); }
static OperatorResultType ComponentsInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ComponentsGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ComponentsFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ComponentsGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_connected_components(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Components failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_components.resize(nc);
    ::onager::onager_compute_connected_components(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_components.data());
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
// Label Propagation
// =============================================================================

struct LabelPropGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_labels;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> LabelPropBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_label_prop requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("label");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> LabelPropInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<LabelPropGlobalState>(); }
static OperatorResultType LabelPropInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<LabelPropGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType LabelPropFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<LabelPropGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_label_propagation(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Label propagation failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_labels.resize(nc);
    ::onager::onager_compute_label_propagation(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_labels.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto l = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; l[i] = gs.result_labels[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Girvan-Newman
// =============================================================================

struct GirvanNewmanBindData : public TableFunctionData { int64_t target_communities = 2; };
struct GirvanNewmanGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_ids, result_communities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> GirvanNewmanBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<GirvanNewmanBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_girvan_newman requires 2 columns");
  for (auto &kv : input.named_parameters) if (kv.first == "communities") bd->target_communities = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("community");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> GirvanNewmanInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<GirvanNewmanGlobalState>(); }
static OperatorResultType GirvanNewmanInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<GirvanNewmanGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType GirvanNewmanFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<GirvanNewmanBindData>(); auto &gs = data.global_state->Cast<GirvanNewmanGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_girvan_newman(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.target_communities, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Girvan-Newman failed: " + GetOnagerError());
    gs.result_ids.resize(nc); gs.result_communities.resize(nc);
    ::onager::onager_compute_girvan_newman(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.target_communities, gs.result_ids.data(), gs.result_communities.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_ids.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_ids[gs.output_idx+i]; c[i] = gs.result_communities[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_ids.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Spectral Clustering
// =============================================================================

struct SpectralBindData : public TableFunctionData { int64_t k = 2; int64_t seed = -1; };
struct SpectralGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_communities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> SpectralBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<SpectralBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_spectral requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "k") bd->k = kv.second.GetValue<int64_t>();
    if (kv.first == "seed") bd->seed = kv.second.GetValue<int64_t>();
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("community");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> SpectralInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<SpectralGlobalState>(); }
static OperatorResultType SpectralInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<SpectralGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType SpectralFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<SpectralBindData>(); auto &gs = data.global_state->Cast<SpectralGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_spectral_clustering(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.k, bd.seed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Spectral clustering failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_communities.resize(nc);
    ::onager::onager_compute_spectral_clustering(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.k, bd.seed, gs.result_nodes.data(), gs.result_communities.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_communities[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Infomap
// =============================================================================

struct InfomapBindData : public TableFunctionData { int64_t max_iter = 100; int64_t seed = -1; };
struct InfomapGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_communities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> InfomapBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<InfomapBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_infomap requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "max_iter") bd->max_iter = kv.second.GetValue<int64_t>();
    if (kv.first == "seed") bd->seed = kv.second.GetValue<int64_t>();
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("community");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> InfomapInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<InfomapGlobalState>(); }
static OperatorResultType InfomapInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<InfomapGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType InfomapFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<InfomapBindData>(); auto &gs = data.global_state->Cast<InfomapGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_infomap(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.max_iter, bd.seed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Infomap failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_communities.resize(nc);
    ::onager::onager_compute_infomap(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.max_iter, bd.seed, gs.result_nodes.data(), gs.result_communities.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_communities[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterCommunityFunctions(ExtensionLoader &loader) {
  TableFunction louvain("onager_cmm_louvain", {LogicalType::TABLE}, nullptr, LouvainBind, LouvainInitGlobal);
  louvain.in_out_function = LouvainInOut;
  louvain.in_out_function_final = LouvainFinal;
  louvain.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(louvain);

  TableFunction components("onager_cmm_components", {LogicalType::TABLE}, nullptr, ComponentsBind, ComponentsInitGlobal);
  components.in_out_function = ComponentsInOut;
  components.in_out_function_final = ComponentsFinal;
  loader.RegisterFunction(components);

  TableFunction label_prop("onager_cmm_label_prop", {LogicalType::TABLE}, nullptr, LabelPropBind, LabelPropInitGlobal);
  label_prop.in_out_function = LabelPropInOut;
  label_prop.in_out_function_final = LabelPropFinal;
  loader.RegisterFunction(label_prop);

  TableFunction girvan_newman("onager_cmm_girvan_newman", {LogicalType::TABLE}, nullptr, GirvanNewmanBind, GirvanNewmanInitGlobal);
  girvan_newman.in_out_function = GirvanNewmanInOut;
  girvan_newman.in_out_function_final = GirvanNewmanFinal;
  girvan_newman.named_parameters["communities"] = LogicalType::BIGINT;
  loader.RegisterFunction(girvan_newman);

  TableFunction spectral("onager_cmm_spectral", {LogicalType::TABLE}, nullptr, SpectralBind, SpectralInitGlobal);
  spectral.in_out_function = SpectralInOut;
  spectral.in_out_function_final = SpectralFinal;
  spectral.named_parameters["k"] = LogicalType::BIGINT;
  spectral.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(spectral);

  TableFunction infomap("onager_cmm_infomap", {LogicalType::TABLE}, nullptr, InfomapBind, InfomapInitGlobal);
  infomap.in_out_function = InfomapInOut;
  infomap.in_out_function_final = InfomapFinal;
  infomap.named_parameters["max_iter"] = LogicalType::BIGINT;
  infomap.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(infomap);
}

} // namespace onager
} // namespace duckdb
