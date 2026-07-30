// Asserts that the vendored DuckDB-Wasm runtime reports the DuckDB version the
// playground ships extensions for. DuckDB resolves extensions as
// <repository>/<duckdb version>/<platform>/onager.duckdb_extension.wasm, so a
// runtime that disagrees with the extension build requests a directory that does
// not exist, and the playground fails to start with an unrelated-looking error.
//
// Usage: node check-duckdb-version.mjs v1.5.5

import { createRequire } from "node:module";
import path from "node:path";

const require = createRequire(import.meta.url);
const entry = "@duckdb/duckdb-wasm/dist/duckdb-node-blocking.cjs";
const duckdb = require(entry);
const distDir = path.dirname(require.resolve(entry));

const expected = process.argv[2];
if (!expected) {
  console.error("usage: node check-duckdb-version.mjs <duckdb version, for example v1.5.5>");
  process.exit(2);
}

const bundles = {
  mvp: { mainModule: path.join(distDir, "duckdb-mvp.wasm"), mainWorker: null },
  eh: { mainModule: path.join(distDir, "duckdb-eh.wasm"), mainWorker: null },
};

const db = await duckdb.createDuckDB(bundles, new duckdb.VoidLogger(), duckdb.NODE_RUNTIME);
await db.instantiate();
const conn = db.connect();
const actual = conn.query("select version() as v;").get(0).v;
conn.close();

if (actual !== expected) {
  console.error(
    `DuckDB version mismatch: the vendored DuckDB-Wasm runtime reports ${actual}, but the ` +
      `playground ships extensions built for ${expected}. Update @duckdb/duckdb-wasm in ` +
      "web/vendor-src/package.json and DUCKDB_WASM_VERSION in web/app.js to the release that " +
      "matches the extension build."
  );
  process.exit(1);
}

console.log(`The vendored DuckDB-Wasm runtime reports ${actual}, matching the extension build.`);
