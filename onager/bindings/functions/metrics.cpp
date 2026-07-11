/**
 * @file metrics.cpp
 * @brief Graph metrics table functions for Onager DuckDB extension.
 *
 * Diameter, Radius, Average Clustering, Average Path Length, Transitivity, Triangle Count.
 */
#include "functions.hpp"
#include <cmath>
#include <mutex>

namespace duckdb {

using namespace onager;


// =============================================================================
// Diameter
// =============================================================================

struct DiameterGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes;
  int64_t result = -1;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

struct DiameterBindData : public TableFunctionData { bool directed = false; };

static unique_ptr<FunctionData> DiameterBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<DiameterBindData>();
  CheckInt64Input(input, "onager_mtr_diameter");
  for (auto &kv : input.named_parameters) if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_mtr_diameter", "directed", kv.second);
  rt.push_back(LogicalType::BIGINT); nm.push_back("diameter");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> DiameterInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<DiameterGlobalState>(); }
static OperatorResultType DiameterInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<DiameterGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_mtr_diameter");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType DiameterFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<DiameterBindData>();
  auto &gs = data.global_state->Cast<DiameterGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_diameter(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed);
    if (gs.result < 0 && OnagerHasPendingError()) throw InvalidInputException("Diameter failed: " + GetOnagerError());
    gs.computed = true;
  }
  if (gs.output_done) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  if (gs.result < 0) {
    // No finite diameter: the graph is disconnected (or not strongly
    // connected in directed mode).
    GetFlatVectorValidityWritable(output.data[0]).SetInvalid(0);
  } else {
    GetFlatVectorDataWritable<int64_t>(output.data[0])[0] = gs.result;
  }
  ONAGER_SET_CARDINALITY(output, 1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Radius
// =============================================================================

struct RadiusGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes;
  int64_t result = -1;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

struct RadiusBindData : public TableFunctionData { bool directed = false; };

static unique_ptr<FunctionData> RadiusBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<RadiusBindData>();
  CheckInt64Input(input, "onager_mtr_radius");
  for (auto &kv : input.named_parameters) if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_mtr_radius", "directed", kv.second);
  rt.push_back(LogicalType::BIGINT); nm.push_back("radius");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> RadiusInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<RadiusGlobalState>(); }
static OperatorResultType RadiusInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<RadiusGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_mtr_radius");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType RadiusFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<RadiusBindData>();
  auto &gs = data.global_state->Cast<RadiusGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_radius(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed);
    if (gs.result < 0 && OnagerHasPendingError()) throw InvalidInputException("Radius failed: " + GetOnagerError());
    gs.computed = true;
  }
  if (gs.output_done) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  if (gs.result < 0) {
    // No finite radius: the graph is disconnected (or not strongly
    // connected in directed mode).
    GetFlatVectorValidityWritable(output.data[0]).SetInvalid(0);
  } else {
    GetFlatVectorDataWritable<int64_t>(output.data[0])[0] = gs.result;
  }
  ONAGER_SET_CARDINALITY(output, 1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Average Clustering
// =============================================================================

struct AvgClusteringGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes;
  double result = 0.0;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> AvgClusteringBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_mtr_avg_clustering");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("avg_clustering");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> AvgClusteringInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<AvgClusteringGlobalState>(); }
static OperatorResultType AvgClusteringInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgClusteringGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_mtr_avg_clustering");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType AvgClusteringFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgClusteringGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_avg_clustering(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    if (std::isnan(gs.result) && OnagerHasPendingError()) throw InvalidInputException("Average clustering failed: " + GetOnagerError());
    gs.computed = true;
  }
  if (gs.output_done) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  if (std::isnan(gs.result)) {
    // The metric is undefined for this graph (for example, a disconnected
    // graph for average path length).
    GetFlatVectorValidityWritable(output.data[0]).SetInvalid(0);
  } else {
    GetFlatVectorDataWritable<double>(output.data[0])[0] = gs.result;
  }
  ONAGER_SET_CARDINALITY(output, 1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Triangle Count
// =============================================================================

struct TriangleCountGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_counts;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> TriangleCountBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_mtr_triangles");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("triangles");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> TriangleCountInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<TriangleCountGlobalState>(); }
static OperatorResultType TriangleCountInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<TriangleCountGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_mtr_triangles");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType TriangleCountFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<TriangleCountGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_triangle_count(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Triangle count failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_counts.resize(nc);
    int64_t rc = ::onager::onager_compute_triangle_count(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_counts.data());
    if (rc != nc) throw InvalidInputException("Triangle count failed: " + GetOnagerError());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]); auto c = GetFlatVectorDataWritable<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_counts[gs.output_idx+i]; }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Transitivity
