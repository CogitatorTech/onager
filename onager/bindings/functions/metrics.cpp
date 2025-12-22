/**
 * @file metrics.cpp
 * @brief Graph metrics table functions for Onager DuckDB extension.
 *
 * Diameter, Radius, Average Clustering, Average Path Length, Transitivity, Triangle Count.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Diameter
// =============================================================================

struct DiameterGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  int64_t result = -1;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DiameterBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_diameter requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("diameter");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> DiameterInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<DiameterGlobalState>(); }
static OperatorResultType DiameterInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<DiameterGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType DiameterFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<DiameterGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_diameter(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    if (gs.result < 0) throw InvalidInputException("Diameter failed: " + GetOnagerError());
    gs.computed = true;
  }
  if (gs.output_done) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<int64_t>(output.data[0])[0] = gs.result;
  output.SetCardinality(1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Radius
// =============================================================================

struct RadiusGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  int64_t result = -1;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> RadiusBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_radius requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("radius");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> RadiusInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<RadiusGlobalState>(); }
static OperatorResultType RadiusInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<RadiusGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType RadiusFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<RadiusGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_radius(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    if (gs.result < 0) throw InvalidInputException("Radius failed: " + GetOnagerError());
    gs.computed = true;
  }
  if (gs.output_done) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<int64_t>(output.data[0])[0] = gs.result;
  output.SetCardinality(1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Average Clustering
// =============================================================================

struct AvgClusteringGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  double result = 0.0;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> AvgClusteringBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_avg_clustering requires 2 columns");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("avg_clustering");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> AvgClusteringInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<AvgClusteringGlobalState>(); }
static OperatorResultType AvgClusteringInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgClusteringGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType AvgClusteringFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgClusteringGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_avg_clustering(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    gs.computed = true;
  }
  if (gs.output_done) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<double>(output.data[0])[0] = gs.result;
  output.SetCardinality(1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Triangle Count
// =============================================================================

struct TriangleCountGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_nodes, result_counts;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> TriangleCountBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_triangles requires 2 columns");
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::BIGINT); nm.push_back("triangles");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> TriangleCountInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<TriangleCountGlobalState>(); }
static OperatorResultType TriangleCountInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<TriangleCountGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType TriangleCountFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<TriangleCountGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_triangle_count(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Triangle count failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_counts.resize(nc);
    ::onager::onager_compute_triangle_count(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_nodes.data(), gs.result_counts.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]); auto c = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { n[i] = gs.result_nodes[gs.output_idx+i]; c[i] = gs.result_counts[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Transitivity
// =============================================================================

struct TransitivityGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  double result = 0.0;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> TransitivityBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_transitivity requires 2 columns");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("transitivity");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> TransitivityInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<TransitivityGlobalState>(); }
static OperatorResultType TransitivityInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<TransitivityGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType TransitivityFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<TransitivityGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_transitivity(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    gs.computed = true;
  }
  if (gs.output_done) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<double>(output.data[0])[0] = gs.result;
  output.SetCardinality(1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Average Path Length
// =============================================================================

struct AvgPathLengthGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  double result = 0.0;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> AvgPathLengthBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_avg_path_length requires 2 columns");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("avg_path_length");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> AvgPathLengthInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<AvgPathLengthGlobalState>(); }
static OperatorResultType AvgPathLengthInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgPathLengthGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType AvgPathLengthFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgPathLengthGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_avg_path_length(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    gs.computed = true;
  }
  if (gs.output_done) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<double>(output.data[0])[0] = gs.result;
  output.SetCardinality(1); gs.output_done = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Assortativity
// =============================================================================

struct AssortativityGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  double result = 0.0;
  bool computed = false, output_done = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> AssortativityBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_assortativity requires 2 columns");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("assortativity");
  return make_uniq<TableFunctionData>();
}
static unique_ptr<GlobalTableFunctionState> AssortativityInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<AssortativityGlobalState>(); }
static OperatorResultType AssortativityInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AssortativityGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]); auto d = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(s[i]); gs.dst_nodes.push_back(d[i]); }
  output.SetCardinality(0); return OperatorResultType::NEED_MORE_INPUT;
}
static OperatorFinalizeResultType AssortativityFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<AssortativityGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.result = ::onager::onager_compute_assortativity(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    gs.computed = true;
  }
  if (gs.output_done) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<double>(output.data[0])[0] = gs.result;
  output.SetCardinality(1); gs.output_done = true;
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
  loader.RegisterFunction(diameter);

  TableFunction radius("onager_mtr_radius", {LogicalType::TABLE}, nullptr, RadiusBind, RadiusInitGlobal);
  radius.in_out_function = RadiusInOut;
  radius.in_out_function_final = RadiusFinal;
  loader.RegisterFunction(radius);

  TableFunction avg_clustering("onager_mtr_avg_clustering", {LogicalType::TABLE}, nullptr, AvgClusteringBind, AvgClusteringInitGlobal);
  avg_clustering.in_out_function = AvgClusteringInOut;
  avg_clustering.in_out_function_final = AvgClusteringFinal;
  loader.RegisterFunction(avg_clustering);

  TableFunction triangles("onager_mtr_triangles", {LogicalType::TABLE}, nullptr, TriangleCountBind, TriangleCountInitGlobal);
  triangles.in_out_function = TriangleCountInOut;
  triangles.in_out_function_final = TriangleCountFinal;
  loader.RegisterFunction(triangles);

  TableFunction transitivity("onager_mtr_transitivity", {LogicalType::TABLE}, nullptr, TransitivityBind, TransitivityInitGlobal);
  transitivity.in_out_function = TransitivityInOut;
  transitivity.in_out_function_final = TransitivityFinal;
  loader.RegisterFunction(transitivity);

  TableFunction avg_path_length("onager_mtr_avg_path_length", {LogicalType::TABLE}, nullptr, AvgPathLengthBind, AvgPathLengthInitGlobal);
  avg_path_length.in_out_function = AvgPathLengthInOut;
  avg_path_length.in_out_function_final = AvgPathLengthFinal;
  loader.RegisterFunction(avg_path_length);

  TableFunction assortativity("onager_mtr_assortativity", {LogicalType::TABLE}, nullptr, AssortativityBind, AssortativityInitGlobal);
  assortativity.in_out_function = AssortativityInOut;
  assortativity.in_out_function_final = AssortativityFinal;
  loader.RegisterFunction(assortativity);
}

} // namespace onager
} // namespace duckdb
