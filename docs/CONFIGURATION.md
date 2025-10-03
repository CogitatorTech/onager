### Onager's Configuration Guide

Onager supports configuration via environment variables to customize its behavior without code changes.

#### Environment Variables

##### Cache Configuration

###### ONAGER_CACHE_DIR

- **Description**: Directory path for caching downloaded Kaggle datasets
- **Type**: String (path)
- **Default**: `$XDG_CACHE_HOME/onager` (normally `~/.cache/onager`)
- **Example**:
  ```bash
  export ONAGER_CACHE_DIR="/var/cache/onager"
  ```

###### ONAGER_CACHE_SIZE_LIMIT

- **Description**: Maximum cache size in megabytes for downloaded datasets
- **Type**: Integer (megabytes) or "unlimited"
- **Default**: `102400` (100GB)
- **Behavior**: Uses soft limit by default - downloads complete even if they exceed the limit, then oldest datasets are
  automatically evicted using LRU (Least Recently Used) policy
- **Example**:
  ```bash
  ## Set to 50GB
  export ONAGER_CACHE_SIZE_LIMIT=51200

  ## Set to 5GB
  export ONAGER_CACHE_SIZE_LIMIT=5120

  ## Set unlimited cache
  export ONAGER_CACHE_SIZE_LIMIT=unlimited
  ```

###### ONAGER_CACHE_HARD_LIMIT

- **Description**: Enable hard limit mode (prevents downloads when cache limit would be exceeded)
- **Type**: Boolean (true/yes/1 or false/no/0)
- **Default**: `false` (soft limit)
- **Example**:
  ```bash
  ## Enable hard limit (prevents downloads when cache is full)
  export ONAGER_CACHE_HARD_LIMIT=true
  ```

##### HTTP Configuration

###### ONAGER_HTTP_TIMEOUT

- **Description**: HTTP request timeout in seconds for Kaggle API requests
- **Type**: Integer (seconds)
- **Default**: `30`
- **Example**:
  ```bash
  export ONAGER_HTTP_TIMEOUT=120
  ```

###### ONAGER_API_BASE

- **Description**: Override the Kaggle API base URL (primarily for testing/mocking)
- **Type**: String (URL)
- **Default**: `https://www.kaggle.com/api/v1`
- **Example**:
  ```bash
  ## Point requests to a local mock server
  export ONAGER_API_BASE=http://127.0.0.1:12345
  ```

###### HTTP Retry Controls

- **ONAGER_HTTP_RETRY_ATTEMPTS**
    - **Description**: Number of retry attempts after the initial try
    - **Type**: Integer
    - **Default**: `3`
- **ONAGER_HTTP_RETRY_DELAY**
    - **Description**: Initial backoff delay in seconds
    - **Type**: Float or integer (seconds)
    - **Default**: `1`
- **ONAGER_HTTP_RETRY_MAX_DELAY**
    - **Description**: Maximum backoff delay cap in seconds
    - **Type**: Float or integer (seconds)
    - **Default**: `30`

These controls enable exponential backoff with cap across metadata/search/download requests.

###### ONAGER_API_MIN_INTERVAL_MS

- **Description**: Optional client-side rate limiting. Enforces a minimum interval between HTTP calls.
- **Type**: Integer (milliseconds)
- **Default**: `0` (disabled)
- **Example**:
  ```bash
  export ONAGER_API_MIN_INTERVAL_MS=200   ## max 5 calls/sec per process
  ```

##### Metadata Caching

###### ONAGER_METADATA_TTL

- **Description**: In-memory cache TTL for dataset metadata responses.
- **Type**: Integer (seconds)
- **Default**: `600` (10 minutes)
- **Example**:
  ```bash
  export ONAGER_METADATA_TTL=300
  ```

##### Download Coordination

When multiple queries attempt to download the same dataset concurrently, Onager coordinates using an in-process lock.
These settings control the wait behavior when a download is already in progress.

- **ONAGER_DOWNLOAD_WAIT_TIMEOUT**
    - **Description**: Maximum time a waiting request will block (seconds)
    - **Type**: Float or integer (seconds)
    - **Default**: `30`
    - **Example**:
      ```bash
      export ONAGER_DOWNLOAD_WAIT_TIMEOUT=600 ## 10 minutes
      ```
- **ONAGER_DOWNLOAD_WAIT_POLL**
    - **Description**: Polling interval while waiting (seconds)
    - **Type**: Float or integer (seconds)
    - **Default**: `0.1`

##### Logging Configuration

###### ONAGER_VERBOSE

- **Description**: Enable verbose logging (boolean)
- **Type**: Boolean (1/true/yes/on or 0/false/no/off)
- **Default**: `false`

###### ONAGER_LOG_LEVEL

- **Description**: Set logging level for structured logs emitted by the Rust core (via tracing)
- **Type**: String (`ERROR`, `WARN`, `INFO`, `DEBUG`, and `TRACE`); case-insensitive
- **Default**: `WARN`
- **Example**:
  ```bash
  export ONAGER_LOG_LEVEL=INFO
  ```

  Notes:
    - Logging is initialized lazily on first use (when the crate is loaded in-process or when `onager::init_logging()`
      is called). The environment variable is read once per process.
    - Logs include a level prefix and optional ANSI colors if stderr is a terminal.

