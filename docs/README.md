### API Reference

The table below includes the information about all SQL functions exposed by Onager.

| #  | Function                                                        | Return Type                                      | Description                                                                                                                                                                                                                               |
|----|:----------------------------------------------------------------|:-------------------------------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1  | `onager_set_credentials(username VARCHAR, key VARCHAR)`         | `BOOLEAN`                                        | Sets Kaggle API credentials from SQL (alternatively use env vars or `~/.kaggle/kaggle.json`). Returns `true` on success.                                                                                                                  |
| 2  | `onager_download(dataset_path VARCHAR)`                         | `VARCHAR`                                        | Downloads a Kaggle dataset to the local cache directory and returns the local dataset path. This function is idempotent.                                                                                                                  |
| 3  | `onager_search(query VARCHAR, page INTEGER, page_size INTEGER)` | `VARCHAR (JSON)`                                 | Searches Kaggle datasets and returns a JSON array. Constraints: `page >= 1`, `1 <= page_size <= 100`.                                                                                                                                     |
| 4  | `onager_info(dataset_path VARCHAR)`                             | `VARCHAR (JSON)`                                 | Returns metadata for a dataset as JSON (for example: `title`, `url`, `last_updated`).                                                                                                                                                     |
| 5  | `onager_version()`                                              | `VARCHAR`                                        | Returns the extension version string (for example: `"0.1.0"`).                                                                                                                                                                            |
| 6  | `onager_clear_cache()`                                          | `BOOLEAN`                                        | Clears the dataset cache directory. Returns `true` on success.                                                                                                                                                                            |
| 7  | `onager_cache_info()`                                           | `VARCHAR (JSON)`                                 | Returns cache info JSON with `path`, `size_mb`, `limit_mb`, `usage_percent`, `is_soft_limit`, and `type` fields.                                                                                                                          |
| 8  | `onager_enforce_cache_limit()`                                  | `BOOLEAN`                                        | Manually enforces cache size limit using LRU eviction. Returns `true` on success. (Automatic with soft limit by default).                                                                                                                 |
| 9  | `onager_is_current(dataset_path VARCHAR)`                       | `BOOLEAN`                                        | Checks if cached dataset is the latest version from Kaggle. Returns `false` if not cached or outdated.                                                                                                                                    |
| 10 | `onager_update_dataset(dataset_path VARCHAR)`                   | `VARCHAR`                                        | Forces update to latest version (ignores cache). Returns local path to freshly downloaded dataset.                                                                                                                                        |
| 11 | `onager_version_info(dataset_path VARCHAR)`                     | `VARCHAR (JSON)`                                 | Returns version info: `cached_version`, `latest_version`, `is_current`, `is_cached`.                                                                                                                                                      |
| 12 | `onager_json_each(json VARCHAR)`                                | `VARCHAR`                                        | Expands a JSON object into newline-delimited JSON rows with fields: `key`, `value`, `type`, `path`. Users normally shouldn't use this function.                                                                                           |
| 13 | `onager_file_path(dataset_path VARCHAR, filename VARCHAR)`      | `VARCHAR`                                        | Resolves a specific file's local path inside a downloaded dataset.                                                                                                                                                                        |
| 14 | `onager_ls(dataset_path VARCHAR[, recursive BOOLEAN])`          | `TABLE(name VARCHAR, size BIGINT, path VARCHAR)` | Lists files in the dataset's local directory; non-recursive by default. When `recursive=true` will walk subdirectories. `path` values are returned as `owner/dataset/<relative-path>` (not an absolute filesystem path); `size` is in MB. |

> [!NOTE]
> * The `onager_file_path` function will retrieve and cache the file if it is not already downloaded; set
    `ONAGER_STRICT_ONDEMAND=1` to prevent fallback to a full dataset download on failures.
>
> * Dataset paths must be in the form `owner/dataset` where `owner` is the username and `dataset` is the dataset name on
    Kaggle. For example: `habedi/flickr-8k-dataset-clean`. You can also read files directly using the replacement scan
    with the `kaggle:` scheme. For example: `'kaggle:habedi/flickr-8k-dataset-clean/flickr8k.parquet`.

---

### Usage Examples

