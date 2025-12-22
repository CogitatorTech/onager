/**
 * @file onager_extension.cpp
 * @brief This file contains the implementation of the Onager DuckDB extension.
 *
 * Onager is a graph data analytics extension for DuckDB, powered by the
 * graphina library. It provides SQL functions for graph analysis including
 * centrality measures, community detection, and path finding.
 *
 * API: Functions accept edge tables as input with (src, dst) or (src, dst, weight) columns.
 */
#define DUCKDB_EXTENSION_MAIN

#include "include/onager_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/types/data_chunk.hpp"
#include "duckdb/common/types/value.hpp"
#include "duckdb/common/types/vector.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rust.h"

namespace duckdb {

using namespace onager;

/**
 * @brief Retrieves the last error message from the Onager Rust core.
 */
static std::string GetOnagerError() {
  const char *err = onager_last_error();
  return err ? std::string(err) : std::string("unknown error");
}

// =============================================================================
// Scalar Functions (Utility)
// =============================================================================

static void GetVersion(DataChunk &args, ExpressionState &state, Vector &result) {
  char *version_c = onager_get_version();
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  ConstantVector::GetData<string_t>(result)[0] = StringVector::AddString(result, version_c);
  ConstantVector::SetNull(result, false);
  onager_free(version_c);
}

static void GetLastError(DataChunk &args, ExpressionState &state, Vector &result) {
  result.SetVectorType(VectorType::CONSTANT_VECTOR);
  const char *err = onager_last_error();
  if (!err) {
    ConstantVector::SetNull(result, true);
    return;
  }
  ConstantVector::GetData<string_t>(result)[0] = StringVector::AddString(result, err);
  ConstantVector::SetNull(result, false);
}

// =============================================================================
// PageRank Table Function (Table Input)
// =============================================================================

struct PageRankBindData : public TableFunctionData {
  double damping = 0.85;
  int64_t iterations = 100;
  bool directed = true;
};

struct PageRankGlobalState : public GlobalTableFunctionState {
  // Edge collection
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;

  // Results
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

  // Validate input table has at least 2 columns (src, dst)
  if (input.input_table_types.size() < 2) {
    throw InvalidInputException("onager_pagerank requires a table with at least 2 columns: (src, dst)");
  }

  // Parse named parameters
  for (auto &kv : input.named_parameters) {
    if (kv.first == "damping") {
      bind_data->damping = kv.second.GetValue<double>();
    } else if (kv.first == "iterations") {
      bind_data->iterations = kv.second.GetValue<int64_t>();
    } else if (kv.first == "directed") {
      bind_data->directed = kv.second.GetValue<bool>();
    }
  }

  // Return types: (node_id BIGINT, rank DOUBLE)
  return_types.push_back(LogicalType::BIGINT);
  names.push_back("node_id");

  return_types.push_back(LogicalType::DOUBLE);
  names.push_back("rank");

  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> PageRankInitGlobal(ClientContext &context,
                                                                TableFunctionInitInput &input) {
  return make_uniq<PageRankGlobalState>();
}

// In-out function: collect edges from input
static OperatorResultType PageRankInOut(ExecutionContext &context, TableFunctionInput &data,
                                         DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<PageRankGlobalState>();

  // Collect edges from input chunk
  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);

  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }

  // Don't output anything yet - wait for all input
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

// Final function: compute and output results
static OperatorFinalizeResultType PageRankFinal(ExecutionContext &context, TableFunctionInput &data,
                                                 DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<PageRankBindData>();
  auto &global_state = data.global_state->Cast<PageRankGlobalState>();

  // Compute PageRank if not yet done
  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) {
      global_state.computed = true;
      output.SetCardinality(0);
      return OperatorFinalizeResultType::FINISHED;
    }

    size_t edge_count = global_state.src_nodes.size();

    // First call to get node count
    int64_t node_count = onager_compute_pagerank(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        bind_data.damping,
        static_cast<size_t>(bind_data.iterations),
        bind_data.directed,
        nullptr,
        nullptr
    );

    if (node_count < 0) {
      throw InvalidInputException("PageRank computation failed: " + GetOnagerError());
    }

    // Allocate and compute
    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_ranks.resize(static_cast<size_t>(node_count));

    int64_t result = onager_compute_pagerank(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        bind_data.damping,
        static_cast<size_t>(bind_data.iterations),
        bind_data.directed,
        global_state.result_nodes.data(),
        global_state.result_ranks.data()
    );

    if (result < 0) {
      throw InvalidInputException("PageRank computation failed: " + GetOnagerError());
    }

    global_state.computed = true;
  }

  // Output results
  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) {
    output.SetCardinality(0);
    return OperatorFinalizeResultType::FINISHED;
  }

  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);

  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto rank_data = FlatVector::GetData<double>(output.data[1]);

  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    rank_data[i] = global_state.result_ranks[global_state.output_idx + i];
  }

  global_state.output_idx += to_output;
  output.SetCardinality(to_output);

  return global_state.output_idx >= global_state.result_nodes.size()
      ? OperatorFinalizeResultType::FINISHED
      : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Degree Centrality Table Function
// =============================================================================

struct DegreeBindData : public TableFunctionData {
  bool directed = true;
};

struct DegreeGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<double> result_in_degree;
  std::vector<double> result_out_degree;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DegreeBind(ClientContext &context,
                                            TableFunctionBindInput &input,
                                            vector<LogicalType> &return_types,
                                            vector<string> &names) {
  auto bind_data = make_uniq<DegreeBindData>();

  if (input.input_table_types.size() < 2) {
    throw InvalidInputException("onager_degree requires a table with at least 2 columns: (src, dst)");
  }

  for (auto &kv : input.named_parameters) {
    if (kv.first == "directed") {
      bind_data->directed = kv.second.GetValue<bool>();
    }
  }

  return_types.push_back(LogicalType::BIGINT);
  names.push_back("node_id");

  return_types.push_back(LogicalType::DOUBLE);
  names.push_back("in_degree");

  return_types.push_back(LogicalType::DOUBLE);
  names.push_back("out_degree");

  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> DegreeInitGlobal(ClientContext &context,
                                                              TableFunctionInitInput &input) {
  return make_uniq<DegreeGlobalState>();
}

static OperatorResultType DegreeInOut(ExecutionContext &context, TableFunctionInput &data,
                                       DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<DegreeGlobalState>();

  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);

  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }

  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType DegreeFinal(ExecutionContext &context, TableFunctionInput &data,
                                               DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<DegreeBindData>();
  auto &global_state = data.global_state->Cast<DegreeGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) {
      global_state.computed = true;
      output.SetCardinality(0);
      return OperatorFinalizeResultType::FINISHED;
    }

    size_t edge_count = global_state.src_nodes.size();

    int64_t node_count = onager_compute_degree(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        bind_data.directed,
        nullptr, nullptr, nullptr
    );

    if (node_count < 0) {
      throw InvalidInputException("Degree computation failed: " + GetOnagerError());
    }

    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_in_degree.resize(static_cast<size_t>(node_count));
    global_state.result_out_degree.resize(static_cast<size_t>(node_count));

    int64_t result = onager_compute_degree(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        bind_data.directed,
        global_state.result_nodes.data(),
        global_state.result_in_degree.data(),
        global_state.result_out_degree.data()
    );

    if (result < 0) {
      throw InvalidInputException("Degree computation failed: " + GetOnagerError());
    }

    global_state.computed = true;
  }

  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) {
    output.SetCardinality(0);
    return OperatorFinalizeResultType::FINISHED;
  }

  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);

  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto in_data = FlatVector::GetData<double>(output.data[1]);
  auto out_data = FlatVector::GetData<double>(output.data[2]);

  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    in_data[i] = global_state.result_in_degree[global_state.output_idx + i];
    out_data[i] = global_state.result_out_degree[global_state.output_idx + i];
  }

  global_state.output_idx += to_output;
  output.SetCardinality(to_output);

  return global_state.output_idx >= global_state.result_nodes.size()
      ? OperatorFinalizeResultType::FINISHED
      : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Betweenness Centrality Table Function
