.echo on

-- Example 3: Dataset Versioning Features
-- This example demonstrates how to work with Kaggle dataset versions

-- Section 1: Setup
select '## Setup: Load extension and set credentials';
load 'build/release/extension/gaggle/gaggle.duckdb_extension';
select gaggle_set_credentials('your-username', 'your-api-key') as credentials_set;

-- Section 2: Download a dataset (stores version metadata)
select '## Download dataset (version is automatically tracked)';
select gaggle_download('uciml/iris') as cached_path;

-- Section 3: Check version status
select '## Check if cached dataset is the latest version';
select gaggle_is_current('uciml/iris') as is_current;

-- Section 4: Get detailed version information
select '## Get detailed version information';
select gaggle_version_info('uciml/iris') as version_info;

-- Parse the JSON to extract specific fields
with version_data as (
    select gaggle_version_info('uciml/iris') as info
)
select
    json_extract_string(info, '$.cached_version') as cached_version,
    json_extract_string(info, '$.latest_version') as latest_version,
    json_extract_string(info, '$.is_current') as is_current,
    json_extract_string(info, '$.is_cached') as is_cached
from version_data;

-- Section 5: Force update to latest version
select '## Force update to latest version (ignores cache)';
-- Uncomment to actually update:
-- select gaggle_update_dataset('uciml/iris') as updated_path;

-- Section 6: Version pinning - download specific versions
select '## Version pinning: download specific versions';
-- Download version 2
-- select gaggle_download('uciml/iris@v2') as v2_path;

-- Download version 5 (without v prefix)
-- select gaggle_download('uciml/iris@5') as v5_path;

-- Explicit latest
-- select gaggle_download('uciml/iris@latest') as latest_path;

-- Compare different versions
-- select 'v2' as version, count(*) from 'kaggle:uciml/iris@v2/iris.csv'
-- UNION ALL
-- select 'v5', count(*) from 'kaggle:uciml/iris@v5/iris.csv';

-- Section 7: Smart download pattern
select '## Smart download: only update if outdated';
-- This pattern checks version and only downloads if needed
select case
    when gaggle_is_current('uciml/iris') then
        (select 'Using cached version: ' || gaggle_download('uciml/iris'))
    else
        (select 'Updating to latest: ' || gaggle_update_dataset('uciml/iris'))
end as status;

-- Section 8: Version audit across multiple datasets
select '## Version audit for multiple datasets';
-- Check versions for multiple datasets
select
    'uciml/iris' as dataset,
    json_extract_string(gaggle_version_info('uciml/iris'), '$.cached_version') as cached,
    json_extract_string(gaggle_version_info('uciml/iris'), '$.latest_version') as latest,
    json_extract_string(gaggle_version_info('uciml/iris'), '$.is_current') as current
-- Add more datasets as needed:
-- union all
-- select
--     'owner/dataset2' as dataset,
--     json_extract_string(gaggle_version_info('owner/dataset2'), '$.cached_version') as cached,
--     json_extract_string(gaggle_version_info('owner/dataset2'), '$.latest_version') as latest,
--     json_extract_string(gaggle_version_info('owner/dataset2'), '$.is_current') as current
;

-- Section 9: Data pipeline with version validation
select '## Data pipeline: make sure the latest version is retrieved before querying';
-- Step 1: Validate version
with validation as (
    select gaggle_is_current('uciml/iris') as is_current
)
select
    case
        when is_current then 'Cache is current, proceeding with query'
        else 'Cache outdated, would update (commented out for safety)'
    end as validation_status
from validation;

-- Step 2: Query data (knowing we have the right version)
-- select * from 'kaggle:uciml/iris/iris.csv' limit 10;

.echo off
