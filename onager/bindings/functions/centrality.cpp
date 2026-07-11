/**
 * @file centrality.cpp
 * @brief Centrality table functions for Onager DuckDB extension.
 *
 * PageRank, Degree, Betweenness, Closeness, Eigenvector, Katz, Harmonic.
 */
#include "functions.hpp"
#include <mutex>

namespace duckdb {

using namespace onager;

// =============================================================================
// PageRank Table Function
// =============================================================================

struct PageRankBindData : public TableFunctionData {
  double damping = 0.85;
  int64_t iterations = 100;
  double tolerance = 1e-6;
  bool directed = false;
  bool weighted = false;
};

struct PageRankGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<double> weights;
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
  CheckInt64Input(input, "onager_ctr_pagerank");
  bind_data->weighted = input.input_table_types.size() >= 3 && input.input_table_types[2] == LogicalType::DOUBLE;
  for (auto &kv : input.named_parameters) {
    if (kv.first == "damping") bind_data->damping = GetRequiredParam<double>("onager_ctr_pagerank", kv.first, kv.second);
    else if (kv.first == "iterations") bind_data->iterations = GetNonNegativeParam("onager_ctr_pagerank", kv.first, kv.second);
    else if (kv.first == "tolerance") bind_data->tolerance = GetRequiredParam<double>("onager_ctr_pagerank", kv.first, kv.second);
    else if (kv.first == "directed") bind_data->directed = GetRequiredParam<bool>("onager_ctr_pagerank", kv.first, kv.second);
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
  auto &bind = data.bind_data->Cast<PageRankBindData>();
  auto &gs = data.global_state->Cast<PageRankGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (bind.weighted) {
    AppendWeightedEdges(input, gs.src_nodes, gs.dst_nodes, gs.weights, "onager_ctr_pagerank");
  } else {
    AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_pagerank");
  }
  ONAGER_SET_CARDINALITY(output, 0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType PageRankFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind = data.bind_data->Cast<PageRankBindData>();
  auto &gs = data.global_state->Cast<PageRankGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    size_t ec = gs.src_nodes.size();
    const double *w = gs.weights.empty() ? nullptr : gs.weights.data();
    int64_t nc = ::onager::onager_compute_pagerank(gs.src_nodes.data(), gs.dst_nodes.data(), ec, w, gs.weights.size(),
        bind.damping, static_cast<size_t>(bind.iterations), bind.tolerance, bind.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("PageRank failed: " + GetOnagerError());
    gs.result_nodes.resize(static_cast<size_t>(nc)); gs.result_ranks.resize(static_cast<size_t>(nc));
    int64_t rc = ::onager::onager_compute_pagerank(gs.src_nodes.data(), gs.dst_nodes.data(), ec, w, gs.weights.size(),
        bind.damping, static_cast<size_t>(bind.iterations), bind.tolerance, bind.directed, gs.result_nodes.data(), gs.result_ranks.data());
    if (rc != nc) throw InvalidInputException("PageRank failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto ndata = GetFlatVectorDataWritable<int64_t>(output.data[0]);
  auto rdata = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { ndata[i] = gs.result_nodes[gs.output_idx+i]; rdata[i] = gs.result_ranks[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Degree Centrality Table Function
// =============================================================================

struct DegreeBindData : public TableFunctionData { bool directed = false; };
struct DegreeGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_in, result_out;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DegreeBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<DegreeBindData>();
  CheckInt64Input(input, "onager_ctr_degree");
  for (auto &kv : input.named_parameters) if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_ctr_degree", kv.first, kv.second);
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("in_degree");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("out_degree");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> DegreeInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<DegreeGlobalState>(); }
static OperatorResultType DegreeInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<DegreeGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_degree");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType DegreeFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<DegreeBindData>(); auto &gs = data.global_state->Cast<DegreeGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_degree(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, nullptr, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Degree failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_in.resize(nc); gs.result_out.resize(nc);
    int64_t rc = ::onager::onager_compute_degree(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, gs.result_nodes.data(), gs.result_in.data(), gs.result_out.data());
    if (rc != nc) throw InvalidInputException("Degree failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto id = GetFlatVectorDataWritable<double>(output.data[1]); auto od = GetFlatVectorDataWritable<double>(output.data[2]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; id[i] = gs.result_in[gs.output_idx+i]; od[i] = gs.result_out[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Betweenness Centrality Table Function
// =============================================================================

struct BetweennessBindData : public TableFunctionData { bool normalized = true; bool directed = false; };
struct BetweennessGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> BetweennessBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<BetweennessBindData>();
  CheckInt64Input(input, "onager_ctr_betweenness");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "normalized") bd->normalized = GetRequiredParam<bool>("onager_ctr_betweenness", kv.first, kv.second);
    if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_ctr_betweenness", kv.first, kv.second);
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("betweenness");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> BetweennessInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<BetweennessGlobalState>(); }
static OperatorResultType BetweennessInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<BetweennessGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_betweenness");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType BetweennessFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<BetweennessBindData>(); auto &gs = data.global_state->Cast<BetweennessGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_betweenness(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.normalized, bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Betweenness failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    int64_t rc = ::onager::onager_compute_betweenness(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.normalized, bd.directed, gs.result_nodes.data(), gs.result_centralities.data());
    if (rc != nc) throw InvalidInputException("Betweenness failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Closeness Centrality Table Function
// =============================================================================

struct ClosenessGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

struct ClosenessBindData : public TableFunctionData { bool directed = false; };

static unique_ptr<FunctionData> ClosenessBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<ClosenessBindData>();
  CheckInt64Input(input, "onager_ctr_closeness");
  for (auto &kv : input.named_parameters) if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_ctr_closeness", kv.first, kv.second);
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("closeness");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> ClosenessInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ClosenessGlobalState>(); }
static OperatorResultType ClosenessInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ClosenessGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_closeness");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType ClosenessFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<ClosenessBindData>();
  auto &gs = data.global_state->Cast<ClosenessGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_closeness(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Closeness failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    int64_t rc = ::onager::onager_compute_closeness(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, gs.result_nodes.data(), gs.result_centralities.data());
    if (rc != nc) throw InvalidInputException("Closeness failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Harmonic Centrality Table Function
// =============================================================================

struct HarmonicGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

struct HarmonicBindData : public TableFunctionData { bool directed = false; };

static unique_ptr<FunctionData> HarmonicBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<HarmonicBindData>();
  CheckInt64Input(input, "onager_ctr_harmonic");
  for (auto &kv : input.named_parameters) if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_ctr_harmonic", kv.first, kv.second);
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("harmonic");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> HarmonicInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<HarmonicGlobalState>(); }
static OperatorResultType HarmonicInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<HarmonicGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_harmonic");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType HarmonicFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<HarmonicBindData>();
  auto &gs = data.global_state->Cast<HarmonicGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_harmonic(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Harmonic failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    int64_t rc = ::onager::onager_compute_harmonic(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, gs.result_nodes.data(), gs.result_centralities.data());
    if (rc != nc) throw InvalidInputException("Harmonic failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Katz Centrality Table Function
// =============================================================================

struct KatzBindData : public TableFunctionData { double alpha = 0.1; double beta = 1.0; int64_t max_iter = 100; double tolerance = 1e-6; bool directed = false; };
struct KatzGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> KatzBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<KatzBindData>();
  CheckInt64Input(input, "onager_ctr_katz");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "alpha") bd->alpha = GetRequiredParam<double>("onager_ctr_katz", kv.first, kv.second);
    if (kv.first == "beta") bd->beta = GetRequiredParam<double>("onager_ctr_katz", kv.first, kv.second);
    if (kv.first == "max_iter") bd->max_iter = GetNonNegativeParam("onager_ctr_katz", kv.first, kv.second);
    if (kv.first == "tolerance") bd->tolerance = GetRequiredParam<double>("onager_ctr_katz", kv.first, kv.second);
    if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_ctr_katz", kv.first, kv.second);
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("katz");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> KatzInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<KatzGlobalState>(); }
static OperatorResultType KatzInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<KatzGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_katz");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType KatzFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<KatzBindData>(); auto &gs = data.global_state->Cast<KatzGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_katz(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.alpha, bd.beta, bd.max_iter, bd.tolerance, bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Katz failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    int64_t rc = ::onager::onager_compute_katz(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.alpha, bd.beta, bd.max_iter, bd.tolerance, bd.directed, gs.result_nodes.data(), gs.result_centralities.data());
    if (rc != nc) throw InvalidInputException("Katz failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Eigenvector Centrality Table Function
// =============================================================================

struct EigenvectorBindData : public TableFunctionData { int64_t max_iter = 100; double tolerance = 1e-6; bool directed = false; };
struct EigenvectorGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> EigenvectorBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<EigenvectorBindData>();
  CheckInt64Input(input, "onager_ctr_eigenvector");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "max_iter") bd->max_iter = GetNonNegativeParam("onager_ctr_eigenvector", kv.first, kv.second);
    if (kv.first == "tolerance") bd->tolerance = GetRequiredParam<double>("onager_ctr_eigenvector", kv.first, kv.second);
    if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_ctr_eigenvector", kv.first, kv.second);
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("eigenvector");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> EigenvectorInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<EigenvectorGlobalState>(); }
static OperatorResultType EigenvectorInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<EigenvectorGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_eigenvector");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType EigenvectorFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<EigenvectorBindData>(); auto &gs = data.global_state->Cast<EigenvectorGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_eigenvector(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.max_iter, bd.tolerance, bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Eigenvector failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centralities.resize(nc);
    int64_t rc = ::onager::onager_compute_eigenvector(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.max_iter, bd.tolerance, bd.directed, gs.result_nodes.data(), gs.result_centralities.data());
    if (rc != nc) throw InvalidInputException("Eigenvector failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centralities[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
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
  pagerank.named_parameters["tolerance"] = LogicalType::DOUBLE;
  pagerank.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(pagerank);
  loader.RegisterFunction(pagerank);

  TableFunction degree("onager_ctr_degree", {LogicalType::TABLE}, nullptr, DegreeBind, DegreeInitGlobal);
  degree.in_out_function = DegreeInOut;
  degree.in_out_function_final = DegreeFinal;
  degree.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(degree);
  loader.RegisterFunction(degree);

  TableFunction betweenness("onager_ctr_betweenness", {LogicalType::TABLE}, nullptr, BetweennessBind, BetweennessInitGlobal);
  betweenness.in_out_function = BetweennessInOut;
  betweenness.in_out_function_final = BetweennessFinal;
  betweenness.named_parameters["normalized"] = LogicalType::BOOLEAN;
  betweenness.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(betweenness);
  loader.RegisterFunction(betweenness);

  TableFunction closeness("onager_ctr_closeness", {LogicalType::TABLE}, nullptr, ClosenessBind, ClosenessInitGlobal);
  closeness.in_out_function = ClosenessInOut;
  closeness.in_out_function_final = ClosenessFinal;
  closeness.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(closeness);
  loader.RegisterFunction(closeness);

  TableFunction harmonic("onager_ctr_harmonic", {LogicalType::TABLE}, nullptr, HarmonicBind, HarmonicInitGlobal);
  harmonic.in_out_function = HarmonicInOut;
  harmonic.in_out_function_final = HarmonicFinal;
  harmonic.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(harmonic);
  loader.RegisterFunction(harmonic);

  TableFunction katz("onager_ctr_katz", {LogicalType::TABLE}, nullptr, KatzBind, KatzInitGlobal);
  katz.in_out_function = KatzInOut;
  katz.in_out_function_final = KatzFinal;
  katz.named_parameters["alpha"] = LogicalType::DOUBLE;
  katz.named_parameters["beta"] = LogicalType::DOUBLE;
  katz.named_parameters["max_iter"] = LogicalType::BIGINT;
  katz.named_parameters["tolerance"] = LogicalType::DOUBLE;
  katz.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(katz);
  loader.RegisterFunction(katz);

  TableFunction eigenvector("onager_ctr_eigenvector", {LogicalType::TABLE}, nullptr, EigenvectorBind, EigenvectorInitGlobal);
  eigenvector.in_out_function = EigenvectorInOut;
  eigenvector.in_out_function_final = EigenvectorFinal;
  eigenvector.named_parameters["max_iter"] = LogicalType::BIGINT;
  eigenvector.named_parameters["tolerance"] = LogicalType::DOUBLE;
  eigenvector.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(eigenvector);
  loader.RegisterFunction(eigenvector);
}

// Forward declare VoteRank registration (defined at end of file)
void RegisterVoteRankFunction(ExtensionLoader &loader);

void RegisterAllCentralityFunctions(ExtensionLoader &loader) {
  RegisterCentralityFunctions(loader);
  RegisterVoteRankFunction(loader);
}

} // namespace onager
} // namespace duckdb

// =============================================================================
// VoteRank Table Function (added after registration to keep pattern)
// =============================================================================

namespace duckdb {

using namespace onager;

struct VoteRankBindData : public TableFunctionData { int64_t num_seeds = 10; bool directed = false; };
struct VoteRankGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> VoteRankBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<VoteRankBindData>();
  CheckInt64Input(input, "onager_ctr_voterank");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "num_seeds") bd->num_seeds = GetNonNegativeParam("onager_ctr_voterank", kv.first, kv.second);
    if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_ctr_voterank", kv.first, kv.second);
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> VoteRankInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<VoteRankGlobalState>(); }
static OperatorResultType VoteRankInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<VoteRankGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_voterank");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType VoteRankFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<VoteRankBindData>(); auto &gs = data.global_state->Cast<VoteRankGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_voterank(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.num_seeds, bd.directed, nullptr);
    if (nc < 0) throw InvalidInputException("VoteRank failed: " + GetOnagerError());
    gs.result_nodes.resize(nc);
    int64_t rc = ::onager::onager_compute_voterank(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.num_seeds, bd.directed, gs.result_nodes.data());
    if (rc != nc) throw InvalidInputException("VoteRank failed: " + GetOnagerError());
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

namespace onager {
// Register VoteRank separately to avoid restructuring entire file
void RegisterVoteRankFunction(ExtensionLoader &loader) {
  TableFunction voterank("onager_ctr_voterank", {LogicalType::TABLE}, nullptr, VoteRankBind, VoteRankInitGlobal);
  voterank.in_out_function = VoteRankInOut;
  voterank.in_out_function_final = VoteRankFinal;
  voterank.named_parameters["num_seeds"] = LogicalType::BIGINT;
  voterank.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(voterank);
  loader.RegisterFunction(voterank);
}
} // namespace onager
} // namespace duckdb

// =============================================================================
// Local Reaching Centrality Table Function
// =============================================================================

namespace duckdb {
using namespace onager;

struct LocalReachingBindData : public TableFunctionData { int64_t distance = 2; bool directed = false; };
struct LocalReachingGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centrality;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> LocalReachingBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<LocalReachingBindData>();
  CheckInt64Input(input, "onager_ctr_local_reaching");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "distance") bd->distance = GetNonNegativeParam("onager_ctr_local_reaching", kv.first, kv.second);
    if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_ctr_local_reaching", kv.first, kv.second);
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("centrality");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> LocalReachingInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<LocalReachingGlobalState>(); }
static OperatorResultType LocalReachingInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<LocalReachingGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_local_reaching");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType LocalReachingFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<LocalReachingBindData>(); auto &gs = data.global_state->Cast<LocalReachingGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_local_reaching(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.distance, bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("LocalReaching failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centrality.resize(nc);
    int64_t rc = ::onager::onager_compute_local_reaching(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.distance, bd.directed, gs.result_nodes.data(), gs.result_centrality.data());
    if (rc != nc) throw InvalidInputException("LocalReaching failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centrality[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Laplacian Centrality Table Function
// =============================================================================

struct LaplacianGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes;
  std::vector<double> result_centrality;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

struct LaplacianBindData : public TableFunctionData { bool directed = false; };

static unique_ptr<FunctionData> LaplacianBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<LaplacianBindData>();
  CheckInt64Input(input, "onager_ctr_laplacian");
  for (auto &kv : input.named_parameters) if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_ctr_laplacian", kv.first, kv.second);
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("centrality");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> LaplacianInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<LaplacianGlobalState>(); }
static OperatorResultType LaplacianInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<LaplacianGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_ctr_laplacian");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType LaplacianFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<LaplacianBindData>();
  auto &gs = data.global_state->Cast<LaplacianGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_laplacian(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Laplacian failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_centrality.resize(nc);
    int64_t rc = ::onager::onager_compute_laplacian(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed, gs.result_nodes.data(), gs.result_centrality.data());
    if (rc != nc) throw InvalidInputException("Laplacian failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_centrality[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

namespace onager {
void RegisterLocalReachingFunction(ExtensionLoader &loader) {
  TableFunction lr("onager_ctr_local_reaching", {LogicalType::TABLE}, nullptr, LocalReachingBind, LocalReachingInitGlobal);
  lr.in_out_function = LocalReachingInOut;
  lr.in_out_function_final = LocalReachingFinal;
  lr.named_parameters["distance"] = LogicalType::BIGINT;
  lr.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(lr);
  loader.RegisterFunction(lr);
}
void RegisterLaplacianFunction(ExtensionLoader &loader) {
  TableFunction lap("onager_ctr_laplacian", {LogicalType::TABLE}, nullptr, LaplacianBind, LaplacianInitGlobal);
  lap.in_out_function = LaplacianInOut;
  lap.in_out_function_final = LaplacianFinal;
  lap.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(lap);
  loader.RegisterFunction(lap);
}
} // namespace onager
} // namespace duckdb