// =============================================================================

struct BetweennessBindData : public TableFunctionData {
  bool normalized = true;
};

struct BetweennessGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> BetweennessBind(ClientContext &context,
                                                 TableFunctionBindInput &input,
                                                 vector<LogicalType> &return_types,
                                                 vector<string> &names) {
  auto bind_data = make_uniq<BetweennessBindData>();

  if (input.input_table_types.size() < 2) {
    throw InvalidInputException("onager_betweenness requires a table with at least 2 columns: (src, dst)");
  }

  for (auto &kv : input.named_parameters) {
    if (kv.first == "normalized") {
      bind_data->normalized = kv.second.GetValue<bool>();
    }
  }

  return_types.push_back(LogicalType::BIGINT);
  names.push_back("node_id");

  return_types.push_back(LogicalType::DOUBLE);
  names.push_back("betweenness");

  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> BetweennessInitGlobal(ClientContext &context,
                                                                   TableFunctionInitInput &input) {
  return make_uniq<BetweennessGlobalState>();
}

static OperatorResultType BetweennessInOut(ExecutionContext &context, TableFunctionInput &data,
                                            DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<BetweennessGlobalState>();

  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);

  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }

  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType BetweennessFinal(ExecutionContext &context, TableFunctionInput &data,
                                                    DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<BetweennessBindData>();
  auto &global_state = data.global_state->Cast<BetweennessGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) {
      global_state.computed = true;
      output.SetCardinality(0);
      return OperatorFinalizeResultType::FINISHED;
    }

    size_t edge_count = global_state.src_nodes.size();

    int64_t node_count = onager_compute_betweenness(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        bind_data.normalized,
        nullptr, nullptr
    );

    if (node_count < 0) {
      throw InvalidInputException("Betweenness computation failed: " + GetOnagerError());
    }

    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_centralities.resize(static_cast<size_t>(node_count));

    int64_t result = onager_compute_betweenness(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        bind_data.normalized,
        global_state.result_nodes.data(),
        global_state.result_centralities.data()
    );

    if (result < 0) {
      throw InvalidInputException("Betweenness computation failed: " + GetOnagerError());
    }

    global_state.computed = true;
  }

  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) {
    output.SetCardinality(0);
    return OperatorFinalizeResultType::FINISHED;
  }

  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);

  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto cent_data = FlatVector::GetData<double>(output.data[1]);

  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    cent_data[i] = global_state.result_centralities[global_state.output_idx + i];
  }

  global_state.output_idx += to_output;
  output.SetCardinality(to_output);

  return global_state.output_idx >= global_state.result_nodes.size()
      ? OperatorFinalizeResultType::FINISHED
      : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Closeness Centrality Table Function
// =============================================================================

struct ClosenessGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ClosenessBind(ClientContext &context,
                                               TableFunctionBindInput &input,
                                               vector<LogicalType> &return_types,
                                               vector<string> &names) {
  if (input.input_table_types.size() < 2) {
    throw InvalidInputException("onager_closeness requires a table with at least 2 columns: (src, dst)");
  }

  return_types.push_back(LogicalType::BIGINT);
  names.push_back("node_id");

  return_types.push_back(LogicalType::DOUBLE);
  names.push_back("closeness");

  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> ClosenessInitGlobal(ClientContext &context,
                                                                 TableFunctionInitInput &input) {
  return make_uniq<ClosenessGlobalState>();
}

static OperatorResultType ClosenessInOut(ExecutionContext &context, TableFunctionInput &data,
                                          DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<ClosenessGlobalState>();

  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);

  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }

  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType ClosenessFinal(ExecutionContext &context, TableFunctionInput &data,
                                                  DataChunk &output) {
  auto &global_state = data.global_state->Cast<ClosenessGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) {
      global_state.computed = true;
      output.SetCardinality(0);
      return OperatorFinalizeResultType::FINISHED;
    }

    size_t edge_count = global_state.src_nodes.size();

    int64_t node_count = onager_compute_closeness(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        nullptr, nullptr
    );

    if (node_count < 0) {
      throw InvalidInputException("Closeness computation failed: " + GetOnagerError());
    }

    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_centralities.resize(static_cast<size_t>(node_count));

    int64_t result = onager_compute_closeness(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        global_state.result_nodes.data(),
        global_state.result_centralities.data()
    );

    if (result < 0) {
      throw InvalidInputException("Closeness computation failed: " + GetOnagerError());
    }

    global_state.computed = true;
  }

  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) {
    output.SetCardinality(0);
    return OperatorFinalizeResultType::FINISHED;
  }

  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);

  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto cent_data = FlatVector::GetData<double>(output.data[1]);

  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    cent_data[i] = global_state.result_centralities[global_state.output_idx + i];
  }

  global_state.output_idx += to_output;
  output.SetCardinality(to_output);

  return global_state.output_idx >= global_state.result_nodes.size()
      ? OperatorFinalizeResultType::FINISHED
      : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Louvain Community Detection Table Function
// =============================================================================

struct LouvainBindData : public TableFunctionData {
  int64_t seed = -1;  // -1 means random
};

struct LouvainGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<int64_t> result_communities;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> LouvainBind(ClientContext &context,
                                             TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types,
                                             vector<string> &names) {
  auto bind_data = make_uniq<LouvainBindData>();

  if (input.input_table_types.size() < 2) {
    throw InvalidInputException("onager_louvain requires a table with at least 2 columns: (src, dst)");
  }

  for (auto &kv : input.named_parameters) {
    if (kv.first == "seed") {
      bind_data->seed = kv.second.GetValue<int64_t>();
    }
  }

  return_types.push_back(LogicalType::BIGINT);
  names.push_back("node_id");

  return_types.push_back(LogicalType::BIGINT);
  names.push_back("community");

  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> LouvainInitGlobal(ClientContext &context,
                                                               TableFunctionInitInput &input) {
  return make_uniq<LouvainGlobalState>();
}

static OperatorResultType LouvainInOut(ExecutionContext &context, TableFunctionInput &data,
                                        DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<LouvainGlobalState>();

  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);

  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }

  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType LouvainFinal(ExecutionContext &context, TableFunctionInput &data,
                                                DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<LouvainBindData>();
  auto &global_state = data.global_state->Cast<LouvainGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) {
      global_state.computed = true;
      output.SetCardinality(0);
      return OperatorFinalizeResultType::FINISHED;
    }

    size_t edge_count = global_state.src_nodes.size();

    int64_t node_count = onager_compute_louvain(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        bind_data.seed,
        nullptr, nullptr
    );

    if (node_count < 0) {
      throw InvalidInputException("Louvain computation failed: " + GetOnagerError());
    }

    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_communities.resize(static_cast<size_t>(node_count));

    int64_t result = onager_compute_louvain(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        bind_data.seed,
        global_state.result_nodes.data(),
        global_state.result_communities.data()
    );

    if (result < 0) {
      throw InvalidInputException("Louvain computation failed: " + GetOnagerError());
    }

    global_state.computed = true;
  }

  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) {
    output.SetCardinality(0);
    return OperatorFinalizeResultType::FINISHED;
  }

  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);

  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto comm_data = FlatVector::GetData<int64_t>(output.data[1]);

  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    comm_data[i] = global_state.result_communities[global_state.output_idx + i];
  }

  global_state.output_idx += to_output;
  output.SetCardinality(to_output);

  return global_state.output_idx >= global_state.result_nodes.size()
      ? OperatorFinalizeResultType::FINISHED
      : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Connected Components Table Function
