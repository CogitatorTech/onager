/**
 * @file personalized.cpp
 * @brief Personalized PageRank table functions for Onager DuckDB extension.
 */
#include "functions.hpp"
#include <mutex>

namespace duckdb {

using namespace onager;

// =============================================================================
// Personalized PageRank
// =============================================================================

struct PersonalizedPageRankBindData : public TableFunctionData {
  double damping = 0.85;
  int64_t max_iter = 100;
  double tolerance = 1e-6;
  bool directed = false;
};
struct PersonalizedPageRankGlobalState : public GlobalTableFunctionState {
  std::mutex input_mutex;
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
    if (kv.first == "directed") bd->directed = kv.second.GetValue<bool>();
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
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  UnifiedVectorFormat s_data, d_data, pn_data, pw_data;
  ONAGER_TO_UNIFIED_FORMAT(input.data[0], input.size(), s_data);
  ONAGER_TO_UNIFIED_FORMAT(input.data[1], input.size(), d_data);
  ONAGER_TO_UNIFIED_FORMAT(input.data[2], input.size(), pn_data);
  ONAGER_TO_UNIFIED_FORMAT(input.data[3], input.size(), pw_data);

  auto s = UnifiedVectorFormat::GetData<int64_t>(s_data);
  auto d = UnifiedVectorFormat::GetData<int64_t>(d_data);
  auto pn = UnifiedVectorFormat::GetData<int64_t>(pn_data);
  auto pw = UnifiedVectorFormat::GetData<double>(pw_data);

  for (idx_t i = 0; i < input.size(); i++) {
    auto s_idx = s_data.sel->get_index(i);
    auto d_idx = d_data.sel->get_index(i);
    auto pn_idx = pn_data.sel->get_index(i);
    auto pw_idx = pw_data.sel->get_index(i);

    if (!s_data.validity.RowIsValid(s_idx) || !d_data.validity.RowIsValid(d_idx)) {
      throw InvalidInputException("onager_ctr_personalized_pagerank does not accept NULL edge endpoints");
    }

    gs.src_nodes.push_back(s[s_idx]);
    gs.dst_nodes.push_back(d[d_idx]);

    if (pn_data.validity.RowIsValid(pn_idx)) {
      gs.pers_nodes.push_back(pn[pn_idx]);
    } else {
      gs.pers_nodes.push_back(0);
    }

    if (pw_data.validity.RowIsValid(pw_idx)) {
      gs.pers_weights.push_back(pw[pw_idx]);
    } else {
      gs.pers_weights.push_back(0.0);
    }
  }
  ONAGER_SET_CARDINALITY(output, 0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType PersonalizedPageRankFinal(ExecutionContext &ctx, TableFunctionInput &data, DataChunk &output) {
  auto &bd = data.bind_data->Cast<PersonalizedPageRankBindData>();
  auto &gs = data.global_state->Cast<PersonalizedPageRankGlobalState>();
  std::lock_guard<std::mutex> lock(gs.input_mutex);
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
    int64_t nc = ::onager::onager_compute_personalized_pagerank(
      gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(),
      gs.pers_nodes.data(), gs.pers_weights.data(), gs.pers_nodes.size(),
      bd.damping, bd.max_iter, bd.tolerance, bd.directed, nullptr, nullptr);
    if (nc < 0) throw InvalidInputException("Personalized PageRank failed: " + GetOnagerError());
    gs.result_nodes.resize(nc); gs.result_scores.resize(nc);
    ::onager::onager_compute_personalized_pagerank(
      gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(),
      gs.pers_nodes.data(), gs.pers_weights.data(), gs.pers_nodes.size(),
      bd.damping, bd.max_iter, bd.tolerance, bd.directed, gs.result_nodes.data(), gs.result_scores.data());
    gs.computed = true;
  }
  idx_t rem = gs.result_nodes.size() - gs.output_idx;
  if (rem == 0) { ONAGER_SET_CARDINALITY(output, 0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to = MinValue<idx_t>(rem, STANDARD_VECTOR_SIZE);
  auto n = GetFlatVectorDataWritable<int64_t>(output.data[0]);
  auto sc = GetFlatVectorDataWritable<double>(output.data[1]);
  for (idx_t i = 0; i < to; i++) {
    n[i] = gs.result_nodes[gs.output_idx+i];
    sc[i] = gs.result_scores[gs.output_idx+i];
  }
  gs.output_idx += to; ONAGER_SET_CARDINALITY(output, to);
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
  pers_pr.named_parameters["directed"] = LogicalType::BOOLEAN;
  ONAGER_SET_NO_ORDER(pers_pr);
  loader.RegisterFunction(pers_pr);
}

} // namespace onager
} // namespace duckdb
