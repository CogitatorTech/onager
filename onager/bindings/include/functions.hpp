/**
 * @file functions.hpp
 * @brief Shared utilities for Onager DuckDB extension table functions.
 */
#pragma once

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
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

// =============================================================================
// DuckDB Version Compatibility
// =============================================================================
namespace duckdb {
namespace onager {

template <int N> struct PriorityTag : PriorityTag<N - 1> {};
template <> struct PriorityTag<0> {};

template <typename T>
struct BindNamesTypeHelper;

template <typename Ret, typename A1, typename A2, typename A3, typename A4>
struct BindNamesTypeHelper<Ret (*)(A1, A2, A3, A4)> {
  using type = std::remove_reference_t<A4>;
};

using bind_names_t = typename BindNamesTypeHelper<duckdb::table_function_bind_t>::type;

template <typename T>
static auto AddNamedParamCompat(T &func, const std::string &name, const LogicalType &type, PriorityTag<2>)
    -> decltype(func.named_parameters[name] = type, void()) {
  func.named_parameters[name] = type;
}

template <typename T>
static auto AddNamedParamCompat(T &func, const std::string &name, const LogicalType &type, PriorityTag<1>)
    -> decltype(func.GetSignature().GetTypedKwargs(), void()) {
  if (!func.GetSignature().GetTypedKwargs()) {
    func.GetSignature().WithTypedKwargs("options", [&](auto &opts) {
      opts.Add(name.c_str(), type);
    });
  } else {
    func.GetSignature().ExtendTypedKwargs([&](auto &opts) {
      opts.Add(name.c_str(), type);
    });
  }
}

template <typename T>
static void AddNamedParamCompat(T &, const std::string &, const LogicalType &, PriorityTag<0>) {}

template <typename T>
inline void AddNamedParam(T &func, const std::string &name, const LogicalType &type) {
  AddNamedParamCompat(func, name, type, PriorityTag<2>{});
}

template <typename T>
static auto SetCardinalityCompat(T &chunk, idx_t count, PriorityTag<1>)
    -> decltype(chunk.SetChildCardinality(count), void()) {
  chunk.SetChildCardinality(count);
}

template <typename T>
static void SetCardinalityCompat(T &chunk, idx_t count, PriorityTag<0>) {
  chunk.SetCardinality(count);
}

template <typename T>
inline void SetCardinalityCompat(T &chunk, idx_t count) {
  SetCardinalityCompat(chunk, count, PriorityTag<1>{});
}

template <typename T>
static auto ToUnifiedFormatCompat(T &vector, idx_t count, UnifiedVectorFormat &data, PriorityTag<1>)
    -> decltype(vector.ToUnifiedFormat(data), void()) {
  vector.ToUnifiedFormat(data);
}

template <typename T>
static void ToUnifiedFormatCompat(T &vector, idx_t count, UnifiedVectorFormat &data, PriorityTag<0>) {
  vector.ToUnifiedFormat(count, data);
}

inline void ToUnifiedFormatCompat(Vector &vector, idx_t count, UnifiedVectorFormat &data) {
  ToUnifiedFormatCompat(vector, count, data, PriorityTag<1>{});
}

template <typename T>
static auto SetNoOrderCompat(T &func, PriorityTag<1>)
    -> decltype(func.order_preservation_type = OrderPreservationType::NO_ORDER, void()) {
  func.order_preservation_type = OrderPreservationType::NO_ORDER;
}

template <typename T>
static void SetNoOrderCompat(T &, PriorityTag<0>) {}

template <typename T>
inline void SetNoOrder(T &func) {
  SetNoOrderCompat(func, PriorityTag<1>{});
}

#define ONAGER_SET_NO_ORDER(func) ::duckdb::onager::SetNoOrder(func)
#define ONAGER_TO_UNIFIED_FORMAT(vector, count, data) ::duckdb::onager::ToUnifiedFormatCompat(vector, count, data)
#define ONAGER_SET_CARDINALITY(chunk, count) ::duckdb::onager::SetCardinalityCompat(chunk, count)

} // namespace onager
} // namespace duckdb