// =============================================================================

struct ComponentsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<int64_t> result_components;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ComponentsBind(ClientContext &context,
                                                TableFunctionBindInput &input,
                                                vector<LogicalType> &return_types,
                                                vector<string> &names) {
  if (input.input_table_types.size() < 2) {
    throw InvalidInputException("onager_components requires a table with at least 2 columns: (src, dst)");
  }

  return_types.push_back(LogicalType::BIGINT);
  names.push_back("node_id");

  return_types.push_back(LogicalType::BIGINT);
  names.push_back("component");

  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> ComponentsInitGlobal(ClientContext &context,
                                                                  TableFunctionInitInput &input) {
  return make_uniq<ComponentsGlobalState>();
}

static OperatorResultType ComponentsInOut(ExecutionContext &context, TableFunctionInput &data,
                                           DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<ComponentsGlobalState>();

  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);

  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }

  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType ComponentsFinal(ExecutionContext &context, TableFunctionInput &data,
                                                   DataChunk &output) {
  auto &global_state = data.global_state->Cast<ComponentsGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) {
      global_state.computed = true;
      output.SetCardinality(0);
      return OperatorFinalizeResultType::FINISHED;
    }

    size_t edge_count = global_state.src_nodes.size();

    int64_t node_count = onager_compute_connected_components(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        nullptr, nullptr
    );

    if (node_count < 0) {
      throw InvalidInputException("Connected components computation failed: " + GetOnagerError());
    }

    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_components.resize(static_cast<size_t>(node_count));

    int64_t result = onager_compute_connected_components(
        global_state.src_nodes.data(),
        global_state.dst_nodes.data(),
        edge_count,
        global_state.result_nodes.data(),
        global_state.result_components.data()
    );

    if (result < 0) {
      throw InvalidInputException("Connected components computation failed: " + GetOnagerError());
    }

    global_state.computed = true;
  }

  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) {
    output.SetCardinality(0);
    return OperatorFinalizeResultType::FINISHED;
  }

  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);

  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto comp_data = FlatVector::GetData<int64_t>(output.data[1]);

  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    comp_data[i] = global_state.result_components[global_state.output_idx + i];
  }

  global_state.output_idx += to_output;
  output.SetCardinality(to_output);

  return global_state.output_idx >= global_state.result_nodes.size()
      ? OperatorFinalizeResultType::FINISHED
      : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Eigenvector Centrality Table Function
// =============================================================================

struct EigenvectorBindData : public TableFunctionData {
  size_t max_iter = 100;
  double tolerance = 1e-6;
};

struct EigenvectorGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> EigenvectorBind(ClientContext &context,
                                                 TableFunctionBindInput &input,
                                                 vector<LogicalType> &return_types,
                                                 vector<string> &names) {
  auto bind_data = make_uniq<EigenvectorBindData>();

  if (input.input_table_types.size() < 2) {
    throw InvalidInputException("onager_ctr_eigenvector requires a table with at least 2 columns: (src, dst)");
  }

  for (auto &kv : input.named_parameters) {
    if (kv.first == "max_iter") {
      bind_data->max_iter = static_cast<size_t>(kv.second.GetValue<int64_t>());
    } else if (kv.first == "tolerance") {
      bind_data->tolerance = kv.second.GetValue<double>();
    }
  }

  return_types.push_back(LogicalType::BIGINT);
  names.push_back("node_id");
  return_types.push_back(LogicalType::DOUBLE);
  names.push_back("eigenvector");

  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> EigenvectorInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<EigenvectorGlobalState>();
}

static OperatorResultType EigenvectorInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<EigenvectorGlobalState>();
  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType EigenvectorFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<EigenvectorBindData>();
  auto &global_state = data.global_state->Cast<EigenvectorGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) {
      global_state.computed = true;
      output.SetCardinality(0);
      return OperatorFinalizeResultType::FINISHED;
    }

    size_t edge_count = global_state.src_nodes.size();
    int64_t node_count = onager_compute_eigenvector(
        global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count,
        bind_data.max_iter, bind_data.tolerance, nullptr, nullptr);
    if (node_count < 0) throw InvalidInputException("Eigenvector computation failed: " + GetOnagerError());

    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_centralities.resize(static_cast<size_t>(node_count));
    onager_compute_eigenvector(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count,
        bind_data.max_iter, bind_data.tolerance, global_state.result_nodes.data(), global_state.result_centralities.data());
    global_state.computed = true;
  }

  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto cent_data = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    cent_data[i] = global_state.result_centralities[global_state.output_idx + i];
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
  return global_state.output_idx >= global_state.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Katz Centrality Table Function
// =============================================================================

struct KatzBindData : public TableFunctionData {
  double alpha = 0.1;
  size_t max_iter = 100;
  double tolerance = 1e-6;
};

struct KatzGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> KatzBind(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
  auto bind_data = make_uniq<KatzBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_ctr_katz requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "alpha") bind_data->alpha = kv.second.GetValue<double>();
    else if (kv.first == "max_iter") bind_data->max_iter = static_cast<size_t>(kv.second.GetValue<int64_t>());
    else if (kv.first == "tolerance") bind_data->tolerance = kv.second.GetValue<double>();
  }
  return_types.push_back(LogicalType::BIGINT); names.push_back("node_id");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("katz");
  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> KatzInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<KatzGlobalState>();
}

static OperatorResultType KatzInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<KatzGlobalState>();
  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType KatzFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<KatzBindData>();
  auto &global_state = data.global_state->Cast<KatzGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) { global_state.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    size_t edge_count = global_state.src_nodes.size();
    int64_t node_count = onager_compute_katz(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count,
        bind_data.alpha, bind_data.max_iter, bind_data.tolerance, nullptr, nullptr);
    if (node_count < 0) throw InvalidInputException("Katz computation failed: " + GetOnagerError());
    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_centralities.resize(static_cast<size_t>(node_count));
    onager_compute_katz(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count,
        bind_data.alpha, bind_data.max_iter, bind_data.tolerance, global_state.result_nodes.data(), global_state.result_centralities.data());
    global_state.computed = true;
  }

  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto cent_data = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    cent_data[i] = global_state.result_centralities[global_state.output_idx + i];
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
  return global_state.output_idx >= global_state.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Harmonic Centrality Table Function
// =============================================================================

struct HarmonicGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<double> result_centralities;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> HarmonicBind(ClientContext &context, TableFunctionBindInput &input,
                                              vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_ctr_harmonic requires 2 columns");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node_id");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("harmonic");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> HarmonicInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<HarmonicGlobalState>();
}