// =============================================================================

struct TransitivityGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes;
  double result = 0.0;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> TransitivityBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_mtr_transitivity");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("transitivity");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> TransitivityInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<TransitivityGlobalState>(); }
static OperatorResultType TransitivityInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<TransitivityGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_mtr_transitivity");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType TransitivityFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<TransitivityGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_transitivity(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    if (std::isnan(gs.result) && OnagerHasPendingError()) throw InvalidInputException("Transitivity failed: " + GetOnagerError());
    gs.computed = true;
  }
  if (gs.output_done) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  if (std::isnan(gs.result)) {
    // The metric is undefined for this graph (for example, a disconnected
    // graph for average path length).
    GetFlatVectorValidityWritable(output.data[0]).SetInvalid(0);
  } else {
    GetFlatVectorDataWritable<double>(output.data[0])[0] = gs.result;
  }
  ONAGER_SET_CARDINALITY(output, 1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Average Path Length
// =============================================================================

struct AvgPathLengthGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes;
  double result = 0.0;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

struct AvgPathLengthBindData : public TableFunctionData { bool directed = false; };

static unique_ptr<FunctionData> AvgPathLengthBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<AvgPathLengthBindData>();
  CheckInt64Input(input, "onager_mtr_avg_path_length");
  for (auto &kv : input.named_parameters) if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_mtr_avg_path_length", "directed", kv.second);
  rt.push_back(LogicalType::DOUBLE); nm.push_back("avg_path_length");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> AvgPathLengthInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<AvgPathLengthGlobalState>(); }
static OperatorResultType AvgPathLengthInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgPathLengthGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_mtr_avg_path_length");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType AvgPathLengthFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<AvgPathLengthBindData>();
  auto &gs = data.global_state->Cast<AvgPathLengthGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_avg_path_length(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed);
    if (std::isnan(gs.result) && OnagerHasPendingError()) throw InvalidInputException("Average path length failed: " + GetOnagerError());
    gs.computed = true;
  }
  if (gs.output_done) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  if (std::isnan(gs.result)) {
    // The metric is undefined for this graph (for example, a disconnected
    // graph for average path length).
    GetFlatVectorValidityWritable(output.data[0]).SetInvalid(0);
  } else {
    GetFlatVectorDataWritable<double>(output.data[0])[0] = gs.result;
  }
  ONAGER_SET_CARDINALITY(output, 1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Assortativity
// =============================================================================

struct AssortativityGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes;
  double result = 0.0;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

struct AssortativityBindData : public TableFunctionData { bool directed = false; };

static unique_ptr<FunctionData> AssortativityBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<AssortativityBindData>();
  CheckInt64Input(input, "onager_mtr_assortativity");
  for (auto &kv : input.named_parameters) if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_mtr_assortativity", "directed", kv.second);
  rt.push_back(LogicalType::DOUBLE); nm.push_back("assortativity");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> AssortativityInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<AssortativityGlobalState>(); }
