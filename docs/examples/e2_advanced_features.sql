.echo on

-- Section 1: get file path and read parquet directly
select '## Read Parquet from Kaggle dataset directly';
load 'build/release/extension/gaggle/gaggle.duckdb_extension';
select gaggle_set_credentials('your-username', 'your-api-key') as credentials_set;

-- Get path to specific file
select gaggle_file_path('habedi/flickr-8k-dataset-clean', 'flickr8k.parquet') as file_path;

-- Use the file path with DuckDB's read_parquet via prepared statement (no subqueries in args)
prepare rp as select * from read_parquet(?) limit 10;
execute rp(gaggle_file_path('habedi/flickr-8k-dataset-clean', 'flickr8k.parquet'));

-- Section 2: list and process multiple files
select '## list and process dataset files (json and table)';
-- Note: `path` values returned by gaggle_ls are of the form 'owner/dataset/<relative-path>' (not absolute filesystem paths)
with files as (
  select to_json(list(struct_pack(name := name, size := size, path := path))) as files_json
  from gaggle_ls('habedi/flickr-8k-dataset-clean')
)
select files_json from files;
select * from gaggle_ls('habedi/flickr-8k-dataset-clean') limit 5;

-- Recursive listing (example)
select '## recursive listing example for flickr dataset';
select * from gaggle_ls('habedi/flickr-8k-dataset-clean', true) limit 10;

-- Section 2b: use replacement scan for direct reads via `kaggle:` URLs
select '## Replacement scan - direct reads via `kaggle:`';
-- Single file read
select count(*) as cnt from 'kaggle:habedi/flickr-8k-dataset-clean/flickr8k.parquet';
-- Glob pattern over parquet files
select count(*) as cnt from 'kaggle:habedi/flickr-8k-dataset-clean/*.parquet';

-- Section 3: download and verify cache
select '## Verify dataset is cached';
select gaggle_download('habedi/flickr-8k-dataset-clean') as cached_path;
select gaggle_cache_info() as cache_status;

-- Section 4: purge cache if needed
select '## Purge cache (optional)';
-- select gaggle_clear_cache() as cache_cleared;

-- Section 5: Dataset versioning
select '## Check dataset versions';
-- Check if cached dataset is the latest version
select gaggle_is_current('habedi/flickr-8k-dataset-clean') as is_current;

-- Get detailed version information
select gaggle_version_info('habedi/flickr-8k-dataset-clean') as version_info;

-- Force update to latest version (if cache is outdated)
-- select gaggle_update_dataset('habedi/flickr-8k-dataset-clean') as updated_path;

.echo off
