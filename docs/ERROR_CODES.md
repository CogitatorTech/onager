### Overview

Onager uses standardized error codes to make error handling more predictable and debugging easier.
Each error includes a numeric code (E001 to E010) that can be used programmatically.
When troubleshooting, look for the bracketed code (like \[E003\]) and refer to the corresponding section below.

#### Error Code Format

All errors follow this format:

```
[Exxx] Error description: additional details
```

Example:

```
[E002] Dataset not found: owner/invalid-dataset
```

#### Error Codes

##### E001 - Invalid Credentials

**Description:**  
Kaggle API credentials are invalid, missing, or incorrectly formatted.

**Common Causes:**

- Wrong username or API key
- Missing credentials (no environment variables or kaggle.json)
- Expired API key
- Incorrectly formatted kaggle.json file

**Example:**

```
[E001] Invalid Kaggle credentials: Username or API key not found
```

**Solutions:**

- Set credentials via SQL: `select onager_set_credentials('your-username', 'your-api-key');`
- Or via env: `export KAGGLE_USERNAME=...` and `export KAGGLE_KEY=...`
- Or create `~/.kaggle/kaggle.json` with username/key (chmod 600)

---

##### E002 - Dataset Not Found

**Description:**  
The requested dataset does not exist on Kaggle or is not accessible.

**Common Causes:**

- Typo in dataset path
- Dataset was deleted or made private
- Wrong owner name
- Dataset requires special permissions

**Example:**

```
[E002] Dataset not found: owner/nonexistent-dataset
```

**Solutions:**

1. **Verify dataset path on Kaggle:**
    - Visit https://www.kaggle.com/datasets/owner/dataset-name
    - Check spelling and owner name

2. **Search for the dataset:**
   ```sql
   select onager_search('dataset keywords', 1, 10);
   ```

3. **Check dataset availability:**
    - Check dataset is public
    - Verify you have access rights

---

##### E003 - Network Error

**Description:**  
Network error occurred during communication with Kaggle API.

**Common Causes:**

- No internet connection
- Kaggle API is down
- Firewall blocking requests
- A timeout happened
- Rate limiting

**Example:**

```
[E003] HTTP request failed: Connection timeout after 30s
```

**Solutions:**

1. **Check internet connection:**
   ```bash
   ping www.kaggle.com
   ```

2. **Increase timeout:**
   ```bash
   export ONAGER_HTTP_TIMEOUT=120  # 2 minutes
   ```

3. **Check Kaggle API status:**
    - Check https://www.kaggle.com is accessible

4. **Retry with backoff:**
   ```bash
   export ONAGER_HTTP_RETRY_ATTEMPTS=5
   export ONAGER_HTTP_RETRY_DELAY=2
   export ONAGER_HTTP_RETRY_MAX_DELAY=30
   ```

5. **Check firewall settings:**
    - Check outbound HTTPS (port 443) is allowed
    - Check corporate proxy settings

---

##### E004 - Invalid Path

**Description:**  
Dataset path format is invalid or contains forbidden characters.

**Common Causes:**

- Missing slash in path
- Path traversal attempts (../)
- Too many path components
- Control characters in path
- Path too long (>4096 characters)

**Example:**

```
[E004] Invalid dataset path: Must be in format 'owner/dataset-name'
```

**Valid Path Format:**

```
owner/dataset-name
owner/dataset-name@v2  (with version)
```

**Invalid Paths:**

```
ownerdataset         # Missing slash
owner/dataset/extra  # Too many components
../dataset           # Path traversal
owner/.              # Dot component
```

**Solutions:**

1. **Use correct format:**
   ```sql
   select onager_download('owner/dataset-name');
   ```

2. **Check for special characters:**
    - Avoid: `..`, `.`, control characters
    - Allowed: letters, numbers, hyphens, underscores

---

##### E005 - File System Error

**Description:**  
Error reading from or writing to the file system.

**Common Causes:**

- Insufficient disk space
- Permission denied
- File not found
- Directory not writable
- Disk full