static OperatorResultType HarmonicInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<HarmonicGlobalState>();
  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType HarmonicFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &global_state = data.global_state->Cast<HarmonicGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) { global_state.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    size_t edge_count = global_state.src_nodes.size();
    int64_t node_count = onager_compute_harmonic(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count, nullptr, nullptr);
    if (node_count < 0) throw InvalidInputException("Harmonic computation failed: " + GetOnagerError());
    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_centralities.resize(static_cast<size_t>(node_count));
    onager_compute_harmonic(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count,
        global_state.result_nodes.data(), global_state.result_centralities.data());
    global_state.computed = true;
  }

  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto cent_data = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    cent_data[i] = global_state.result_centralities[global_state.output_idx + i];
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
  return global_state.output_idx >= global_state.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Dijkstra Shortest Path Table Function
// =============================================================================

struct DijkstraBindData : public TableFunctionData {
  int64_t source = 0;
};

struct DijkstraGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_nodes;
  std::vector<double> result_distances;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DijkstraBind(ClientContext &context, TableFunctionBindInput &input,
                                              vector<LogicalType> &return_types, vector<string> &names) {
  auto bind_data = make_uniq<DijkstraBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_pth_dijkstra requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "source") bind_data->source = kv.second.GetValue<int64_t>();
  }
  return_types.push_back(LogicalType::BIGINT); names.push_back("node_id");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("distance");
  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> DijkstraInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<DijkstraGlobalState>();
}

static OperatorResultType DijkstraInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<DijkstraGlobalState>();
  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType DijkstraFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<DijkstraBindData>();
  auto &global_state = data.global_state->Cast<DijkstraGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) { global_state.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    size_t edge_count = global_state.src_nodes.size();
    int64_t node_count = onager_compute_dijkstra(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count,
        bind_data.source, nullptr, nullptr);
    if (node_count < 0) throw InvalidInputException("Dijkstra computation failed: " + GetOnagerError());
    global_state.result_nodes.resize(static_cast<size_t>(node_count));
    global_state.result_distances.resize(static_cast<size_t>(node_count));
    onager_compute_dijkstra(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count,
        bind_data.source, global_state.result_nodes.data(), global_state.result_distances.data());
    global_state.computed = true;
  }

  idx_t remaining = global_state.result_nodes.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto dist_data = FlatVector::GetData<double>(output.data[1]);
  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_nodes[global_state.output_idx + i];
    dist_data[i] = global_state.result_distances[global_state.output_idx + i];
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
  return global_state.output_idx >= global_state.result_nodes.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// BFS Traversal Table Function
// =============================================================================

struct BfsBindData : public TableFunctionData {
  int64_t source = 0;
};

struct BfsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_order;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> BfsBind(ClientContext &context, TableFunctionBindInput &input,
                                         vector<LogicalType> &return_types, vector<string> &names) {
  auto bind_data = make_uniq<BfsBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_trv_bfs requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "source") bind_data->source = kv.second.GetValue<int64_t>();
  }
  return_types.push_back(LogicalType::BIGINT); names.push_back("node_id");
  return_types.push_back(LogicalType::BIGINT); names.push_back("order");
  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> BfsInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<BfsGlobalState>();
}

static OperatorResultType BfsInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<BfsGlobalState>();
  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType BfsFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<BfsBindData>();
  auto &global_state = data.global_state->Cast<BfsGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) { global_state.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    size_t edge_count = global_state.src_nodes.size();
    int64_t node_count = onager_compute_bfs(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count, bind_data.source, nullptr);
    if (node_count < 0) throw InvalidInputException("BFS computation failed: " + GetOnagerError());
    global_state.result_order.resize(static_cast<size_t>(node_count));
    onager_compute_bfs(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count, bind_data.source, global_state.result_order.data());
    global_state.computed = true;
  }

  idx_t remaining = global_state.result_order.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto order_data = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_order[global_state.output_idx + i];
    order_data[i] = static_cast<int64_t>(global_state.output_idx + i);
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
  return global_state.output_idx >= global_state.result_order.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// DFS Traversal Table Function
// =============================================================================

struct DfsBindData : public TableFunctionData {
  int64_t source = 0;
};

struct DfsGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_order;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DfsBind(ClientContext &context, TableFunctionBindInput &input,
                                         vector<LogicalType> &return_types, vector<string> &names) {
  auto bind_data = make_uniq<DfsBindData>();
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_trv_dfs requires 2 columns");
  for (auto &kv : input.named_parameters) {
    if (kv.first == "source") bind_data->source = kv.second.GetValue<int64_t>();
  }
  return_types.push_back(LogicalType::BIGINT); names.push_back("node_id");
  return_types.push_back(LogicalType::BIGINT); names.push_back("order");
  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> DfsInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<DfsGlobalState>();
}

static OperatorResultType DfsInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<DfsGlobalState>();
  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType DfsFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<DfsBindData>();
  auto &global_state = data.global_state->Cast<DfsGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) { global_state.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    size_t edge_count = global_state.src_nodes.size();
    int64_t node_count = onager_compute_dfs(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count, bind_data.source, nullptr);
    if (node_count < 0) throw InvalidInputException("DFS computation failed: " + GetOnagerError());
    global_state.result_order.resize(static_cast<size_t>(node_count));
    onager_compute_dfs(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count, bind_data.source, global_state.result_order.data());
    global_state.computed = true;
  }

  idx_t remaining = global_state.result_order.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto node_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto order_data = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to_output; i++) {
    node_data[i] = global_state.result_order[global_state.output_idx + i];
    order_data[i] = static_cast<int64_t>(global_state.output_idx + i);
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
  return global_state.output_idx >= global_state.result_order.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Jaccard Coefficient Table Function
// =============================================================================

struct JaccardGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  std::vector<int64_t> result_node1;
  std::vector<int64_t> result_node2;
  std::vector<double> result_coef;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> JaccardBind(ClientContext &context, TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_lnk_jaccard requires 2 columns");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node1");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node2");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("jaccard");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> JaccardInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<JaccardGlobalState>();
}

