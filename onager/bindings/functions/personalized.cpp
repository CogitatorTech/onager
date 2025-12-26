/**
 * @file personalized.cpp
 * @brief Personalized PageRank table functions for Onager DuckDB extension.
 */
#include "functions.hpp"

namespace duckdb {

using namespace onager;

// =============================================================================
// Personalized PageRank
// =============================================================================

struct PersonalizedPageRankBindData : public TableFunctionData {
  double damping = 0.85;
  int64_t max_iter = 100;
  double tolerance = 1e-6;
};
struct PersonalizedPageRankGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, pers_nodes, result_nodes;
  std::vector<double> pers_weights, result_scores;
  idx_t output_idx = 0; bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> PersonalizedPageRankBind(ClientContext &ctx, TableFunctionBindInput &input, vector<LogicalType> &rt, vector<string> &nm) {
  auto bd = make_uniq<PersonalizedPageRankBindData>();
  CheckInt64Input(input, "onager_ctr_personalized_pagerank", 4);
  for (auto &kv : input.named_parameters) {
    if (kv.first == "damping") bd->damping = kv.second.GetValue<double>();
    if (kv.first == "max_iter") bd->max_iter = kv.second.GetValue<int64_t>();
    if (kv.first == "tolerance") bd->tolerance = kv.second.GetValue<double>();
  }
  rt.push_back(LogicalType::BIGINT); nm.push_back("node_id");
  rt.push_back(LogicalType::DOUBLE); nm.push_back("score");
  return std::move(bd);
}

static unique_ptr<GlobalTableFunctionState> PersonalizedPageRankInitGlobal(ClientContext &ctx, TableFunctionInitInput &input) {
  return make_uniq<PersonalizedPageRankGlobalState>();
}

static OperatorResultType PersonalizedPageRankInOut(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<PersonalizedPageRankGlobalState>();
  auto s = FlatVector::GetData<int64_t>(input.data[0]);
  auto d = FlatVector::GetData<int64_t>(input.data[1]);
  auto pn = FlatVector::GetData<int64_t>(input.data[2]);
  auto pw = FlatVector::GetData<double>(input.data[3]);
  for (idx_t i = 0; i < input.size(); i++) {
    gs.src_nodes.push_back(s[i]);
    gs.dst_nodes.push_back(d[i]);
    gs.pers_nodes.push_back(pn[i]);
    gs.pers_weights.push_back(pw[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType PersonalizedPageRankFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<PersonalizedPageRankBindData>();
  auto &gs = data.global_state->Cast<PersonalizedPageRankGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_personalized_pagerank(
      gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(),
      gs.pers_nodes.data(), gs.pers_weights.data(), gs.pers_nodes.size(),
      bd.damping, bd.max_iter, bd.tolerance, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Personalized PageRank failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_scores.resize(nc);
    ::onager::onager_compute_personalized_pagerank(
      gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(),
      gs.pers_nodes.data(), gs.pers_weights.data(), gs.pers_nodes.size(),
      bd.damping, bd.max_iter, bd.tolerance, gs.result_nodes.data(), gs.result_scores.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = FlatVector::GetData<int64_t>(output.data[0]);
  auto sc = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) {
    n[i] = gs.result_nodes[gs.output_idx+i];
    sc[i] = gs.result_scores[gs.output_idx+i];
  }
  gs.output_idx += to; output.SetCardinality(to);
  return gs.output_idx >= gs.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Registration
// =============================================================================

namespace onager {

void RegisterPersonalizedFunctions(ExtensionLoader &loader) {
  TableFunction pers_pr("onager_ctr_personalized_pagerank", {LogicalType::TABLE}, nullptr, PersonalizedPageRankBind, PersonalizedPageRankInitGlobal);
  pers_pr.in_out_function = PersonalizedPageRankInOut;
  pers_pr.in_out_function_final = PersonalizedPageRankFinal;
  pers_pr.named_parameters["damping"] = LogicalType::DOUBLE;
  pers_pr.named_parameters["max_iter"] = LogicalType::BIGINT;
  pers_pr.named_parameters["tolerance"] = LogicalType::DOUBLE;
  loader.RegisterFunction(pers_pr);
}

} // namespace onager
} // namespace duckdb
