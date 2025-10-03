.echo on

-- Section 1: load extension and get version
select '## Load extension and get version';
load 'build/release/extension/gaggle/gaggle.duckdb_extension';
select gaggle_version() as version;

-- Section 2: set kaggle credentials
select '## Set Kaggle credentials';
-- Method 1: set directly (or use KAGGLE_USERNAME/KAGGLE_KEY env vars or ~/.kaggle/kaggle.json)
select gaggle_set_credentials('your-username', 'your-api-key') as credentials_set;

-- Section 3: search for datasets
select '## Search for datasets';
select gaggle_search('covid', 1, 5) as search_results;

-- Section 3b: parse search results (JSON)
select '## Parse search results (JSON)';
with search_results as (
  select from_json(gaggle_search('covid', 1, 10)) as j
)
select json_extract_string(value, '$.ref') as ref,
       json_extract_string(value, '$.title') as title
from json_each((select j from search_results))
limit 5;

-- Section 4: download a dataset
select '## Download a dataset';
select gaggle_download('uciml/iris') as download_path;

-- Section 5: list files (JSON)
select '## list files (json)';
-- Note: `path` values returned by gaggle_ls are of the form 'owner/dataset/<relative-path>' (not absolute filesystem paths)
select to_json(
         list(struct_pack(name := name, size := size, path := path))
       ) as files_json
from gaggle_ls('uciml/iris');

-- Section 5b: list files (table)
select '## list files (table)';
select * from gaggle_ls('uciml/iris') limit 5;

-- Recursive listing example (walk subdirectories)
select '## recursive listing example';
select * from gaggle_ls('suganthidevasagayam/social-media-post-of-postpartum-depression', true) limit 10;

-- Section 6: get dataset metadata
select '## get dataset metadata';
select gaggle_info('uciml/iris') as dataset_metadata;

-- Section 7: get cache information
select '## Get cache information';
select gaggle_cache_info() as cache_info;

.echo off
