/**
 * @file generators.cpp
 * @brief Graph generator table functions for Onager DuckDB extension.
 *
 * Erdős-Rényi, Barabási-Albert, Watts-Strogatz.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Erdős-Rényi Random Graph
// =============================================================================

struct ErdosRenyiBindData : public TableFunctionData {
  int64_t n = 10; double p = 0.5; int64_t seed = 42;
};
struct ErdosRenyiGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> result_src, result_dst;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ErdosRenyiBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<ErdosRenyiBindData>();
  if (input.inputs.size() >= 1) bd->n = input.inputs[0].GetValue<int64_t>();
  if (input.inputs.size() >= 2) bd->p = input.inputs[1].GetValue<double>();
  for (auto &kv : input.named_parameters) if (kv.first == "seed") bd->seed = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("src");
  rt.push_back(LogicalType::BIGINT); nm.push_back("dst");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> ErdosRenyiInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<ErdosRenyiGlobalState>(); }
static void ErdosRenyiFunction(ClientContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<ErdosRenyiBindData>(); auto &gs = data.global_state->Cast<ErdosRenyiGlobalState>();
  if (!gs.computed) {
    int64_t ec = ::onager::onager_generate_erdos_renyi(static_cast<size_t>(bd.n), bd.p, static_cast<uint64_t>(bd.seed), nullptr, nullptr);
    if (ec < 0) throw InvalidInputException("Erdos-Renyi failed: " + GetOnagerError());
    gs.result_src.resize(ec); gs.result_dst.resize(ec);
    ::onager::onager_generate_erdos_renyi(static_cast<size_t>(bd.n), bd.p, static_cast<uint64_t>(bd.seed), gs.result_src.data(), gs.result_dst.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_src.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto s = FlatVector::GetData<int64_t>(output.data[0]); auto d = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { s[i] = gs.result_src[gs.output_idx+i]; d[i] = gs.result_dst[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
}

// =============================================================================
// Barabási-Albert Preferential Attachment
// =============================================================================

struct BarabasiAlbertBindData : public TableFunctionData {
  int64_t n = 10; int64_t m = 2; int64_t seed = 42;
};
struct BarabasiAlbertGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> result_src, result_dst;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> BarabasiAlbertBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<BarabasiAlbertBindData>();
  if (input.inputs.size() >= 1) bd->n = input.inputs[0].GetValue<int64_t>();
  if (input.inputs.size() >= 2) bd->m = input.inputs[1].GetValue<int64_t>();
  for (auto &kv : input.named_parameters) if (kv.first == "seed") bd->seed = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("src");
  rt.push_back(LogicalType::BIGINT); nm.push_back("dst");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> BarabasiAlbertInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<BarabasiAlbertGlobalState>(); }
static void BarabasiAlbertFunction(ClientContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<BarabasiAlbertBindData>(); auto &gs = data.global_state->Cast<BarabasiAlbertGlobalState>();
  if (!gs.computed) {
    int64_t ec = ::onager::onager_generate_barabasi_albert(static_cast<size_t>(bd.n), static_cast<size_t>(bd.m), static_cast<uint64_t>(bd.seed), nullptr, nullptr);
    if (ec < 0) throw InvalidInputException("Barabasi-Albert failed: " + GetOnagerError());
    gs.result_src.resize(ec); gs.result_dst.resize(ec);
    ::onager::onager_generate_barabasi_albert(static_cast<size_t>(bd.n), static_cast<size_t>(bd.m), static_cast<uint64_t>(bd.seed), gs.result_src.data(), gs.result_dst.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_src.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto s = FlatVector::GetData<int64_t>(output.data[0]); auto d = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { s[i] = gs.result_src[gs.output_idx+i]; d[i] = gs.result_dst[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
}

// =============================================================================
// Watts-Strogatz Small World
// =============================================================================

struct WattsStrogatzBindData : public TableFunctionData {
  int64_t n = 10; int64_t k = 4; double beta = 0.5; int64_t seed = 42;
};
struct WattsStrogatzGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> result_src, result_dst;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> WattsStrogatzBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<WattsStrogatzBindData>();
  if (input.inputs.size() >= 1) bd->n = input.inputs[0].GetValue<int64_t>();
  if (input.inputs.size() >= 2) bd->k = input.inputs[1].GetValue<int64_t>();
  if (input.inputs.size() >= 3) bd->beta = input.inputs[2].GetValue<double>();
  for (auto &kv : input.named_parameters) if (kv.first == "seed") bd->seed = kv.second.GetValue<int64_t>();
  rt.push_back(LogicalType::BIGINT); nm.push_back("src");
  rt.push_back(LogicalType::BIGINT); nm.push_back("dst");
  return std::move(bd);
}
static unique_ptr<GlobalTableFunctionState> WattsStrogatzInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) { return make_uniq<WattsStrogatzGlobalState>(); }
static void WattsStrogatzFunction(ClientContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<WattsStrogatzBindData>(); auto &gs = data.global_state->Cast<WattsStrogatzGlobalState>();
  if (!gs.computed) {
    int64_t ec = ::onager::onager_generate_watts_strogatz(static_cast<size_t>(bd.n), static_cast<size_t>(bd.k), bd.beta, static_cast<uint64_t>(bd.seed), nullptr, nullptr);
    if (ec < 0) throw InvalidInputException("Watts-Strogatz failed: " + GetOnagerError());
    gs.result_src.resize(ec); gs.result_dst.resize(ec);
    ::onager::onager_generate_watts_strogatz(static_cast<size_t>(bd.n), static_cast<size_t>(bd.k), bd.beta, static_cast<uint64_t>(bd.seed), gs.result_src.data(), gs.result_dst.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_src.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto s = FlatVector::GetData<int64_t>(output.data[0]); auto d = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to; i++) { s[i] = gs.result_src[gs.output_idx+i]; d[i] = gs.result_dst[gs.output_idx+i]; }
  gs.output_idx += to; output.SetCardinality(to);
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterGeneratorFunctions(ExtensionLoader &loader) {
  TableFunction erdos_renyi("onager_gen_erdos_renyi", {LogicalType::BIGINT, LogicalType::DOUBLE}, ErdosRenyiFunction, ErdosRenyiBind, ErdosRenyiInitGlobal);
  erdos_renyi.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(erdos_renyi);

  TableFunction barabasi_albert("onager_gen_barabasi_albert", {LogicalType::BIGINT, LogicalType::BIGINT}, BarabasiAlbertFunction, BarabasiAlbertBind, BarabasiAlbertInitGlobal);
  barabasi_albert.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(barabasi_albert);

  TableFunction watts_strogatz("onager_gen_watts_strogatz", {LogicalType::BIGINT, LogicalType::BIGINT, LogicalType::DOUBLE}, WattsStrogatzFunction, WattsStrogatzBind, WattsStrogatzInitGlobal);
  watts_strogatz.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(watts_strogatz);
}

} // namespace onager
} // namespace duckdb
