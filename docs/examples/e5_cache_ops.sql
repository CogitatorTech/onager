.echo on

-- Example 5: Cache operations and housekeeping
-- Demonstrates gaggle_version, gaggle_cache_info, gaggle_clear_cache, gaggle_enforce_cache_limit

select '## Load extension';
load 'build/release/extension/gaggle/gaggle.duckdb_extension';

select '## Extension version';
select gaggle_version() as version;

select '## Cache info (path, size, limit)';
select gaggle_cache_info() as cache_info_json;

select '## Clear cache (optional)';
-- Uncomment to clear the local dataset cache
-- select gaggle_clear_cache() as cache_cleared;

select '## Enforce cache size limit (LRU eviction)';
-- This triggers cleanup based on the configured limit; safe to run repeatedly
select gaggle_enforce_cache_limit() as enforced;

.echo off