static OperatorResultType JaccardInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<JaccardGlobalState>();
  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType JaccardFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &global_state = data.global_state->Cast<JaccardGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) { global_state.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    size_t edge_count = global_state.src_nodes.size();
    int64_t pair_count = onager_compute_jaccard(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count, nullptr, nullptr, nullptr);
    if (pair_count < 0) throw InvalidInputException("Jaccard computation failed: " + GetOnagerError());
    global_state.result_node1.resize(static_cast<size_t>(pair_count));
    global_state.result_node2.resize(static_cast<size_t>(pair_count));
    global_state.result_coef.resize(static_cast<size_t>(pair_count));
    onager_compute_jaccard(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count,
        global_state.result_node1.data(), global_state.result_node2.data(), global_state.result_coef.data());
    global_state.computed = true;
  }

  idx_t remaining = global_state.result_node1.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto n1_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto n2_data = FlatVector::GetData<int64_t>(output.data[1]);
  auto coef_data = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to_output; i++) {
    n1_data[i] = global_state.result_node1[global_state.output_idx + i];
    n2_data[i] = global_state.result_node2[global_state.output_idx + i];
    coef_data[i] = global_state.result_coef[global_state.output_idx + i];
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
  return global_state.output_idx >= global_state.result_node1.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Diameter Table Function
// =============================================================================

struct DiameterGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes;
  std::vector<int64_t> dst_nodes;
  int64_t diameter = -1;
  bool computed = false;
  bool returned = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> DiameterBind(ClientContext &context, TableFunctionBindInput &input,
                                              vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_mtr_diameter requires 2 columns");
  return_types.push_back(LogicalType::BIGINT); names.push_back("diameter");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> DiameterInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<DiameterGlobalState>();
}

static OperatorResultType DiameterInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &global_state = data.global_state->Cast<DiameterGlobalState>();
  auto src_vec = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst_vec = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) {
    global_state.src_nodes.push_back(src_vec[i]);
    global_state.dst_nodes.push_back(dst_vec[i]);
  }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType DiameterFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &global_state = data.global_state->Cast<DiameterGlobalState>();

  if (!global_state.computed) {
    if (global_state.src_nodes.empty()) { global_state.computed = true; global_state.returned = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    size_t edge_count = global_state.src_nodes.size();
    global_state.diameter = onager_compute_diameter(global_state.src_nodes.data(), global_state.dst_nodes.data(), edge_count);
    global_state.computed = true;
  }

  if (global_state.returned) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  auto diam_data = FlatVector::GetData<int64_t>(output.data[0]);
  diam_data[0] = global_state.diameter;
  output.SetCardinality(1);
  global_state.returned = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Erdős-Rényi Generator Table Function
// =============================================================================

struct ErdosRenyiBindData : public TableFunctionData {
  int64_t n;
  double p;
  uint64_t seed = 42;
};

struct ErdosRenyiGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src;
  std::vector<int64_t> dst;
  idx_t output_idx = 0;
  bool generated = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ErdosRenyiBind(ClientContext &context, TableFunctionBindInput &input,
                                                vector<LogicalType> &return_types, vector<string> &names) {
  auto bind_data = make_uniq<ErdosRenyiBindData>();
  bind_data->n = input.inputs[0].GetValue<int64_t>();
  bind_data->p = input.inputs[1].GetValue<double>();
  for (auto &kv : input.named_parameters) {
    if (kv.first == "seed") bind_data->seed = static_cast<uint64_t>(kv.second.GetValue<int64_t>());
  }
  return_types.push_back(LogicalType::BIGINT); names.push_back("src");
  return_types.push_back(LogicalType::BIGINT); names.push_back("dst");
  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> ErdosRenyiInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<ErdosRenyiGlobalState>();
}

static void ErdosRenyiFunction(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<ErdosRenyiBindData>();
  auto &global_state = data.global_state->Cast<ErdosRenyiGlobalState>();

  if (!global_state.generated) {
    int64_t edge_count = onager_generate_erdos_renyi(static_cast<size_t>(bind_data.n), bind_data.p, bind_data.seed, nullptr, nullptr);
    if (edge_count < 0) throw InvalidInputException("Erdős-Rényi generation failed: " + GetOnagerError());
    global_state.src.resize(static_cast<size_t>(edge_count));
    global_state.dst.resize(static_cast<size_t>(edge_count));
    onager_generate_erdos_renyi(static_cast<size_t>(bind_data.n), bind_data.p, bind_data.seed, global_state.src.data(), global_state.dst.data());
    global_state.generated = true;
  }

  idx_t remaining = global_state.src.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto src_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto dst_data = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to_output; i++) {
    src_data[i] = global_state.src[global_state.output_idx + i];
    dst_data[i] = global_state.dst[global_state.output_idx + i];
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
}

// =============================================================================
// Barabási-Albert Generator Table Function
// =============================================================================

struct BarabasiAlbertBindData : public TableFunctionData {
  int64_t n;
  int64_t m;
  uint64_t seed = 42;
};

struct BarabasiAlbertGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src;
  std::vector<int64_t> dst;
  idx_t output_idx = 0;
  bool generated = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> BarabasiAlbertBind(ClientContext &context, TableFunctionBindInput &input,
                                                    vector<LogicalType> &return_types, vector<string> &names) {
  auto bind_data = make_uniq<BarabasiAlbertBindData>();
  bind_data->n = input.inputs[0].GetValue<int64_t>();
  bind_data->m = input.inputs[1].GetValue<int64_t>();
  for (auto &kv : input.named_parameters) {
    if (kv.first == "seed") bind_data->seed = static_cast<uint64_t>(kv.second.GetValue<int64_t>());
  }
  return_types.push_back(LogicalType::BIGINT); names.push_back("src");
  return_types.push_back(LogicalType::BIGINT); names.push_back("dst");
  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> BarabasiAlbertInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<BarabasiAlbertGlobalState>();
}

static void BarabasiAlbertFunction(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<BarabasiAlbertBindData>();
  auto &global_state = data.global_state->Cast<BarabasiAlbertGlobalState>();

  if (!global_state.generated) {
    int64_t edge_count = onager_generate_barabasi_albert(static_cast<size_t>(bind_data.n), static_cast<size_t>(bind_data.m), bind_data.seed, nullptr, nullptr);
    if (edge_count < 0) throw InvalidInputException("Barabási-Albert generation failed: " + GetOnagerError());
    global_state.src.resize(static_cast<size_t>(edge_count));
    global_state.dst.resize(static_cast<size_t>(edge_count));
    onager_generate_barabasi_albert(static_cast<size_t>(bind_data.n), static_cast<size_t>(bind_data.m), bind_data.seed, global_state.src.data(), global_state.dst.data());
    global_state.generated = true;
  }

  idx_t remaining = global_state.src.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto src_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto dst_data = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to_output; i++) {
    src_data[i] = global_state.src[global_state.output_idx + i];
    dst_data[i] = global_state.dst[global_state.output_idx + i];
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
}

// =============================================================================
// Watts-Strogatz Generator Table Function
// =============================================================================

struct WattsStrogatzBindData : public TableFunctionData {
  int64_t n;
  int64_t k;
  double beta;
  uint64_t seed = 42;
};

struct WattsStrogatzGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src;
  std::vector<int64_t> dst;
  idx_t output_idx = 0;
  bool generated = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> WattsStrogatzBind(ClientContext &context, TableFunctionBindInput &input,
                                                   vector<LogicalType> &return_types, vector<string> &names) {
  auto bind_data = make_uniq<WattsStrogatzBindData>();
  bind_data->n = input.inputs[0].GetValue<int64_t>();
  bind_data->k = input.inputs[1].GetValue<int64_t>();
  bind_data->beta = input.inputs[2].GetValue<double>();
  for (auto &kv : input.named_parameters) {
    if (kv.first == "seed") bind_data->seed = static_cast<uint64_t>(kv.second.GetValue<int64_t>());
  }
  return_types.push_back(LogicalType::BIGINT); names.push_back("src");
  return_types.push_back(LogicalType::BIGINT); names.push_back("dst");
  return std::move(bind_data);
}

static unique_ptr<GlobalTableFunctionState> WattsStrogatzInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<WattsStrogatzGlobalState>();
}

static void WattsStrogatzFunction(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &bind_data = data.bind_data->Cast<WattsStrogatzBindData>();
  auto &global_state = data.global_state->Cast<WattsStrogatzGlobalState>();

  if (!global_state.generated) {
    int64_t edge_count = onager_generate_watts_strogatz(static_cast<size_t>(bind_data.n), static_cast<size_t>(bind_data.k), bind_data.beta, bind_data.seed, nullptr, nullptr);
    if (edge_count < 0) throw InvalidInputException("Watts-Strogatz generation failed: " + GetOnagerError());
    global_state.src.resize(static_cast<size_t>(edge_count));
    global_state.dst.resize(static_cast<size_t>(edge_count));
    onager_generate_watts_strogatz(static_cast<size_t>(bind_data.n), static_cast<size_t>(bind_data.k), bind_data.beta, bind_data.seed, global_state.src.data(), global_state.dst.data());
    global_state.generated = true;
  }

  idx_t remaining = global_state.src.size() - global_state.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return; }
  idx_t to_output = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto src_data = FlatVector::GetData<int64_t>(output.data[0]);
  auto dst_data = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to_output; i++) {
    src_data[i] = global_state.src[global_state.output_idx + i];
    dst_data[i] = global_state.dst[global_state.output_idx + i];
  }
  global_state.output_idx += to_output;
  output.SetCardinality(to_output);
}

