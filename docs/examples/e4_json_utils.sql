.echo on

-- Example 4: JSON helper utilities
-- Shows how to expand JSON using gaggle_json_each

select '## Load extension';
load 'build/release/extension/gaggle/gaggle.duckdb_extension';

select '## Expand JSON values into newline-delimited JSON rows';
select gaggle_json_each('{"a":1,"b":[true,{"c":"x"}],"d":null}') as rows;

-- Combine with DuckDB JSON functions
with x as (
  select gaggle_json_each('{"a":1,"b":[true,{"c":"x"}],"d":null}') as row
)
select
  json_type(row)   as value_type,
  json_extract(row, '$') as raw,
  json_extract_string(row, '$.key') as key,
  json_extract(row, '$.value') as value
from x;

.echo off
