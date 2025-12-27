/**
 * @file onager_extension.cpp
 * @brief Main entry point for the Onager DuckDB extension.
 *
 * Onager is a graph data analytics extension for DuckDB, powered by the
 * graphina library. It provides SQL functions for graph analysis including
 * centrality measures, community detection, and path finding.
 *
 * This file provides the extension registration; all functions are implemented
 * in modular files under functions/.
 */
#define DUCKDB_EXTENSION_MAIN

#include "include/onager_extension.hpp"
#include "functions.hpp"
#include "duckdb.hpp"

namespace duckdb {

void OnagerExtension::Load(ExtensionLoader &loader) {
  loader.SetDescription("Onager: Graph Data Analytics Extension");

  // Register all functions from modular files
  onager::RegisterScalarFunctions(loader);
  onager::RegisterAllCentralityFunctions(loader);
  onager::RegisterCommunityFunctions(loader);
  onager::RegisterTraversalFunctions(loader);
  onager::RegisterGeneratorFunctions(loader);
  onager::RegisterLinkFunctions(loader);
  onager::RegisterMetricFunctions(loader);
  onager::RegisterMstFunctions(loader);
  onager::RegisterApproximationFunctions(loader);
  onager::RegisterPersonalizedFunctions(loader);
  onager::RegisterSubgraphFunctions(loader);
  onager::RegisterParallelFunctions(loader);
  onager::RegisterLocalReachingFunction(loader);
  onager::RegisterLaplacianFunction(loader);
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

DUCKDB_CPP_EXTENSION_ENTRY(onager, loader) {
  duckdb::OnagerExtension extension;
  extension.Load(loader);
}

DUCKDB_EXTENSION_API const char *onager_duckdb_cpp_version() {
  return duckdb::DuckDB::LibraryVersion();
}

}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