To be able to use most of the examples below, you need to have a valid Kaggle username and API key.
Check out the [Kaggle API documentation](https://www.kaggle.com/docs/api) for more information on how to
get your username and API key.

#### Dataset Management

```sql
-- Load the Onager extension
load
'build/release/extension/onager/onager.duckdb_extension';

-- Set Kaggle credentials (or read from environment variables or from `~/.kaggle/kaggle.json` file)
select onager_set_credentials('your-username', 'your-api-key');

-- Check version
select onager_version();

-- Search datasets (returns a JSON array)
-- (This function is disabled in offline mode (when ONAGER_OFFLINE=1))
select onager_search('iris', 1, 5);

-- Download a dataset and get its local path
select onager_download('uciml/iris') as local_path;

-- Get dataset metadata (as a JSON object)
-- (This function is disabled in offline mode (when ONAGER_OFFLINE=1))
select onager_info('uciml/iris') as dataset_metadata;
```

#### Reading Data

```sql
-- List files as a table (non-recursive)
select *
from onager_ls('uciml/iris') limit 5;

-- List files as a table (recursive)
select *
from onager_ls('suganthidevasagayam/social-media-post-of-postpartum-depression', true) limit 10;

-- List files as a JSON array
select to_json(list(struct_pack(name := name, size := size, path := path))) as files_json
from onager_ls('uciml/iris');

-- Note: returned `path` values are in the form 'owner/dataset/...',
-- which work for use with replacement scans or as an identifier inside the cache;
-- to get an absolute filesystem path use `onager_file_path(owner_dataset, relative_path)`.

-- Resolve a file path and read it via a prepared statement
prepare rp as select * from read_parquet(?) limit 10;
execute rp(onager_file_path('owner/dataset', 'file.parquet'));
```

```sql
-- Replacement scan: read a single Parquet file via `kaggle:` URL scheme
select count(*)
from 'kaggle:owner/dataset/file.parquet';

-- Replacement scan: glob Parquet files in a dataset directory
select count(*)
from 'kaggle:owner/dataset/*.parquet';
```

#### Dataset Versioning

```sql
-- Check if cached dataset is the latest version
select onager_is_current('owner/dataset') as is_current;

-- Get detailed version information
select onager_version_info('owner/dataset') as version_info;
-- Returns: {"cached_version": "3", "latest_version": "5", "is_current": false, "is_cached": true}

-- Force update to latest version (ignores cache)
select onager_update_dataset('owner/dataset') as updated_path;

-- Download specific version (version pinning)
select onager_download('owner/dataset@v2'); -- Version 2
select onager_download('owner/dataset@5'); -- Version 5 (without 'v' prefix)
select onager_download('owner/dataset@latest');
-- Explicit latest

-- Use versioned datasets in queries
select *
from 'kaggle:owner/dataset@v2/file.csv';
select *
from 'kaggle:owner/dataset@v5/*.parquet';

-- Smart download: update only if outdated
select case
           when onager_is_current('owner/dataset') then onager_download('owner/dataset')
           else onager_update_dataset('owner/dataset')
           end as path;
```

#### Utility Functions

```sql
-- Purge cache and see cache info
select onager_clear_cache();
select onager_cache_info();

-- Manually enforce cache size limit
-- (Automatic enforcement is done with a soft limit by default and older files are removed first)
select onager_enforce_cache_limit();

-- Expand JSON into newline-delimited rows
select onager_json_each('{"a":1,"b":[true,{"c":"x"}]}') as rows;

-- Parse search results as JSON and extract a couple of fields
with s as (select from_json(onager_search('iris', 1, 10)) as j)
select json_extract_string(value, '$.ref')   as ref,
       json_extract_string(value, '$.title') as title
from json_each((select j from s)) limit 5;
```

---

### Building Onager from Source

To build Onager from source, you need GNU Make, CMake, a modern C++ compiler (like GCC or Clang), Rust and Cargo.

1. **Clone the repository:**
   ```bash
   git clone --recursive https://github.com/CogitatorTech/onager.git
   cd onager
   ```

2. **Build the extension:**
   ```bash
   make release
   ```
   This will create a `duckdb` executable inside `build/release/` and a loadable extension at
   `build/release/extension/onager/onager.duckdb_extension`.

3. **Run the custom DuckDB shell:**
   ```bash
   ./build/release/duckdb
   ```
   You can load the extension with:
   ```sql
   load 'build/release/extension/onager/onager.duckdb_extension';
   ```

---

### Architecture Overview

Onager is made up of two main components:

1. **Rust Core (`onager/src/`)** that handles:
    - Credentials management
    - HTTP client with timeout and exponential backoff
    - Dataset download with safe ZIP extraction and file resolution
    - Search and metadata requests
    - A few C-compatible FFI functions for use by DuckDB

2. **C++ DuckDB Bindings (`onager/bindings/`)** that:
    - Defines the custom SQL functions (for example: `onager_ls`, `onager_file_path`, and `onager_search`)
    - Integrates with DuckDB’s extension system and replacement scans (`'kaggle:...'`)
    - Marshals values between DuckDB vectors and the Rust FFI

### Additional Resources

- [ERROR_CODES.md](ERROR_CODES.md): information about the error codes returned by Onager.
- [CONFIGURATION.md](CONFIGURATION.md): details about environment variables that can be used to configure Onager's
  behavior.
- [examples/](examples): example SQL scripts that showcase various Onager features.