// =============================================================================
// Adamic-Adar Link Prediction Table Function
// =============================================================================

struct AdamicAdarGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_n1, result_n2;
  std::vector<double> result_scores;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> AdamicAdarBind(ClientContext &context, TableFunctionBindInput &input,
                                                vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_lnk_adamic_adar requires 2 columns");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node1");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node2");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("score");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> AdamicAdarInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<AdamicAdarGlobalState>();
}

static OperatorResultType AdamicAdarInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AdamicAdarGlobalState>();
  auto src = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(src[i]); gs.dst_nodes.push_back(dst[i]); }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType AdamicAdarFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<AdamicAdarGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t count = onager_compute_adamic_adar(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr);
    if (count < 0) throw InvalidInputException("Adamic-Adar failed: " + GetOnagerError());
    gs.result_n1.resize(count); gs.result_n2.resize(count); gs.result_scores.resize(count);
    onager_compute_adamic_adar(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_n1.data(), gs.result_n2.data(), gs.result_scores.data());
    gs.computed = true;
  }
  idx_t remaining = gs.result_n1.size() - gs.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_out = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto n1 = FlatVector::GetData<int64_t>(output.data[0]);
  auto n2 = FlatVector::GetData<int64_t>(output.data[1]);
  auto sc = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to_out; i++) { n1[i] = gs.result_n1[gs.output_idx+i]; n2[i] = gs.result_n2[gs.output_idx+i]; sc[i] = gs.result_scores[gs.output_idx+i]; }
  gs.output_idx += to_out;
  output.SetCardinality(to_out);
  return gs.output_idx >= gs.result_n1.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Preferential Attachment Link Prediction Table Function
// =============================================================================

struct PrefAttachGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_n1, result_n2;
  std::vector<double> result_scores;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> PrefAttachBind(ClientContext &context, TableFunctionBindInput &input,
                                                vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_lnk_pref_attach requires 2 columns");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node1");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node2");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("score");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> PrefAttachInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<PrefAttachGlobalState>();
}

static OperatorResultType PrefAttachInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<PrefAttachGlobalState>();
  auto src = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(src[i]); gs.dst_nodes.push_back(dst[i]); }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType PrefAttachFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<PrefAttachGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t count = onager_compute_preferential_attachment(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr);
    if (count < 0) throw InvalidInputException("Preferential attachment failed: " + GetOnagerError());
    gs.result_n1.resize(count); gs.result_n2.resize(count); gs.result_scores.resize(count);
    onager_compute_preferential_attachment(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_n1.data(), gs.result_n2.data(), gs.result_scores.data());
    gs.computed = true;
  }
  idx_t remaining = gs.result_n1.size() - gs.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_out = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto n1 = FlatVector::GetData<int64_t>(output.data[0]);
  auto n2 = FlatVector::GetData<int64_t>(output.data[1]);
  auto sc = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to_out; i++) { n1[i] = gs.result_n1[gs.output_idx+i]; n2[i] = gs.result_n2[gs.output_idx+i]; sc[i] = gs.result_scores[gs.output_idx+i]; }
  gs.output_idx += to_out;
  output.SetCardinality(to_out);
  return gs.output_idx >= gs.result_n1.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Resource Allocation Link Prediction Table Function
// =============================================================================

struct ResourceAllocGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_n1, result_n2;
  std::vector<double> result_scores;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> ResourceAllocBind(ClientContext &context, TableFunctionBindInput &input,
                                                   vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_lnk_resource_alloc requires 2 columns");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node1");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node2");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("score");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> ResourceAllocInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<ResourceAllocGlobalState>();
}

static OperatorResultType ResourceAllocInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<ResourceAllocGlobalState>();
  auto src = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(src[i]); gs.dst_nodes.push_back(dst[i]); }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType ResourceAllocFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<ResourceAllocGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t count = onager_compute_resource_allocation(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr, nullptr);
    if (count < 0) throw InvalidInputException("Resource allocation failed: " + GetOnagerError());
    gs.result_n1.resize(count); gs.result_n2.resize(count); gs.result_scores.resize(count);
    onager_compute_resource_allocation(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_n1.data(), gs.result_n2.data(), gs.result_scores.data());
    gs.computed = true;
  }
  idx_t remaining = gs.result_n1.size() - gs.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_out = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto n1 = FlatVector::GetData<int64_t>(output.data[0]);
  auto n2 = FlatVector::GetData<int64_t>(output.data[1]);
  auto sc = FlatVector::GetData<double>(output.data[2]);
  for (idx_t i = 0; i < to_out; i++) { n1[i] = gs.result_n1[gs.output_idx+i]; n2[i] = gs.result_n2[gs.output_idx+i]; sc[i] = gs.result_scores[gs.output_idx+i]; }
  gs.output_idx += to_out;
  output.SetCardinality(to_out);
  return gs.output_idx >= gs.result_n1.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Label Propagation Community Table Function
// =============================================================================

struct LabelPropGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes, result_ids, result_labels;
  idx_t output_idx = 0;
  bool computed = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> LabelPropBind(ClientContext &context, TableFunctionBindInput &input,
                                               vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_cmm_label_prop requires 2 columns");
  return_types.push_back(LogicalType::BIGINT); names.push_back("node_id");
  return_types.push_back(LogicalType::BIGINT); names.push_back("label");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> LabelPropInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<LabelPropGlobalState>();
}

static OperatorResultType LabelPropInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<LabelPropGlobalState>();
  auto src = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(src[i]); gs.dst_nodes.push_back(dst[i]); }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType LabelPropFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<LabelPropGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    int64_t count = onager_compute_label_propagation(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), nullptr, nullptr);
    if (count < 0) throw InvalidInputException("Label propagation failed: " + GetOnagerError());
    gs.result_ids.resize(count); gs.result_labels.resize(count);
    onager_compute_label_propagation(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size(), gs.result_ids.data(), gs.result_labels.data());
    gs.computed = true;
  }
  idx_t remaining = gs.result_ids.size() - gs.output_idx;
  if (remaining == 0) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  idx_t to_out = MinValue<idx_t>(remaining, STANDARD_VECTOR_SIZE);
  auto ids = FlatVector::GetData<int64_t>(output.data[0]);
  auto lbl = FlatVector::GetData<int64_t>(output.data[1]);
  for (idx_t i = 0; i < to_out; i++) { ids[i] = gs.result_ids[gs.output_idx+i]; lbl[i] = gs.result_labels[gs.output_idx+i]; }
  gs.output_idx += to_out;
  output.SetCardinality(to_out);
  return gs.output_idx >= gs.result_ids.size() ? OperatorFinalizeResultType::FINISHED : OperatorFinalizeResultType::HAVE_MORE_OUTPUT;
}

// =============================================================================
// Radius Metric Table Function
// =============================================================================

struct RadiusGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  int64_t radius = -1;
  bool computed = false, returned = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> RadiusBind(ClientContext &context, TableFunctionBindInput &input,
                                            vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_mtr_radius requires 2 columns");
  return_types.push_back(LogicalType::BIGINT); names.push_back("radius");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> RadiusInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<RadiusGlobalState>();
}

