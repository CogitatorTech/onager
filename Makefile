# ==============================================================================
# Variables
# ==============================================================================
PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
EXT_NAME := onager
RUST_LIB := onager/target/release/$(EXT_NAME).a
DUCKDB_SRCDIR := ./external/duckdb/
EXT_CONFIG := ${PROJ_DIR}extension_config.cmake
EXAMPLES_DIR := docs/examples
SHELL := /bin/bash
PYTHON := python3

# ==============================================================================
# DuckDB Extension Build Configuration
# ==============================================================================
include external/extension-ci-tools/makefiles/duckdb_extension.Makefile
set_duckdb_version:
	cd external/duckdb && git checkout $(DUCKDB_GIT_VERSION)

# ==============================================================================
# Help Target
# ==============================================================================
.DEFAULT_GOAL := help

.PHONY: help
help: ## Show the help messages for all targets
	@echo "Usage: make <target>"
	@echo ""
	@echo "Targets:"
	@grep -E '^[a-zA-Z_-]+:.*## .*$$' Makefile | \
	awk 'BEGIN {FS = ":.*## "}; {printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2}'

# ==============================================================================
# Rust Targets
# ==============================================================================
.PHONY: rust-build
rust-build: ## Build Onager in release mode
	@echo "Building Onager in release mode..."
	@cd onager && cargo build --release --features "duckdb_extension"

.PHONY: rust-build-debug
rust-build-debug: ## Build Onager in debug mode
	@echo "Building Onager in debug mode..."
	@cd onager && cargo build --features "duckdb_extension"

.PHONY: rust-format
rust-format: ## Format Rust files
	@echo "Formatting Rust files..."
	@cargo fmt --manifest-path onager/Cargo.toml

.PHONY: rust-test
rust-test: rust-format ## Run tests
	@echo "Running the unit tests for Onager..."
	@cargo test --manifest-path onager/Cargo.toml --all-targets -- --nocapture

.PHONY: rust-coverage
rust-coverage: ## Generate code coverage report for Onager crate
	@echo "Generating coverage report..."
	@cargo tarpaulin --manifest-path onager/Cargo.toml --all-targets --out Xml

.PHONY: rust-lint
rust-lint: rust-format ## Run linter checks on Rust files
	@echo "Linting Rust files..."
	@cargo clippy --manifest-path onager/Cargo.toml -- -D warnings -D clippy::unwrap_used -D clippy::expect_used

.PHONY: rust-fix-lint
rust-fix-lint: ## Fix Rust linter warnings
	@echo "Fixing linter warnings..."
	@cargo clippy --fix --allow-dirty --allow-staged --manifest-path onager/Cargo.toml -- -D warnings -D clippy::unwrap_used -D clippy::expect_used

.PHONY: rust-careful
careful: ## Run security checks on Rust code
	@echo "Running security checks..."
	@cargo careful

.PHONY: rust-clean
rust-clean: ## Clean Rust build artifacts
	@echo "Cleaning Rust build artifacts..."
	@cd onager && cargo clean

.PHONY: create-bindings
create-bindings: ## Generate C bindings from Rust code
	@echo "Generating C bindings for Onager..."
	@cd onager && cbindgen --config cbindgen.toml --crate onager --output bindings/include/rust.h
	@echo "C bindings generated at onager/bindings/include/rust.h"

# ==============================================================================
# Targets for Building the Extension
# ==============================================================================
# We need to build the DuckDB first, so we can link against it
# when building the Onager as a DuckDB extension.
# We normally need to build the DuckDB once only.
# The `build/{release,debug}` directories will contain the DuckDB build that is staticly linked
# with the Onager extension. That is used for testing the extension.

.PHONY: release
release: rust-build ## Build the extension in release mode (DuckDB + extension)
	@echo "Building the extension in release mode..."
	@mkdir -p build/release
	@cmake $(GENERATOR) $(BUILD_FLAGS) $(EXT_RELEASE_FLAGS) $(VCPKG_MANIFEST_FLAGS) -DBUILD_SHELL=TRUE -DBUILD_MAIN_DUCKDB_LIBRARY=TRUE -DCMAKE_BUILD_TYPE=Release -S $(DUCKDB_SRCDIR) -B build/release
	@cmake --build build/release --config Release

.PHONY: debug
debug: rust-build-debug ## Build the extension in debug mode (DuckDB + extension)
	@echo "Building the extension in debug mode..."
	@mkdir -p build/debug
	@cmake $(GENERATOR) $(BUILD_FLAGS) $(EXT_DEBUG_FLAGS) $(VCPKG_MANIFEST_FLAGS) -DBUILD_SHELL=TRUE -DBUILD_MAIN_DUCKDB_LIBRARY=TRUE -DCMAKE_BUILD_TYPE=Debug -S $(DUCKDB_SRCDIR) -B build/debug
	@cmake --build build/debug --config Debug

# ==============================================================================
# Development Targets
# ==============================================================================
.PHONY: install-deps
install-deps: ## Set up development environment (for Debian-based systems)
	@echo "Setting up development environment..."
	@sudo apt-get install -y cmake clang-format snap python3-pip liblzma-dev
	@sudo snap install rustup --classic
	@cargo install cargo-tarpaulin cbindgen cargo-edit cargo-audit cargo-outdated cargo-careful
	@cd onager && cargo check
	@git submodule update --init --recursive
	@pip install --user --upgrade pip uv
	@uv sync --extra dev
	@pre-commit install-hooks
	@echo "Development environment ready"

.PHONY: setup-hooks
setup-hooks: ## Install Git hooks (pre-commit and pre-push)
	@echo "Installing Git hooks..."
	@pre-commit install --hook-type pre-commit
	@pre-commit install --hook-type pre-push
	@pre-commit install-hooks

.PHONY: test-hooks
test-hooks: ## Run Git hooks on all files manually
	@echo "Running Git hooks..."
	@pre-commit run --all-files

.PHONY: clean-all
clean-all: clean rust-clean ## Clean everything
	@echo "All clean!"

.PHONY: check
check: rust-lint rust-test ## Run all checks (linting and tests)
	@echo "All checks passed!"

.PHONY: examples
examples: ## Run SQL examples for Onager extension
	@echo "Running the examples in the ${EXAMPLES_DIR} directory..."
	@for sql_file in $(EXAMPLES_DIR)/*.sql; do \
		echo "Running example: $$sql_file"; \
		./build/release/duckdb < $$sql_file; \
		echo "============================================================================"; \
	done