**Example:**

```
[E005] IO error: Permission denied (os error 13)
```

**Solutions:**

1. **Check disk space:**
   ```bash
   df -h
   ```

2. **Check permissions:**
   ```bash
   ls -la ~/.cache/onager_cache
   chmod -R u+rw ~/.cache/onager_cache
   ```

3. **Verify cache directory:**
   ```sql
   select onager_cache_info();
   ```

4. **Change cache directory:**
   ```bash
   export ONAGER_CACHE_DIR=/path/with/space
   ```

5. **Clean up cache:**
   ```sql
   select onager_clear_cache();
   ```

---

##### E006 - JSON Error

**Description:**  
Error parsing or serializing JSON data.

**Common Causes:**

- Corrupted cache metadata
- Invalid JSON response from Kaggle API
- Encoding issues
- Malformed JSON

**Example:**

```
[E006] JSON serialization error: expected `,` or `}` at line 5 column 10
```

**Solutions:**

1. **Clear cache:**
   ```sql
   select onager_clear_cache();
   ```

2. **Re-download dataset:**
   ```sql
   select onager_update_dataset('owner/dataset');
   ```

3. **Check Kaggle API response manually:**
   ```bash
   curl -u username:key https://www.kaggle.com/api/v1/datasets/view/owner/dataset
   ```

---

##### E007 - ZIP Extraction Error

**Description:**  
Error extracting downloaded ZIP file.

**Common Causes:**

- Corrupted download
- ZIP bomb protection triggered (>10GB uncompressed)
- Path traversal in ZIP
- Symlinks in ZIP
- Invalid ZIP format

**Example:**

```
[E007] ZIP extraction failed: ZIP file too large (exceeds 10GB)
```

**Solutions:**

1. **Re-download dataset:**
   ```sql
   select onager_update_dataset('owner/dataset');
   ```

2. **Check dataset size:**
   ```sql
   select onager_info('owner/dataset');
   ```

3. **For large datasets:**
    - Note: 10GB uncompressed limit is a security feature
    - Consider using a different dataset or smaller subset

4. **Check ZIP integrity:**
   ```bash
   unzip -t /path/to/dataset.zip
   ```

---

##### E008 - CSV Parsing Error

**Description:**  
Error parsing CSV file format.

**Common Causes:**

- Malformed CSV
- Inconsistent column count
- Invalid quotes or delimiters
- Encoding issues

**Example:**

```
[E008] CSV parsing error: record 145 has different field count
```

**Solutions:**

1. **Check CSV format:**
   ```bash
   head -20 /path/to/file.csv
   ```

2. **Use DuckDB's flexible CSV reader:**
   ```sql
   select * FROM read_csv_auto('kaggle:owner/dataset/file.csv',
                                ignore_errors := true);
   ```

3. **Try different parser options:**
   ```sql
   select * FROM read_csv('kaggle:owner/dataset/file.csv',
                          delim := ';',
                          quote := '"',
                          escape := '\\');
   ```

---

##### E009 - UTF-8 Encoding Error

**Description:**  
String is not valid UTF-8.

**Common Causes:**

- Binary data in string field
- Wrong character encoding
- Corrupted data
- FFI boundary issues

**Example:**

```
[E009] Invalid UTF-8 string
```

**Solutions:**

1. **Check file encoding:**
   ```bash
   file -i /path/to/file.csv
   ```

2. **Convert to UTF-8:**
   ```bash
   iconv -f ISO-8859-1 -t UTF-8 input.csv > output.csv
   ```

3. **Use DuckDB encoding options:**
   ```sql
   select * FROM read_csv('file.csv', encoding := 'ISO-8859-1');
   ```

---

##### E010 - Null Pointer Error

**Description:**  
NULL pointer passed to FFI function.

**Common Causes:**

- Internal programming error
- Invalid function call
- Memory corruption

**Example:**

```
[E010] Null pointer passed
```

**Solutions:**

- This is typically an internal error
- Report as a bug if you encounter this
- Include reproduction steps