namespace duckdb {
namespace onager {

/**
 * @brief Returns a writable pointer to a flat vector's data across DuckDB versions.
 *
 * In DuckDB v1.5.2, `FlatVector::GetData<T>(Vector &)` returns `T *`. In newer
 * DuckDB versions it returns `const T *` and `GetDataMutable<T>` must be used
 * for writable access. Using `FlatVector::GetDataMutable` directly would break
 * compilation on v1.5.2, so this helper casts away const on the result of
 * `GetData`, which is safe for flat vectors the extension owns.
 */
template <typename T>
inline T *GetFlatVectorDataWritable(Vector &vector) {
  return const_cast<T *>(FlatVector::GetData<T>(vector));
}

/**
 * @brief Returns a writable pointer to a constant vector's data across DuckDB versions.
 */
template <typename T>
inline T *GetConstantVectorDataWritable(Vector &vector) {
  return const_cast<T *>(ConstantVector::GetData<T>(vector));
}

/**
 * @brief Returns a writable reference to a flat vector's validity mask across DuckDB versions.
 *
 * On v1.5.2, `FlatVector::Validity(Vector &)` already returns `ValidityMask &`.
 * On newer DuckDB it returns `const ValidityMask &` and `ValidityMutable` must be
 * used. Casting away const here keeps a single call site that compiles on both.
 */
inline ValidityMask &GetFlatVectorValidityWritable(Vector &vector) {
  return const_cast<ValidityMask &>(FlatVector::Validity(vector));
}

/**
 * @brief Retrieves the last error message from the Onager Rust core.
 */
inline std::string GetOnagerError() {
  const char *err = ::onager::onager_last_error();
  return err ? std::string(err) : std::string("unknown error");
}

/**
 * @brief Returns true when the Rust core has a pending error on this thread.
 *
 * Every Onager FFI call clears the thread-local error on entry, so checking
 * immediately after a call on the same thread distinguishes an error sentinel
 * (NaN or a negative count) from the same value occurring legitimately.
 */
inline bool OnagerHasPendingError() {
  return ::onager::onager_last_error() != nullptr;
}

/**
 * @brief Returns a named or positional parameter value, rejecting SQL NULL.
 */
template <typename T>
inline T GetRequiredParam(const std::string &function_name, const std::string &param_name, const Value &value) {
  if (value.IsNull()) {
    throw BinderException(function_name + " parameter " + param_name + " cannot be NULL");
  }
  return value.GetValue<T>();
}

/**
 * @brief Returns a BIGINT parameter value, rejecting SQL NULL and negative values.
 */
inline int64_t GetNonNegativeParam(const std::string &function_name, const std::string &param_name, const Value &value) {
  auto v = GetRequiredParam<int64_t>(function_name, param_name, value);
  if (v < 0) {
    throw BinderException(function_name + " parameter " + param_name + " cannot be negative");
  }
  return v;
}

/**
 * @brief Validates the logical type of an auxiliary input column at bind time.
 *
 * Columns past (src, dst) are read through raw typed buffers during
 * execution, so a mismatched column type would reinterpret or overrun the
 * underlying vector. This check makes the mismatch a bind-time error.
 */
inline void CheckColumnType(TableFunctionBindInput &input, const std::string &name, idx_t col, const LogicalType &expected) {
  if (input.input_table_types.size() > col && input.input_table_types[col] != expected) {
    throw InvalidInputException(name + " requires column " + std::to_string(col + 1) + " to be " + expected.ToString() + ". Please cast the column (e.g. column::" + expected.ToString() + "). Found: " + input.input_table_types[col].ToString());
  }
}

/**
 * @brief Validates that input table has BIGINT columns for (src, dst).
 * @param input The table function bind input
 * @param name The function name for error messages
 * @param min_cols Minimum required columns (default 2)
 * @throws InvalidInputException if validation fails
 */
inline void CheckInt64Input(TableFunctionBindInput &input, const std::string &name, size_t min_cols = 2) {
  if (input.input_table_types.size() < min_cols) {
    throw InvalidInputException(name + " requires a table with at least " + std::to_string(min_cols) + " columns");
  }
  if (input.input_table_types[0] != LogicalType::BIGINT || input.input_table_types[1] != LogicalType::BIGINT) {
    throw InvalidInputException(name + " requires (src, dst) columns to be BIGINT. Please cast inputs to BIGINT (e.g. column::bigint). Found: " + input.input_table_types[0].ToString() + ", " + input.input_table_types[1].ToString());
  }
}

/**
 * @brief Appends edge endpoints safely from an unweighted edge list chunk.
 */
inline void AppendInt64Edges(DataChunk &input, std::vector<int64_t> &src_nodes, std::vector<int64_t> &dst_nodes, const char *function_name) {
  UnifiedVectorFormat src_data, dst_data;
  ONAGER_TO_UNIFIED_FORMAT(input.data[0], input.size(), src_data);
  ONAGER_TO_UNIFIED_FORMAT(input.data[1], input.size(), dst_data);

  auto src = UnifiedVectorFormat::GetData<int64_t>(src_data);
  auto dst = UnifiedVectorFormat::GetData<int64_t>(dst_data);
  for (idx_t i = 0; i < input.size(); i++) {
    auto src_idx = src_data.sel->get_index(i);
    auto dst_idx = dst_data.sel->get_index(i);
    if (!src_data.validity.RowIsValid(src_idx) || !dst_data.validity.RowIsValid(dst_idx)) {
      throw InvalidInputException(std::string(function_name) + " does not accept NULL edge endpoints");
    }
    src_nodes.push_back(src[src_idx]);
    dst_nodes.push_back(dst[dst_idx]);
  }
}

/**
 * @brief Appends weighted edge endpoints and weights safely from a weighted edge list chunk.
 */
inline void AppendWeightedEdges(DataChunk &input, std::vector<int64_t> &src_nodes, std::vector<int64_t> &dst_nodes, std::vector<double> &weights, const char *function_name) {
  UnifiedVectorFormat src_data, dst_data, w_data;
  ONAGER_TO_UNIFIED_FORMAT(input.data[0], input.size(), src_data);
  ONAGER_TO_UNIFIED_FORMAT(input.data[1], input.size(), dst_data);
  ONAGER_TO_UNIFIED_FORMAT(input.data[2], input.size(), w_data);

  auto src = UnifiedVectorFormat::GetData<int64_t>(src_data);
  auto dst = UnifiedVectorFormat::GetData<int64_t>(dst_data);
  auto w = UnifiedVectorFormat::GetData<double>(w_data);
  for (idx_t i = 0; i < input.size(); i++) {
    auto src_idx = src_data.sel->get_index(i);
    auto dst_idx = dst_data.sel->get_index(i);
    auto w_idx = w_data.sel->get_index(i);
    if (!src_data.validity.RowIsValid(src_idx) || !dst_data.validity.RowIsValid(dst_idx)) {
      throw InvalidInputException(std::string(function_name) + " does not accept NULL edge endpoints");
    }
    if (!w_data.validity.RowIsValid(w_idx)) {
      throw InvalidInputException(std::string(function_name) + " does not accept NULL edge weights");
    }
    src_nodes.push_back(src[src_idx]);
    dst_nodes.push_back(dst[dst_idx]);
    weights.push_back(w[w_idx]);
  }
}


// Forward declarations for modular function registration
void RegisterScalarFunctions(ExtensionLoader &loader);
void RegisterCentralityFunctions(ExtensionLoader &loader);
void RegisterAllCentralityFunctions(ExtensionLoader &loader);
void RegisterCommunityFunctions(ExtensionLoader &loader);
void RegisterTraversalFunctions(ExtensionLoader &loader);
void RegisterGeneratorFunctions(ExtensionLoader &loader);
void RegisterLinkFunctions(ExtensionLoader &loader);
void RegisterMetricFunctions(ExtensionLoader &loader);
void RegisterMstFunctions(ExtensionLoader &loader);
void RegisterApproximationFunctions(ExtensionLoader &loader);
void RegisterPersonalizedFunctions(ExtensionLoader &loader);
void RegisterSubgraphFunctions(ExtensionLoader &loader);
void RegisterParallelFunctions(ExtensionLoader &loader);
void RegisterLocalReachingFunction(ExtensionLoader &loader);
void RegisterLaplacianFunction(ExtensionLoader &loader);

} // namespace onager
} // namespace duckdb