static OperatorResultType RadiusInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<RadiusGlobalState>();
  auto src = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(src[i]); gs.dst_nodes.push_back(dst[i]); }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType RadiusFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<RadiusGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; gs.returned = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.radius = onager_compute_radius(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    gs.computed = true;
  }
  if (gs.returned) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<int64_t>(output.data[0])[0] = gs.radius;
  output.SetCardinality(1);
  gs.returned = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Average Clustering Coefficient Metric Table Function
// =============================================================================

struct AvgClusteringGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  double value = 0.0;
  bool computed = false, returned = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> AvgClusteringBind(ClientContext &context, TableFunctionBindInput &input,
                                                   vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_mtr_avg_clustering requires 2 columns");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("avg_clustering");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> AvgClusteringInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<AvgClusteringGlobalState>();
}

static OperatorResultType AvgClusteringInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgClusteringGlobalState>();
  auto src = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(src[i]); gs.dst_nodes.push_back(dst[i]); }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType AvgClusteringFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgClusteringGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; gs.returned = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.value = onager_compute_avg_clustering(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    gs.computed = true;
  }
  if (gs.returned) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<double>(output.data[0])[0] = gs.value;
  output.SetCardinality(1);
  gs.returned = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Average Path Length Metric Table Function
// =============================================================================

struct AvgPathLengthGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  double value = 0.0;
  bool computed = false, returned = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> AvgPathLengthBind(ClientContext &context, TableFunctionBindInput &input,
                                                   vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_mtr_avg_path_length requires 2 columns");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("avg_path_length");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> AvgPathLengthInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<AvgPathLengthGlobalState>();
}

static OperatorResultType AvgPathLengthInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgPathLengthGlobalState>();
  auto src = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(src[i]); gs.dst_nodes.push_back(dst[i]); }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType AvgPathLengthFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<AvgPathLengthGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; gs.returned = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.value = onager_compute_avg_path_length(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    gs.computed = true;
  }
  if (gs.returned) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<double>(output.data[0])[0] = gs.value;
  output.SetCardinality(1);
  gs.returned = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Transitivity Metric Table Function
// =============================================================================

struct TransitivityGlobalState : public GlobalTableFunctionState {
  std::vector<int64_t> src_nodes, dst_nodes;
  double value = 0.0;
  bool computed = false, returned = false;
  idx_t MaxThreads() const override { return 1; }
};

static unique_ptr<FunctionData> TransitivityBind(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names) {
  if (input.input_table_types.size() < 2) throw InvalidInputException("onager_mtr_transitivity requires 2 columns");
  return_types.push_back(LogicalType::DOUBLE); names.push_back("transitivity");
  return make_uniq<TableFunctionData>();
}

static unique_ptr<GlobalTableFunctionState> TransitivityInitGlobal(ClientContext &context, TableFunctionInitInput &input) {
  return make_uniq<TransitivityGlobalState>();
}

static OperatorResultType TransitivityInOut(ExecutionContext &context, TableFunctionInput &data, DataChunk &input, DataChunk &output) {
  auto &gs = data.global_state->Cast<TransitivityGlobalState>();
  auto src = FlatVector::GetData<int64_t>(input.data[0]);
  auto dst = FlatVector::GetData<int64_t>(input.data[1]);
  for (idx_t i = 0; i < input.size(); i++) { gs.src_nodes.push_back(src[i]); gs.dst_nodes.push_back(dst[i]); }
  output.SetCardinality(0);
  return OperatorResultType::NEED_MORE_INPUT;
}

