// Onager Playground: run graph analytics in the browser via DuckDB-Wasm.
// The Onager extension is served same-origin from ./extensions, so no CORS setup is needed.

import * as duckdb from "https://cdn.jsdelivr.net/npm/@duckdb/duckdb-wasm@1.33.1-dev57.0/+esm";

const statusEl = document.getElementById("status");
const runBtn = document.getElementById("run");
const resetBtn = document.getElementById("reset");
const sqlEl = document.getElementById("sql");
const resultEl = document.getElementById("result");
const resultMetaEl = document.getElementById("result-meta");
const demoButtonsEl = document.getElementById("demo-buttons");
const demoDescEl = document.getElementById("demo-desc");

// The sample graph every demo runs against.
const SAMPLE_EDGES = `create or replace table edges as
select * from (values
  (1::bigint, 2::bigint), (2, 3), (3, 1),
  (3, 4), (4, 5), (5, 6)
) t(src, dst);`;

// Guided demos wired to real Onager functions (thread-dependent onager_par_* are avoided).
const DEMOS = [
  {
    label: "PageRank",
    desc: "Rank nodes by importance using PageRank centrality.",
    sql: `select node_id, round(rank, 4) as importance
from onager_ctr_pagerank((select src, dst from edges))
order by rank desc;`,
  },
  {
    label: "Communities (Louvain)",
    desc: "Detect clusters of densely connected nodes with the Louvain method.",
    sql: `select node_id, community
from onager_cmm_louvain((select src, dst from edges))
order by community, node_id;`,
  },
  {
    label: "Shortest Paths",
    desc: "Compute shortest-path distances from node 1 using Dijkstra's algorithm.",
    sql: `select node_id, distance
from onager_pth_dijkstra((select src, dst from edges), source := 1::bigint)
order by distance;`,
  },
  {
    label: "Graph Metrics",
    desc: "Measure structural properties of the whole graph.",
    sql: `select 'diameter' as metric, diameter::double as value
  from onager_mtr_diameter((select src, dst from edges))
union all
select 'density', density
  from onager_mtr_density((select src, dst from edges))
union all
select 'triangles', triangles::double
  from onager_mtr_triangles((select src, dst from edges));`,
  },
  {
    label: "Link Prediction",
    desc: "Score candidate links between nodes with the Jaccard coefficient.",
    sql: `select *
from onager_lnk_jaccard((select src, dst from edges))
order by 3 desc
limit 20;`,
  },
  {
    label: "Generate a Graph",
    desc: "Generate a random Erdos-Renyi graph; edge lists like this feed any Onager function.",
    sql: `select src, dst
from onager_gen_erdos_renyi(10, 0.3, seed := 42)
order by src, dst;`,
  },
];

let conn = null;

function setStatus(kind, text) {
  statusEl.className = `status status-${kind}`;
  statusEl.textContent = text;
}

function setBusy(busy) {
  runBtn.disabled = busy || !conn;
  resetBtn.disabled = busy || !conn;
}

async function init() {
  try {
    const bundles = duckdb.getJsDelivrBundles();
    const bundle = await duckdb.selectBundle(bundles);

    // Workers cannot be loaded cross-origin directly; wrap the CDN worker in a same-origin blob.
    const workerUrl = URL.createObjectURL(
      new Blob([`importScripts("${bundle.mainWorker}");`], { type: "text/javascript" })
    );
    const worker = new Worker(workerUrl);
    const logger = new duckdb.ConsoleLogger(duckdb.LogLevel.WARNING);
    const db = new duckdb.AsyncDuckDB(logger, worker);
    await db.instantiate(bundle.mainModule, bundle.pthreadWorker);
    URL.revokeObjectURL(workerUrl);

    await db.open({
      path: ":memory:",
      query: { castBigIntToDouble: true },
      allowUnsignedExtensions: true,
    });
    conn = await db.connect();

    // Load Onager from the same-origin extension repository shipped alongside this app.
    const repo = new URL("extensions", document.baseURI).href;
    await conn.query(`set custom_extension_repository = '${repo}';`);
    await conn.query(`install onager;`);
    await conn.query(`load onager;`);

    const version = await scalar("select onager_version() as v;", "onager_version");
    await conn.query(SAMPLE_EDGES);

    setStatus("ready", `Ready. Onager ${version ?? ""} loaded. Sample table "edges" created.`);
    setBusy(false);
    // Show the first demo by default so users see output immediately.
    selectDemo(0, false);
    await runQuery();
  } catch (err) {
    console.error("Onager playground init failed:", err);
    setStatus(
      "error",
      "Failed to initialize the playground.\n" +
        formatError(err) +
        "\n\nThe Onager Wasm extension may not be deployed yet, or your browser may not support DuckDB-Wasm."
    );
  }
}

