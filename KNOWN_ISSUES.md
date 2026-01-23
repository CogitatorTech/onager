## Known Issues

This document lists known issues and limitations in the Onager extension.

### Issue \#1: DuckDB 1.4.x and Older: Multi-threaded Execution May Cause Errors

**Affected Versions:** DuckDB 1.4.x and earlier

**Issue:** When using Onager table functions with DuckDB `1.4.x` or older versions, running with multiple threads
(threads > 1) may cause race
condition errors during `CREATE TABLE AS SELECT` or when materializing results.

See [this issue](https://github.com/CogitatorTech/onager/issues/3) for more details.

The error typically is occurrs with a message like this:

```
INTERNAL Error:
PhysicalBatchInsert::AddCollection error: batch index 9999999999999 is present in multiple collections.
This occurs when batch indexes are not uniquely distributed over threads
```

**Root Cause:** DuckDB 1.4.x lacks the `order_preservation_type` API that was added in DuckDB 1.5.0 to properly handle
batch index management for
table functions that don't preserve insertion order.

**Workaround:** Set the number of threads to 1 before executing Onager functions:

```sql
SET threads TO 1;

-- Then run your Onager queries
CREATE TABLE results AS
SELECT * FROM onager_cmm_components((SELECT source, target FROM edges));

-- Optionally restore threads after
SET threads TO 4;
```

**Solution:** The bug is fixed in DuckDB 1.5.0 and later versions.
We recommend upgrading to DuckDB 1.5.0 or newer for full multi-threaded support with Onager.

---

### Reporting New Issues

If you encounter any issues or bugs not listed here, please report them on our [GitHub
Issues](https://github.com/CogitatorTech/onager/issues) page.
