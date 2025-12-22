/**
 * @file centrality.cpp
 * @brief Centrality table functions for Onager DuckDB extension.
 *
 * PageRank, Degree, Betweenness, Closeness, Eigenvector, Katz, Harmonic.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// PageRank Table Function
// =============================================================================

struct PageRankBindData : public TableFunctionData {
  double damping = 0.85;
  int64_t iterations = 100;
  bool directed = true;
};

struct PageRankGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<double> result_ranks;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> PageRankBind(ClientContext &context,
                                              TableFunctionBindInput &input,
                                              vector<LogicalType> &return_types,
                                              vector<string> &names) {
  auto bind_data = make_uniq<PageRankBindData>();
  if (input.input_table_types.size() < 2) {
    throw InvalidInputException("onager_pagerank requires a table with at least 2 columns: (src, dst)");
  }
  for (auto &kv : input.named_parameters) {
    if (kv.first == "damping") bind_data->damping = kv.second.GetValue<double>();
    else if (kv.first == "iterations") bind_data->iterations = kv.second.GetValue<int64_t>();
    else if (kv.first == "directed") bind_data->directed = kv.second.GetValue<bool>();
  }
  return_types.push_back(LogicalType::BIGINT); names.push_back("node_id");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("rank");
  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> PageRankInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<PageRankGlobalState>();
}

static OperatorResultType PageRankInOut(ExecutionContext &context, TableFunctionInput &data,
                                         DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<PageRankGlobalState>();
  auto src = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) {
    gs.src_nodes.push_back(src[i]);
    gs.dst_nodes.push_back(dst[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType PageRankFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind = data.bind_data->Cast<PageRankBindData>();
  auto &gs = data.global_state->Cast<PageRankGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    size_t ec = gs.src_nodes.size();
    int64_t nc = ::onager::onager_compute_pagerank(gs.src_nodes.data(), gs.dst_nodes.data(), ec,
        bind.damping, static_cast<size_t>(bind.iterations), bind.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("PageRank failed: " + GetOnagerError());
    gs.result_nodes.resize(static_cast<size_t>(nc)); gs.result_ranks.resize(static_cast<size_t>(nc));
    ::onager::onager_compute_pagerank(gs.src_nodes.data(), gs.dst_nodes.data(), ec,
        bind.damping, static_cast<size_t>(bind.iterations), bind.directed, gs.result_nodes.data(), gs.result_ranks.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto ndata = FlatVector::GetData<int64_t>(output.data[0]);
  auto rdata = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { ndata[i] = gs.result_nodes[gs.output_idx+i]; rdata[i] = gs.result_ranks[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Degree Centrality Table Function
// =============================================================================

struct DegreeBindData : public TableFunctionData { bool directed = true; };
struct DegreeGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_in, result_out;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DegreeBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<DegreeBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_degree requires 2 columns");
  for (auto &kv : input.named_parameters) if (kv.first == "directed") bd->directed = kv.second.GetValue<bool>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("in_degree");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("out_degree");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> DegreeInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<DegreeGlobalState>(); }
static OperatorResultType DegreeInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<DegreeGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType DegreeFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<DegreeBindData>(); auto &gs = data.global_state->Cast<DegreeGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_degree(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, nullptr, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Degree failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_in.resize(nc); gs.result_out.resize(nc);
    ::onager::onager_compute_degree(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, gs.result_nodes.data(), gs.result_in.data(), gs.result_out.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto id = FlatVector::GetData<double>(output.data[1]); auto od = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; id[i] = gs.result_in[gs.output_idx+i]; od[i] = gs.result_out[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Betweenness Centrality Table Function
// =============================================================================

struct BetweennessBindData : public TableFunctionData { bool normalized = true; };
struct BetweennessGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> BetweennessBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<BetweennessBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_betweenness requires 2 columns");
  for (auto &kv : input.named_parameters) if (kv.first == "normalized") bd->normalized = kv.second.GetValue<bool>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("betweenness");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> BetweennessInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<BetweennessGlobalState>(); }
static OperatorResultType BetweennessInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<BetweennessGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType BetweennessFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<BetweennessBindData>(); auto &gs = data.global_state->Cast<BetweennessGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_betweenness(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.normalized, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Betweenness failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    ::onager::onager_compute_betweenness(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.normalized, gs.result_nodes.data(), gs.result_centralities.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Closeness Centrality Table Function
// =============================================================================

struct ClosenessGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ClosenessBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_closeness requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("closeness");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> ClosenessInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ClosenessGlobalState>(); }
static OperatorResultType ClosenessInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ClosenessGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ClosenessFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ClosenessGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_closeness(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Closeness failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    ::onager::onager_compute_closeness(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_centralities.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Harmonic Centrality Table Function
// =============================================================================

struct HarmonicGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> HarmonicBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_harmonic requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("harmonic");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> HarmonicInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<HarmonicGlobalState>(); }
static OperatorResultType HarmonicInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<HarmonicGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType HarmonicFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<HarmonicGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_harmonic(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Harmonic failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    ::onager::onager_compute_harmonic(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_centralities.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Katz Centrality Table Function
// =============================================================================

struct KatzBindData : public TableFunctionData { double alpha = 0.1; int64_t max_iter = 100; double tolerance = 1e-6; };
struct KatzGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> KatzBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<KatzBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_katz requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "alpha") bd->alpha = kv.second.GetValue<double>();
    if (kv.first == "max_iter") bd->max_iter = kv.second.GetValue<int64_t>();
    if (kv.first == "tolerance") bd->tolerance = kv.second.GetValue<double>();
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("katz");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> KatzInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<KatzGlobalState>(); }
static OperatorResultType KatzInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<KatzGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType KatzFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<KatzBindData>(); auto &gs = data.global_state->Cast<KatzGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_katz(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.alpha, bd.max_iter, bd.tolerance, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Katz failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    ::onager::onager_compute_katz(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.alpha, bd.max_iter, bd.tolerance, gs.result_nodes.data(), gs.result_centralities.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Eigenvector Centrality Table Function
// =============================================================================

struct EigenvectorBindData : public TableFunctionData { int64_t max_iter = 100; double tolerance = 1e-6; };
struct EigenvectorGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> EigenvectorBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<EigenvectorBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_eigenvector requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "max_iter") bd->max_iter = kv.second.GetValue<int64_t>();
    if (kv.first == "tolerance") bd->tolerance = kv.second.GetValue<double>();
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("eigenvector");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> EigenvectorInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<EigenvectorGlobalState>(); }
static OperatorResultType EigenvectorInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<EigenvectorGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType EigenvectorFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<EigenvectorBindData>(); auto &gs = data.global_state->Cast<EigenvectorGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_eigenvector(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.max_iter, bd.tolerance, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Eigenvector failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    ::onager::onager_compute_eigenvector(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.max_iter, bd.tolerance, gs.result_nodes.data(), gs.result_centralities.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterCentralityFunctions(ExtensionLoader &loader) {
  TableFunction pagerank("onager_ctr_pagerank", {LogicalType::TABLE}, nullptr, PageRankBind, PageRankInitGlobal);
  pagerank.in_out_function = PageRankInOut;
  pagerank.in_out_function_final = PageRankFinal;
  pagerank.named_parameters["damping"] = LogicalType::DOUBLE;
  pagerank.named_parameters["iterations"] = LogicalType::BIGINT;
  pagerank.named_parameters["directed"] = LogicalType::BOOLEAN;
  loader.RegisterFunction(pagerank);

  TableFunction degree("onager_ctr_degree", {LogicalType::TABLE}, nullptr, DegreeBind, DegreeInitGlobal);
  degree.in_out_function = DegreeInOut;
  degree.in_out_function_final = DegreeFinal;
  degree.named_parameters["directed"] = LogicalType::BOOLEAN;
  loader.RegisterFunction(degree);

  TableFunction betweenness("onager_ctr_betweenness", {LogicalType::TABLE}, nullptr, BetweennessBind, BetweennessInitGlobal);
  betweenness.in_out_function = BetweennessInOut;
  betweenness.in_out_function_final = BetweennessFinal;
  betweenness.named_parameters["normalized"] = LogicalType::BOOLEAN;
  loader.RegisterFunction(betweenness);

  TableFunction closeness("onager_ctr_closeness", {LogicalType::TABLE}, nullptr, ClosenessBind, ClosenessInitGlobal);
  closeness.in_out_function = ClosenessInOut;
  closeness.in_out_function_final = ClosenessFinal;
  loader.RegisterFunction(closeness);

  TableFunction harmonic("onager_ctr_harmonic", {LogicalType::TABLE}, nullptr, HarmonicBind, HarmonicInitGlobal);
  harmonic.in_out_function = HarmonicInOut;
  harmonic.in_out_function_final = HarmonicFinal;
  loader.RegisterFunction(harmonic);

  TableFunction katz("onager_ctr_katz", {LogicalType::TABLE}, nullptr, KatzBind, KatzInitGlobal);
  katz.in_out_function = KatzInOut;
  katz.in_out_function_final = KatzFinal;
  katz.named_parameters["alpha"] = LogicalType::DOUBLE;
  katz.named_parameters["max_iter"] = LogicalType::BIGINT;
  katz.named_parameters["tolerance"] = LogicalType::DOUBLE;
  loader.RegisterFunction(katz);

  TableFunction eigenvector("onager_ctr_eigenvector", {LogicalType::TABLE}, nullptr, EigenvectorBind, EigenvectorInitGlobal);
  eigenvector.in_out_function = EigenvectorInOut;
  eigenvector.in_out_function_final = EigenvectorFinal;
  eigenvector.named_parameters["max_iter"] = LogicalType::BIGINT;
  eigenvector.named_parameters["tolerance"] = LogicalType::DOUBLE;
  loader.RegisterFunction(eigenvector);

  // VoteRank needs forward declarations which are below, so we register inline here
  // The implementation is at the end of the file
}

} // namespace onager
} // namespace duckdb

// =============================================================================
// VoteRank Table Function (added after registration to keep pattern)
// =============================================================================

namespace duckdb {

using namespace onager;

struct VoteRankBindData : public TableFunctionData { int64_t num_seeds = 10; };
struct VoteRankGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> VoteRankBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<VoteRankBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_voterank requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "num_seeds") bd->num_seeds = kv.second.GetValue<int64_t>();
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> VoteRankInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<VoteRankGlobalState>(); }
static OperatorResultType VoteRankInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<VoteRankGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType VoteRankFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<VoteRankBindData>(); auto &gs = data.global_state->Cast<VoteRankGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_voterank(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.num_seeds, nullptr);
    if (nc < 0) throw InvalidInputException("VoteRank failed: " + GetOnagerError());
    gs.result_nodes.resize(nc);
    ::onager::onager_compute_voterank(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.num_seeds, gs.result_nodes.data());
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

namespace onager {
// Register VoteRank separately to avoid restructuring entire file
static void RegisterVoteRankFunction(ExtensionLoader &loader) {
  TableFunction voterank("onager_ctr_voterank", {LogicalType::TABLE}, nullptr, VoteRankBind, VoteRankInitGlobal);
  voterank.in_out_function = VoteRankInOut;
  voterank.in_out_function_final = VoteRankFinal;
  voterank.named_parameters["num_seeds"] = LogicalType::BIGINT;
  loader.RegisterFunction(voterank);
}
} // namespace onager
} // namespace duckdb