##### Offline Mode

- **ONAGER_OFFLINE**
    - **Description**: Disable network access. When enabled, operations that require network will fail fast unless data
      is already cached.
    - **Type**: Boolean (`1`, `true`, `yes`, `on` to enable)
    - **Default**: `false`
    - **Effects**:
        - Downloads fail if dataset isn’t cached.
        - Search and metadata fetch fail fast.
        - Version checks use cached .downloaded metadata when available; otherwise latest_version becomes "unknown".

    - **Example**:
      ```bash
      export ONAGER_OFFLINE=1
      ```

##### On-Demand Download Behavior

- **ONAGER_STRICT_ONDEMAND**
    - **Description**: When enabled, `onager_file_path` will NOT fall back to a full dataset download if the single-file request fails.
    - **Type**: Boolean (`1`, `true`, `yes`, `on` to enable)
    - **Default**: `false`

#### Usage Examples

##### Example 1: Custom Cache Directory

```bash
### Set custom cache directory
export ONAGER_CACHE_DIR="/mnt/fast-ssd/kaggle-cache"

### Start DuckDB
./build/release/duckdb

### Check configuration
SELECT onager_search('iris', 1, 10);
```

##### Example 2: Larger Cache for Big Datasets

```bash
## Set cache to 50GB for large datasets
export ONAGER_CACHE_SIZE_LIMIT=51200

## Download and query large Kaggle datasets
./build/release/duckdb
```

##### Example 3: Production Configuration

```bash
## Complete production configuration
export ONAGER_CACHE_DIR="/var/lib/onager/cache"
export ONAGER_CACHE_SIZE_LIMIT=51200     ## 50GB
export ONAGER_HTTP_TIMEOUT=120              ## 2 minutes
export ONAGER_HTTP_RETRY_ATTEMPTS=5         ## Retry up to 5 times
export ONAGER_HTTP_RETRY_DELAY=2            ## 2 second initial delay
export ONAGER_HTTP_RETRY_MAX_DELAY=30       ## Cap backoff at 30s
export ONAGER_LOG_LEVEL=WARN                ## Production logging

### Set Kaggle credentials
export KAGGLE_USERNAME="your-username"
export KAGGLE_KEY="your-api-key"

### Run DuckDB with Onager
./build/release/duckdb
```

##### Example 4: Development/Debug Configuration

```bash
### Development setup with verbose logging
export ONAGER_CACHE_DIR="./dev-cache"
export ONAGER_LOG_LEVEL=DEBUG               ### Detailed debug logs
export ONAGER_HTTP_TIMEOUT=10               ### Shorter timeout for dev
export ONAGER_HTTP_RETRY_ATTEMPTS=1         ### Fail fast in development
export ONAGER_HTTP_RETRY_DELAY=0.25         ### Quick retry (250ms)

### Run DuckDB
./build/release/duckdb
```

##### Example 5: Slow Network Configuration

```bash
### Configuration for slow or unreliable networks
export ONAGER_HTTP_TIMEOUT=300              ### 5 minute timeout
export ONAGER_HTTP_RETRY_ATTEMPTS=10        ### Many retries
export ONAGER_HTTP_RETRY_DELAY=5            ### 5 second initial delay
export ONAGER_HTTP_RETRY_MAX_DELAY=60       ### Cap at 60s

./build/release/duckdb
```

##### Example 6: Offline Mode

```bash
## Enable offline mode
export ONAGER_OFFLINE=1

## Attempt to download a dataset (will fail if not cached)
SELECT onager_download('username/dataset-name');

## Querying metadata or searching will fail fast in offline mode
SELECT onager_info('username/dataset-name');
SELECT onager_search('keyword', 1, 10);
```

#### Configuration Verification

You can verify your configuration at runtime:

```sql
-- Check cache info (includes limit and usage)
SELECT onager_cache_info();
-- Returns: {"path": "...", "size_mb": 1024, "limit_mb": 102400, "usage_percent": 1, "is_soft_limit": true, "type": "local"}

-- Manually enforce cache limit (LRU eviction)
SELECT onager_enforce_cache_limit();

-- Search datasets (requires valid credentials)
SELECT onager_search('housing', 1, 10);

-- Get dataset metadata
SELECT onager_info('username/dataset-name');

-- Retrieve last error string (or NULL if none)
SELECT onager_last_error();
```

#### Retry Policy Details

Onager implements retries with exponential backoff for HTTP requests. The number of attempts, initial delay, and
maximum delay can be tuned with the environment variables above.

#### Logging Levels

Detailed logging control via `ONAGER_LOG_LEVEL` is implemented.

#### Units

- Storage sizes are reported in megabytes (MB) throughout the API and SQL functions.
- Timeouts and retry delays are configured in seconds via environment variables with clean names (no unit suffixes). For
  example: `ONAGER_HTTP_RETRY_DELAY=1.5`.

```sql
-- Example cache info (note size is in MB only)
SELECT onager_cache_info();
-- {"path":"...","size_mb":42,"limit_mb":102400,"usage_percent":0,"is_soft_limit":true,"type":"local"}
```