static OperatorResultType AssortativityInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AssortativityGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_mtr_assortativity");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType AssortativityFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<AssortativityBindData>();
  auto &gs = data.global_state->Cast<AssortativityGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_assortativity(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed);
    if (std::isnan(gs.result) && OnagerHasPendingError()) throw InvalidInputException("Assortativity failed: " + GetOnagerError());
    gs.computed = true;
  }
  if (gs.output_done) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  if (std::isnan(gs.result)) {
    // The metric is undefined for this graph (for example, a disconnected
    // graph for average path length).
    GetFlatVectorValidityWritable(output.data[0]).SetInvalid(0);
  } else {
    GetFlatVectorDataWritable<double>(output.data[0])[0] = gs.result;
  }
  ONAGER_SET_CARDINALITY(output, 1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Graph Density
// =============================================================================

struct DensityBindData : public TableFunctionData { bool directed = false; };
struct DensityGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
  std::vector<int64_t> src_nodes, dst_nodes;
  double result = 0.0;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DensityBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  CheckInt64Input(input, "onager_mtr_density");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("density");
  auto bd = make_uniq<DensityBindData>();
  for (auto &kv : input.named_parameters) {
    if (kv.first == "directed") bd->directed = GetRequiredParam<bool>("onager_mtr_density", "directed", kv.second);
  }
  return bd;
}
static unique_ptr<GlobalTableFunctionState> DensityInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<DensityGlobalState>(); }
static OperatorResultType DensityInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<DensityGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  AppendInt64Edges(input, gs.src_nodes, gs.dst_nodes, "onager_mtr_density");
  ONAGER_SET_CARDINALITY(output, 0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType DensityFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<DensityGlobalState>();
  auto &bd = data.bind_data->Cast<DensityBindData>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_graph_density(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), bd.directed);
    if (std::isnan(gs.result) && OnagerHasPendingError()) throw InvalidInputException("Density failed: " + GetOnagerError());
    gs.computed = true;
  }
  if (gs.output_done) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  GetFlatVectorDataWritable<double>(output.data[0])[0] = gs.result;
  ONAGER_SET_CARDINALITY(output, 1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterMetricFunctions(ExtensionLoader &loader) {
  TableFunction diameter("onager_mtr_diameter", {LogicalType::TABLE}, nullptr, DiameterBind, DiameterInitGlobal);
  diameter.in_out_function = DiameterInOut;
  diameter.in_out_function_final = DiameterFinal;
  diameter.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(diameter);
  loader.RegisterFunction(diameter);

  TableFunction radius("onager_mtr_radius", {LogicalType::TABLE}, nullptr, RadiusBind, RadiusInitGlobal);
  radius.in_out_function = RadiusInOut;
  radius.in_out_function_final = RadiusFinal;
  radius.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(radius);
  loader.RegisterFunction(radius);

  TableFunction avg_clustering("onager_mtr_avg_clustering", {LogicalType::TABLE}, nullptr, AvgClusteringBind, AvgClusteringInitGlobal);
  avg_clustering.in_out_function = AvgClusteringInOut;
  avg_clustering.in_out_function_final = AvgClusteringFinal;
  ONAGER_SET_NO_ORDER(avg_clustering);
  loader.RegisterFunction(avg_clustering);

  TableFunction triangles("onager_mtr_triangles", {LogicalType::TABLE}, nullptr, TriangleCountBind, TriangleCountInitGlobal);
  triangles.in_out_function = TriangleCountInOut;
  triangles.in_out_function_final = TriangleCountFinal;
  ONAGER_SET_NO_ORDER(triangles);
  loader.RegisterFunction(triangles);

  TableFunction transitivity("onager_mtr_transitivity", {LogicalType::TABLE}, nullptr, TransitivityBind, TransitivityInitGlobal);
  transitivity.in_out_function = TransitivityInOut;
  transitivity.in_out_function_final = TransitivityFinal;
  ONAGER_SET_NO_ORDER(transitivity);
  loader.RegisterFunction(transitivity);

  TableFunction avg_path_length("onager_mtr_avg_path_length", {LogicalType::TABLE}, nullptr, AvgPathLengthBind, AvgPathLengthInitGlobal);
  avg_path_length.in_out_function = AvgPathLengthInOut;
  avg_path_length.in_out_function_final = AvgPathLengthFinal;
  avg_path_length.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(avg_path_length);
  loader.RegisterFunction(avg_path_length);

  TableFunction assortativity("onager_mtr_assortativity", {LogicalType::TABLE}, nullptr, AssortativityBind, AssortativityInitGlobal);
  assortativity.in_out_function = AssortativityInOut;
  assortativity.in_out_function_final = AssortativityFinal;
  assortativity.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(assortativity);
  loader.RegisterFunction(assortativity);

  TableFunction density("onager_mtr_density", {LogicalType::TABLE}, nullptr, DensityBind, DensityInitGlobal);
  density.in_out_function = DensityInOut;
  density.in_out_function_final = DensityFinal;
  density.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(density);
  loader.RegisterFunction(density);
}

} // namespace onager
} // namespace duckdb