static OperatorFinalizeResultType TransitivityFinal(ExecutionContext &context, TableFunctionInput &data, DataChunk &output) {
  auto &gs = data.global_state->Cast<TransitivityGlobalState>();
  if (!gs.computed) {
    if (gs.src_nodes.empty()) { gs.computed = true; gs.returned = true; output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
    gs.value = onager_compute_transitivity(gs.src_nodes.data(), gs.dst_nodes.data(), gs.src_nodes.size());
    gs.computed = true;
  }
  if (gs.returned) { output.SetCardinality(0); return OperatorFinalizeResultType::FINISHED; }
  FlatVector::GetData<double>(output.data[0])[0] = gs.value;
  output.SetCardinality(1);
  gs.returned = true;
  return OperatorFinalizeResultType::FINISHED;
}

// =============================================================================
// Extension Registration
// =============================================================================

void OnagerExtension::Load(ExtensionLoader &loader) {
  loader.SetDescription("Onager: Graph Data Analytics Extension");

  // Utility functions
  loader.RegisterFunction(ScalarFunction("onager_version", {}, LogicalType::VARCHAR, GetVersion));
  loader.RegisterFunction(ScalarFunction("onager_last_error", {}, LogicalType::VARCHAR, GetLastError));

  // Centrality functions (ctr prefix)
  TableFunction pagerank_func("onager_ctr_pagerank", {LogicalType::TABLE}, nullptr, PageRankBind, PageRankInitGlobal);
  pagerank_func.in_out_function = PageRankInOut;
  pagerank_func.in_out_function_final = PageRankFinal;
  pagerank_func.named_parameters["damping"] = LogicalType::DOUBLE;
  pagerank_func.named_parameters["iterations"] = LogicalType::BIGINT;
  pagerank_func.named_parameters["directed"] = LogicalType::BOOLEAN;
  loader.RegisterFunction(pagerank_func);

  TableFunction degree_func("onager_ctr_degree", {LogicalType::TABLE}, nullptr, DegreeBind, DegreeInitGlobal);
  degree_func.in_out_function = DegreeInOut;
  degree_func.in_out_function_final = DegreeFinal;
  degree_func.named_parameters["directed"] = LogicalType::BOOLEAN;
  loader.RegisterFunction(degree_func);

  TableFunction betweenness_func("onager_ctr_betweenness", {LogicalType::TABLE}, nullptr, BetweennessBind, BetweennessInitGlobal);
  betweenness_func.in_out_function = BetweennessInOut;
  betweenness_func.in_out_function_final = BetweennessFinal;
  betweenness_func.named_parameters["normalized"] = LogicalType::BOOLEAN;
  loader.RegisterFunction(betweenness_func);

  TableFunction closeness_func("onager_ctr_closeness", {LogicalType::TABLE}, nullptr, ClosenessBind, ClosenessInitGlobal);
  closeness_func.in_out_function = ClosenessInOut;
  closeness_func.in_out_function_final = ClosenessFinal;
  loader.RegisterFunction(closeness_func);

  // Community functions (cmm prefix)
  TableFunction louvain_func("onager_cmm_louvain", {LogicalType::TABLE}, nullptr, LouvainBind, LouvainInitGlobal);
  louvain_func.in_out_function = LouvainInOut;
  louvain_func.in_out_function_final = LouvainFinal;
  louvain_func.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(louvain_func);

  TableFunction components_func("onager_cmm_components", {LogicalType::TABLE}, nullptr, ComponentsBind, ComponentsInitGlobal);
  components_func.in_out_function = ComponentsInOut;
  components_func.in_out_function_final = ComponentsFinal;
  loader.RegisterFunction(components_func);

  // Additional centrality functions
  TableFunction eigenvector_func("onager_ctr_eigenvector", {LogicalType::TABLE}, nullptr, EigenvectorBind, EigenvectorInitGlobal);
  eigenvector_func.in_out_function = EigenvectorInOut;
  eigenvector_func.in_out_function_final = EigenvectorFinal;
  eigenvector_func.named_parameters["max_iter"] = LogicalType::BIGINT;
  eigenvector_func.named_parameters["tolerance"] = LogicalType::DOUBLE;
  loader.RegisterFunction(eigenvector_func);

  TableFunction katz_func("onager_ctr_katz", {LogicalType::TABLE}, nullptr, KatzBind, KatzInitGlobal);
  katz_func.in_out_function = KatzInOut;
  katz_func.in_out_function_final = KatzFinal;
  katz_func.named_parameters["alpha"] = LogicalType::DOUBLE;
  katz_func.named_parameters["max_iter"] = LogicalType::BIGINT;
  katz_func.named_parameters["tolerance"] = LogicalType::DOUBLE;
  loader.RegisterFunction(katz_func);

  TableFunction harmonic_func("onager_ctr_harmonic", {LogicalType::TABLE}, nullptr, HarmonicBind, HarmonicInitGlobal);
  harmonic_func.in_out_function = HarmonicInOut;
  harmonic_func.in_out_function_final = HarmonicFinal;
  loader.RegisterFunction(harmonic_func);

  // Path functions (pth prefix)
  TableFunction dijkstra_func("onager_pth_dijkstra", {LogicalType::TABLE}, nullptr, DijkstraBind, DijkstraInitGlobal);
  dijkstra_func.in_out_function = DijkstraInOut;
  dijkstra_func.in_out_function_final = DijkstraFinal;
  dijkstra_func.named_parameters["source"] = LogicalType::BIGINT;
  loader.RegisterFunction(dijkstra_func);

  // Traversal functions (trv prefix)
  TableFunction bfs_func("onager_trv_bfs", {LogicalType::TABLE}, nullptr, BfsBind, BfsInitGlobal);
  bfs_func.in_out_function = BfsInOut;
  bfs_func.in_out_function_final = BfsFinal;
  bfs_func.named_parameters["source"] = LogicalType::BIGINT;
  loader.RegisterFunction(bfs_func);

  TableFunction dfs_func("onager_trv_dfs", {LogicalType::TABLE}, nullptr, DfsBind, DfsInitGlobal);
  dfs_func.in_out_function = DfsInOut;
  dfs_func.in_out_function_final = DfsFinal;
  dfs_func.named_parameters["source"] = LogicalType::BIGINT;
  loader.RegisterFunction(dfs_func);

  // Link prediction functions (lnk prefix)
  TableFunction jaccard_func("onager_lnk_jaccard", {LogicalType::TABLE}, nullptr, JaccardBind, JaccardInitGlobal);
  jaccard_func.in_out_function = JaccardInOut;
  jaccard_func.in_out_function_final = JaccardFinal;
  loader.RegisterFunction(jaccard_func);

  // Metric functions (mtr prefix)
  TableFunction diameter_func("onager_mtr_diameter", {LogicalType::TABLE}, nullptr, DiameterBind, DiameterInitGlobal);
  diameter_func.in_out_function = DiameterInOut;
  diameter_func.in_out_function_final = DiameterFinal;
  loader.RegisterFunction(diameter_func);

  // Generator functions (gen prefix) - these are regular table functions, not in_out
  TableFunction erdos_renyi_func("onager_gen_erdos_renyi", {LogicalType::BIGINT, LogicalType::DOUBLE}, ErdosRenyiFunction, ErdosRenyiBind, ErdosRenyiInitGlobal);
  erdos_renyi_func.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(erdos_renyi_func);

  TableFunction barabasi_albert_func("onager_gen_barabasi_albert", {LogicalType::BIGINT, LogicalType::BIGINT}, BarabasiAlbertFunction, BarabasiAlbertBind, BarabasiAlbertInitGlobal);
  barabasi_albert_func.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(barabasi_albert_func);

  TableFunction watts_strogatz_func("onager_gen_watts_strogatz", {LogicalType::BIGINT, LogicalType::BIGINT, LogicalType::DOUBLE}, WattsStrogatzFunction, WattsStrogatzBind, WattsStrogatzInitGlobal);
  watts_strogatz_func.named_parameters["seed"] = LogicalType::BIGINT;
  loader.RegisterFunction(watts_strogatz_func);

  // More Link Prediction functions
  TableFunction adamic_adar_func("onager_lnk_adamic_adar", {LogicalType::TABLE}, nullptr, AdamicAdarBind, AdamicAdarInitGlobal);
  adamic_adar_func.in_out_function = AdamicAdarInOut;
  adamic_adar_func.in_out_function_final = AdamicAdarFinal;
  loader.RegisterFunction(adamic_adar_func);

  TableFunction pref_attach_func("onager_lnk_pref_attach", {LogicalType::TABLE}, nullptr, PrefAttachBind, PrefAttachInitGlobal);
  pref_attach_func.in_out_function = PrefAttachInOut;
  pref_attach_func.in_out_function_final = PrefAttachFinal;
  loader.RegisterFunction(pref_attach_func);

  TableFunction resource_alloc_func("onager_lnk_resource_alloc", {LogicalType::TABLE}, nullptr, ResourceAllocBind, ResourceAllocInitGlobal);
  resource_alloc_func.in_out_function = ResourceAllocInOut;
  resource_alloc_func.in_out_function_final = ResourceAllocFinal;
  loader.RegisterFunction(resource_alloc_func);

  // More Community Detection functions
  TableFunction label_prop_func("onager_cmm_label_prop", {LogicalType::TABLE}, nullptr, LabelPropBind, LabelPropInitGlobal);
  label_prop_func.in_out_function = LabelPropInOut;
  label_prop_func.in_out_function_final = LabelPropFinal;
  loader.RegisterFunction(label_prop_func);

  // More Metric functions
  TableFunction radius_func("onager_mtr_radius", {LogicalType::TABLE}, nullptr, RadiusBind, RadiusInitGlobal);
  radius_func.in_out_function = RadiusInOut;
  radius_func.in_out_function_final = RadiusFinal;
  loader.RegisterFunction(radius_func);

  TableFunction avg_clustering_func("onager_mtr_avg_clustering", {LogicalType::TABLE}, nullptr, AvgClusteringBind, AvgClusteringInitGlobal);
  avg_clustering_func.in_out_function = AvgClusteringInOut;
  avg_clustering_func.in_out_function_final = AvgClusteringFinal;
  loader.RegisterFunction(avg_clustering_func);

  TableFunction avg_path_length_func("onager_mtr_avg_path_length", {LogicalType::TABLE}, nullptr, AvgPathLengthBind, AvgPathLengthInitGlobal);
  avg_path_length_func.in_out_function = AvgPathLengthInOut;
  avg_path_length_func.in_out_function_final = AvgPathLengthFinal;
  loader.RegisterFunction(avg_path_length_func);

  TableFunction transitivity_func("onager_mtr_transitivity", {LogicalType::TABLE}, nullptr, TransitivityBind, TransitivityInitGlobal);
  transitivity_func.in_out_function = TransitivityInOut;
  transitivity_func.in_out_function_final = TransitivityFinal;
  loader.RegisterFunction(transitivity_func);
}



std::string OnagerExtension::Name() {
  return "onager";
}

std::string OnagerExtension::Version() const {
#ifdef EXT_VERSION_ONAGER
  return EXT_VERSION_ONAGER;
#else
  return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void onager_init(duckdb::DatabaseInstance &db) {
  duckdb::DuckDB db_wrapper(db);
  db_wrapper.LoadStaticExtension<duckdb::OnagerExtension>();
}

DUCKDB_EXTENSION_API const char *onager_version() {
  return duckdb::DuckDB::LibraryVersion();
}

}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
