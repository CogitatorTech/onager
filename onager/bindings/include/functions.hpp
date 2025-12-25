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

namespace duckdb {
namespace onager {

/**
 * @brief Retrieves the last error message from the Onager Rust core.
 */
inline std::string GetOnagerError() {
  const char *err = ::onager::onager_last_error();
  return err ? std::string(err) : std::string("unknown error");
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