// Turn any thrown value into a readable message plus stack for the on-page box.
function formatError(err) {
  if (err == null) return "Unknown error.";
  const msg = err.message ? String(err.message) : String(err);
  const stack = err.stack ? String(err.stack) : "";
  // Some stacks already start with the message; avoid duplicating it.
  if (stack && !stack.startsWith(msg)) return `${msg}\n\n${stack}`;
  return stack || msg;
}

// Read one cell defensively. Different duckdb-wasm/arrow builds expose values in
// slightly different shapes, so never let a single value break the whole render.
function readCell(vector, i) {
  try {
    return vector.get(i);
  } catch {
    return undefined;
  }
}

// Run a query returning a single scalar, tolerant of failure.
async function scalar(sql, col) {
  try {
    const res = await conn.query(sql);
    if (res.numRows === 0) return null;
    const child = res.getChild(col);
    const v = child ? child.get(0) : null;
    return v == null ? null : String(v);
  } catch {
    return null;
  }
}

function formatValue(v) {
  if (v === null || v === undefined) return "NULL";
  if (typeof v === "bigint") return v.toString();
  if (typeof v === "object") {
    // DECIMAL, nested, or other structured values: prefer a JSON-ish string.
    try {
      return typeof v.toString === "function" && v.toString !== Object.prototype.toString
        ? v.toString()
        : JSON.stringify(v);
    } catch {
      return String(v);
    }
  }
  return String(v);
}

function isNumeric(v) {
  return typeof v === "number" || typeof v === "bigint";
}

// Render an Arrow Table by reading column vectors directly (no row-proxy objects).
function renderTable(res) {
  const fields = res.schema.fields.map((f) => f.name);
  const vectors = fields.map((_, c) => res.getChildAt(c));
  const nrows = res.numRows;

  const table = document.createElement("table");
  table.className = "grid";

  const thead = table.createTHead();
  const hr = thead.insertRow();
  for (const name of fields) {
    const th = document.createElement("th");
    th.textContent = name;
    hr.appendChild(th);
  }

  const tbody = table.createTBody();
  for (let i = 0; i < nrows; i++) {
    const tr = tbody.insertRow();
    for (let c = 0; c < fields.length; c++) {
      const td = tr.insertCell();
      const value = vectors[c] ? readCell(vectors[c], i) : undefined;
      td.textContent = formatValue(value);
      if (isNumeric(value)) td.className = "num";
    }
  }

  resultEl.replaceChildren(table);
  resultMetaEl.textContent = `${nrows} row${nrows === 1 ? "" : "s"}, ${fields.length} column${
    fields.length === 1 ? "" : "s"
  }.`;
}

async function runQuery() {
  if (!conn) return;
  const sql = sqlEl.value.trim();
  if (!sql) return;
  setBusy(true);
  resultMetaEl.textContent = "Running…";
  try {
    const res = await conn.query(sql);
    renderTable(res);
    setStatus("ready", "Query finished.");
  } catch (err) {
    console.error("Onager playground query failed:", err);
    resultEl.replaceChildren();
    resultMetaEl.textContent = "";
    const detail = await scalar("select onager_last_error() as onager_last_error;", "onager_last_error");
    setStatus(
      "error",
      "Query failed.\n" +
        formatError(err) +
        (detail ? `\n\nOnager: ${detail}` : "")
    );
  } finally {
    setBusy(false);
  }
}

function selectDemo(index, run = true) {
  const demo = DEMOS[index];
  if (!demo) return;
  sqlEl.value = demo.sql;
  demoDescEl.textContent = demo.desc;
  for (const btn of demoButtonsEl.children) {
    btn.classList.toggle("active", Number(btn.dataset.index) === index);
  }
  if (run) runQuery();
}

function buildDemoButtons() {
  DEMOS.forEach((demo, i) => {
    const btn = document.createElement("button");
    btn.className = "btn";
    btn.textContent = demo.label;
    btn.dataset.index = String(i);
    btn.addEventListener("click", () => selectDemo(i));
    demoButtonsEl.appendChild(btn);
  });
}

runBtn.addEventListener("click", runQuery);
resetBtn.addEventListener("click", async () => {
  setBusy(true);
  try {
    await conn.query(SAMPLE_EDGES);
    setStatus("ready", 'Sample table "edges" recreated.');
  } catch (err) {
    setStatus("error", "Failed to reset sample data.\n" + String(err));
  } finally {
    setBusy(false);
  }
});
sqlEl.addEventListener("keydown", (e) => {
  if ((e.ctrlKey || e.metaKey) && e.key === "Enter") {
    e.preventDefault();
    runQuery();
  }
});

// Surface anything that escapes the try/catch blocks so it never fails silently.
window.addEventListener("error", (e) => {
  console.error("Onager playground uncaught error:", e.error || e.message);
});
window.addEventListener("unhandledrejection", (e) => {
  console.error("Onager playground unhandled rejection:", e.reason);
});

buildDemoButtons();
init();
