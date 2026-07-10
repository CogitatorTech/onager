// Onager Playground: run graph analytics in the browser via DuckDB-Wasm.
// The Onager extension is served same-origin from ./extensions, or falls back to live GitHub Pages build.

import * as duckdb from "https://cdn.jsdelivr.net/npm/@duckdb/duckdb-wasm@1.33.1-dev57.0/+esm";

const statusEl = document.getElementById("status");
const runBtn = document.getElementById("run");
const resetBtn = document.getElementById("reset");
const sqlEl = document.getElementById("sql");
const resultEl = document.getElementById("result");
const resultMetaEl = document.getElementById("result-meta");
const demoButtonsEl = document.getElementById("demo-buttons");
const demoDescEl = document.getElementById("demo-desc");
const demoCategoryEl = document.getElementById("demo-category");
const demoDocsLinkEl = document.getElementById("demo-docs-link");
const functionSearchEl = document.getElementById("function-search");
const functionListEl = document.getElementById("function-list");
const shareBtn = document.getElementById("share");
const clearSqlBtn = document.getElementById("clear-sql");
const loadCsvBtn = document.getElementById("load-csv-btn");
const csvFileInput = document.getElementById("csv-file-input");
const sqlBackdropEl = document.getElementById("sql-backdrop");
const sqlHighlightEl = document.getElementById("sql-highlight");


// Form inputs & tabs
const templateSelect = document.getElementById("graph-template");
const edgesInput = document.getElementById("custom-edges-input");
const updateGraphBtn = document.getElementById("update-graph-btn");
const tabTable = document.getElementById("tab-table");
const tabGraph = document.getElementById("tab-graph");
const tableContainer = document.getElementById("result-table-container");
const graphContainer = document.getElementById("result-graph-container");
const resultsActions = document.getElementById("results-actions-container");
const exportCsvBtn = document.getElementById("export-csv");
const exportJsonBtn = document.getElementById("export-json");
const fitGraphBtn = document.getElementById("graph-fit");
const svgEl = document.getElementById("graph-svg");

// Default Krackhardt Kite Graph Edges
const SAMPLE_EDGES = `create or replace table edges as
select * from (values
  (1::bigint, 2::bigint), (1, 3), (2, 3), (2, 4), (3, 4),
  (2, 6), (3, 5), (4, 5), (4, 6), (5, 6),
  (5, 7), (6, 7), (7, 8), (8, 9), (9, 10)
) t(src, dst);`;

// Rendering caps that keep the UI responsive on large results.
const MAX_TABLE_ROWS = 1000;
const MAX_GRAPH_NODES = 100;
const MAX_HISTORY_ITEMS = 5;

// Graph templates configuration
const GRAPH_TEMPLATES = {
  kite: `1, 2\n1, 3\n2, 3\n2, 4\n3, 4\n2, 6\n3, 5\n4, 5\n4, 6\n5, 6\n5, 7\n6, 7\n7, 8\n8, 9\n9, 10`,
  clique: `1, 2\n1, 3\n1, 4\n1, 5\n2, 3\n2, 4\n2, 5\n3, 4\n3, 5\n4, 5`,
  star: `1, 2\n1, 3\n1, 4\n1, 5\n1, 6`,
  cycle: `1, 2\n2, 3\n3, 4\n4, 5\n5, 6\n6, 1`,
  tree: `1, 2\n1, 3\n2, 4\n2, 5\n3, 6\n3, 7`
};

// Generator presets. Each query is seeded, so the same option always produces the
// same edge list. Generators return edges only, so isolated nodes are not included.
const GENERATOR_TEMPLATES = {
  "gen-erdos-renyi": {
    label: "Erdos-Renyi",
    sql: "select src, dst from onager_gen_erdos_renyi(20, 0.15, seed := 68) order by src, dst;",
  },
  "gen-barabasi-albert": {
    label: "Barabasi-Albert",
    sql: "select src, dst from onager_gen_barabasi_albert(20, 2, seed := 68) order by src, dst;",
  },
  "gen-watts-strogatz": {
    label: "Watts-Strogatz",
    sql: "select src, dst from onager_gen_watts_strogatz(20, 4, 0.2, seed := 68) order by src, dst;",
  },
};

// Example catalog grouped by algorithm category, mirroring the docs guide pages.
// Every query runs against the editable "edges" table unless noted otherwise.
const DEMO_CATEGORIES = [
  {
    label: "Centrality",
    docs: "../guide/centrality/",
    demos: [
      {
        label: "PageRank",
        desc: "Rank nodes by importance using PageRank. Node size reflects the score in the graph view.",
        sql: `select node_id, round(rank, 4) as rank
from onager_ctr_pagerank((select src, dst from edges))
order by rank desc;`,
      },
      {
        label: "Weig. PageRank",
        desc: "Rank nodes with edge weights, so rank flows in proportion to each edge's weight.",
        sql: `select node_id, round(rank, 4) as rank
from onager_ctr_pagerank((select src, dst, (src + dst)::double as weight from edges))
order by rank desc;`,
      },
      {
        label: "Dir. PageRank",
        desc: "Treat each edge as one-way with directed := true. Most functions accept this parameter; the default is undirected.",
        sql: `select node_id, round(rank, 4) as rank
from onager_ctr_pagerank((select src, dst from edges), directed := true)
order by rank desc;`,
      },
      {
        label: "Betweenness",
        desc: "Count how often each node sits on shortest paths between other nodes.",
        sql: `select node_id, round(betweenness, 4) as score
from onager_ctr_betweenness((select src, dst from edges))
order by score desc;`,
      },
      {
        label: "Closeness",
        desc: "Score nodes by how close they are to every other node.",
        sql: `select node_id, round(closeness, 4) as score
from onager_ctr_closeness((select src, dst from edges))
order by score desc;`,
      },
      {
        label: "VoteRank",
        desc: "Find the three most influential spreader nodes using VoteRank.",
        sql: `select node_id
from onager_ctr_voterank((select src, dst from edges), num_seeds := 3);`,
      },
      {
        label: "Eigenvector",
        desc: "Score nodes by the importance of their neighbors using eigenvector centrality.",
        sql: `select node_id, round(eigenvector, 4) as score
from onager_ctr_eigenvector((select src, dst from edges))
order by score desc;`,
      },
      {
        label: "Katz",
        desc: "Score nodes by counting all paths, attenuated by alpha, with a constant base weight beta.",
        sql: `select node_id, round(katz, 4) as score
from onager_ctr_katz((select src, dst from edges), alpha := 0.1, beta := 1.0)
order by score desc;`,
      },
      {
        label: "Harmonic",
        desc: "Score nodes by the sum of inverse distances to every other node. Works on disconnected graphs.",
        sql: `select node_id, round(harmonic, 4) as score
from onager_ctr_harmonic((select src, dst from edges))
order by score desc;`,
      },
      {
        label: "Laplacian",
        desc: "Score nodes by their contribution to the graph Laplacian energy.",
        sql: `select node_id, round(centrality, 4) as score
from onager_ctr_laplacian((select src, dst from edges))
order by score desc;`,
      },
      {
        label: "Local Reaching",
        desc: "Score nodes by the fraction of the graph reachable within two hops.",
        sql: `select node_id, round(centrality, 4) as score
from onager_ctr_local_reaching((select src, dst from edges), distance := 2)
order by score desc;`,
      },
      {
        label: "Person. PageRank",
        desc: "Bias the PageRank random walk toward node 1. The input adds a personalization node and weight per row.",
        sql: `select node_id, round(score, 4) as score
from onager_ctr_personalized_pagerank((
  select e.src, e.dst, p.node as pers_node, p.weight::double as pers_weight
  from edges e
  cross join (values (1::bigint, 1.0::double)) p(node, weight)
))
order by score desc;`,
      },
    ],
  },
  {
    label: "Community Detection",
    docs: "../guide/community/",
    demos: [
      {
        label: "Louvain",
        desc: "Detect communities using Louvain modularity optimization. Colors show the communities in the graph view.",
        sql: `select node_id, community
from onager_cmm_louvain((select src, dst from edges))
order by community, node_id;`,
      },
      {
        label: "Label Propagation",
        desc: "Detect communities by propagating labels between neighbors. Results can vary between runs.",
        sql: `select node_id, label as community
from onager_cmm_label_prop((select src, dst from edges))
order by community, node_id;`,
      },
      {
        label: "Components",
        desc: "Find connected components. Try deleting an edge in the builder to split the graph.",
        sql: `select node_id, component as community
from onager_cmm_components((select src, dst from edges))
order by community, node_id;`,
      },
      {
        label: "Girvan-Newman",
        desc: "Split the graph into two communities by removing high-betweenness edges.",
        sql: `select node_id, community
from onager_cmm_girvan_newman((select src, dst from edges), communities := 2)
order by community, node_id;`,
      },
      {
        label: "Spectral",
        desc: "Cluster nodes into two communities using the graph Laplacian's eigenvectors.",
        sql: `select node_id, community
from onager_cmm_spectral((select src, dst from edges), k := 2, seed := 68)
order by community, node_id;`,
      },
      {
        label: "Infomap",
        desc: "Detect communities by minimizing the description length of random walks.",
        sql: `select node_id, community
from onager_cmm_infomap((select src, dst from edges), seed := 68)
order by community, node_id;`,
      },
    ],
  },
  {
    label: "Paths and Traversal",
    docs: "../guide/traversal/",
    demos: [
      {
        label: "Dijkstra",
        desc: "Compute shortest-path distances from node 1 using Dijkstra's algorithm.",
        sql: `select node_id, distance
from onager_pth_dijkstra((select src, dst from edges), source := 1::bigint)
order by distance;`,
      },
      {
        label: "Weighted Dijkstra",
        desc: "Compute shortest-path distances from node 1 with per-edge weights instead of hop counts.",
        sql: `select node_id, distance
from onager_pth_dijkstra((select src, dst, (src + dst)::double as weight from edges), source := 1::bigint)
order by distance;`,
      },
      {
        label: "BFS Order",
        desc: "Visit nodes in breadth-first order starting from node 1.",
        sql: `select node_id
from onager_trv_bfs((select src, dst from edges), source := 1);`,
      },
      {
        label: "All-Pairs Distances",
        desc: "Compute distances between every pair of nodes with Floyd-Warshall, using weight 1 per edge. The table shows the farthest pairs.",
        sql: `select src as node1, dst as node2, distance
from onager_pth_floyd_warshall((select src, dst, 1.0::double as weight from edges))
order by distance desc, node1, node2
limit 10;`,
      },
      {
        label: "Bellman-Ford",
        desc: "Compute weighted shortest-path distances from node 1. Unlike Dijkstra, negative weights are allowed.",
        sql: `select node_id, distance
from onager_pth_bellman_ford((select src, dst, (src + dst)::double as weight from edges), source := 1)
order by distance;`,
      },
      {
        label: "DFS Order",
        desc: "Visit nodes in depth-first order starting from node 1.",
        sql: `select node_id
from onager_trv_dfs((select src, dst from edges), source := 1);`,
      },
    ],
  },
  {
    label: "Minimum Spanning Tree",
    docs: "../guide/mst/",
    demos: [
      {
        label: "Kruskal",
        desc: "Build a minimum spanning tree with Kruskal's algorithm. Weights are synthesized from node ids, and the graph view shows the tree.",
        sql: `select src, dst, round(weight, 1) as weight
from onager_mst_kruskal((select src, dst, (src + dst)::double as weight from edges))
order by weight;`,
      },
      {
        label: "Prim",
        desc: "Build a minimum spanning tree with Prim's algorithm on the same synthesized weights.",
        sql: `select src, dst, round(weight, 1) as weight
from onager_mst_prim((select src, dst, (src + dst)::double as weight from edges))
order by weight;`,
      },
    ],
  },
  {
    label: "Graph Metrics",
    docs: "../guide/metrics/",
    demos: [
      {
        label: "Summary",
        desc: "Measure a few global structural properties of the graph.",
        sql: `select 'diameter' as metric, diameter::double as value
  from onager_mtr_diameter((select src, dst from edges))
union all
select 'density', density
  from onager_mtr_density((select src, dst from edges))
union all
-- Each triangle is counted once per member node, so divide the sum by three.
select 'triangles', (sum(triangles) / 3)::double
  from onager_mtr_triangles((select src, dst from edges));`,
      },
      {
        label: "Clustering",
        desc: "Compare transitivity, average clustering, and degree assortativity.",
        sql: `select 'transitivity' as metric, transitivity as value
  from onager_mtr_transitivity((select src, dst from edges))
union all
select 'avg_clustering', avg_clustering
  from onager_mtr_avg_clustering((select src, dst from edges))
union all
select 'assortativity', assortativity
  from onager_mtr_assortativity((select src, dst from edges));`,
      },
      {
        label: "Distances",
        desc: "Compare the graph radius with the average shortest-path length.",
        sql: `select 'radius' as metric, radius::double as value
  from onager_mtr_radius((select src, dst from edges))
union all
select 'avg_path_length', avg_path_length
  from onager_mtr_avg_path_length((select src, dst from edges));`,
      },
    ],
  },
  {
    label: "Link Prediction",
    docs: "../guide/links/",
    demos: [
      {
        label: "Jaccard",
        desc: "Score candidate links between nodes using the Jaccard coefficient.",
        sql: `select *
from onager_lnk_jaccard((select src, dst from edges))
order by 3 desc
limit 20;`,
      },
      {
        label: "Adamic-Adar",
        desc: "Score candidate links with the Adamic-Adar index, which favors rare shared neighbors.",
        sql: `select node1, node2, round(score, 3) as score
from onager_lnk_adamic_adar((select src, dst from edges))
order by score desc
limit 15;`,
      },
      {
        label: "Common Neighbors",
        desc: "Count shared neighbors for each candidate pair.",
        sql: `select *
from onager_lnk_common_neighbors((select src, dst from edges))
order by 3 desc
limit 15;`,
      },
      {
        label: "Preferential Attachment",
        desc: "Score candidate links by the product of node degrees, favoring hubs.",
        sql: `select *
from onager_lnk_pref_attach((select src, dst from edges))
order by 3 desc
limit 15;`,
      },
      {
        label: "Resource Allocation",
        desc: "Score candidate links by resources shared through common neighbors.",
        sql: `select *
from onager_lnk_resource_alloc((select src, dst from edges))
order by 3 desc
limit 15;`,
      },
    ],
  },
  {
    label: "Subgraphs",
    docs: "../guide/subgraphs/",
    demos: [
      {
        label: "Ego Graph",
        desc: "Extract the neighborhood around node 5. The graph view shows only the extracted subgraph.",
        sql: `select src, dst
from onager_sub_ego_graph((select src, dst from edges), center := 5, radius := 1);`,
      },
      {
        label: "K-Hop Neighbors",
        desc: "List every node within two hops of node 1.",
        sql: `select node_id
from onager_sub_k_hop((select src, dst from edges), start := 1, k := 2)
order by node_id;`,
      },
      {
        label: "Induced Subgraph",
        desc: "Keep only the edges whose endpoints are both in a given node set. The set arrives as a third input column.",
        sql: `select src, dst
from onager_sub_induced((
  select e.src, e.dst, n.node as filter_node
  from edges e
  cross join (values (1::bigint), (2), (3), (4), (5)) n(node)
));`,
      },
    ],
  },
  {
    label: "Approximation",
    docs: "../guide/approximation/",
    demos: [
      {
        label: "Max Clique",
        desc: "Approximate the largest set of nodes that are all connected to each other.",
        sql: `select node_id
from onager_apx_max_clique((select src, dst from edges))
order by node_id;`,
      },
      {
        label: "Vertex Cover",
        desc: "Approximate the smallest set of nodes that touches every edge.",
        sql: `select node_id
from onager_apx_vertex_cover((select src, dst from edges))
order by node_id;`,
      },
      {
        label: "Independent Set",
        desc: "Approximate the largest set of nodes with no edges between them.",
        sql: `select node_id
from onager_apx_independent_set((select src, dst from edges))
order by node_id;`,
      },
      {
        label: "TSP Tour",
        desc: "Approximate a traveling salesman tour over the graph, starting from node 1.",
        sql: `select "order", node_id
from onager_apx_tsp((select src, dst, (src + dst)::double as weight from edges), start := 1)
order by "order";`,
      },
    ],
  },
  {
    label: "Generators",
    docs: "../guide/generators/",
    demos: [
      {
        label: "Erdos-Renyi",
        desc: "Generate a random graph. The graph view shows the generated edges instead of the edges table.",
        sql: `select src, dst
from onager_gen_erdos_renyi(10, 0.35, seed := 68)
order by src, dst;`,
      },
      {
        label: "Watts-Strogatz",
        desc: "Generate a small-world graph: a ring lattice with a few rewired shortcuts.",
        sql: `select src, dst
from onager_gen_watts_strogatz(20, 4, 0.2, seed := 68)
order by src, dst;`,
      },
      {
        label: "Scale-Free Hubs",
        desc: "Compose a generator with degree centrality to find the hubs of a Barabasi-Albert graph. See the table view.",
        sql: `select node_id as hub, in_degree + out_degree as degree
from onager_ctr_degree((select src, dst from onager_gen_barabasi_albert(40, 2, seed := 68)))
order by degree desc
limit 10;`,
      },
    ],
  },
  {
    label: "Graph Registry",
    docs: "../guide/graph-registry/",
    demos: [
      {
        label: "1. Create Graph",
        desc: "Run the registry examples in order. This creates a named graph in the registry (0 means success).",
        sql: `select onager_create_graph('my_graph', false) as status;`,
      },
      {
        label: "2. Add Edges",
        desc: "Add three weighted edges to the named graph (run 1. Create Graph first).",
        sql: `select onager_add_edge('my_graph', 1, 2, 1.0) as e1,
       onager_add_edge('my_graph', 2, 3, 1.0) as e2,
       onager_add_edge('my_graph', 3, 1, 1.0) as e3;`,
      },
      {
        label: "3. Inspect",
        desc: "Inspect the named graph with scalar registry functions.",
        sql: `select onager_node_count('my_graph') as nodes,
       onager_edge_count('my_graph') as edges,
       onager_list_graphs() as graphs;`,
      },
    ],
  },
];

let conn = null;
let lastResult = null;
let queryHistory = [];
let activeTab = "table";

// Graph simulation state
let simulation = null;
let ticks = 0;
let graphData = { nodes: [], links: [] };
let draggedNode = null;

// Determine local vs remote extension repository URL
async function getExtensionRepository() {
  const localRepo = new URL("extensions", document.baseURI).href;
  const isLocal = window.location.hostname === "localhost" || window.location.hostname === "127.0.0.1";
  if (isLocal) {
    try {
      const testUrl = `${localRepo}/v1.5.4/wasm_mvp/onager.duckdb_extension.wasm`;
      const res = await fetch(testUrl, { method: "HEAD" });
      if (res.ok) {
        console.log("Using local extension repository:", localRepo);
        return localRepo;
      }
    } catch (e) {
      // ignore
    }
    const fallbackRepo = "https://cogitatortech.github.io/onager/playground/extensions";
    console.log("Local extensions folder not found. Falling back to remote extension repository:", fallbackRepo);
    return fallbackRepo;
  }
  return localRepo;
}

function updateFooterVersions(duckdbVersion, onagerVersion) {
  const footerNote = document.getElementById("footer-note");
  if (!footerNote) return;
  const duckdbLabel = duckdbVersion ? `DuckDB-Wasm (${duckdbVersion})` : "DuckDB-Wasm";
  const onagerLabel = onagerVersion ? `Onager (${onagerVersion})` : "Onager";
  footerNote.textContent =
    `This playground app is powered by ${duckdbLabel} and ${onagerLabel}, ` +
    "and everything (including the queries) runs safely in your browser.";
}

function setStatus(kind, text) {
  statusEl.className = `status status-${kind}`;
  if (kind === "loading") {
    statusEl.innerHTML = `<div class="spinner"></div><span>${text}</span>`;
  } else {
    statusEl.textContent = text;
  }
}

function setBusy(busy) {
  runBtn.disabled = busy || !conn;
  resetBtn.disabled = busy || !conn;
  updateGraphBtn.disabled = busy || !conn;
  shareBtn.disabled = busy || !conn;
  clearSqlBtn.disabled = busy || !conn;
  loadCsvBtn.disabled = busy || !conn;
}

async function init() {
  initTheme();
  loadHistory();
  setupTabs();
  setupDragBehavior();
  setupExporters();
  setupGraphTemplateListeners();
  setupHighlighting();
  setupCsvUpload();

  try {
    const bundles = duckdb.getJsDelivrBundles();
    // Force the MVP (non-exception) bundle instead of letting selectBundle pick "eh".
    // The Onager extension embeds Rust code compiled for wasm32-unknown-emscripten, whose
    // exception model does not match DuckDB-Wasm's native-wasm-exceptions ("eh") runtime.
    // Mixing them makes function pointers mismatch at call time, surfacing as
    // "indirect call signature mismatch" / "index out of bounds" once a query runs. The MVP
    // runtime and the wasm_mvp extension share a consistent ABI, so pin to it.
    const bundle = await duckdb.selectBundle({ mvp: bundles.mvp });

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

    // Load Onager
    const repo = await getExtensionRepository();
    await conn.query(`set custom_extension_repository = '${repo}';`);
    await conn.query(`install onager;`);
    await conn.query(`load onager;`);

    const version = await scalar("select onager_version() as v;", "v");
    const duckdbVersion = await scalar("select version() as v;", "v");
    updateFooterVersions(duckdbVersion, version);
    await conn.query(SAMPLE_EDGES);

    // Populate initial textareas
    edgesInput.value = GRAPH_TEMPLATES.kite;

    setStatus("ready", `Ready. Onager ${version ?? ""} loaded. Krackhardt Kite "edges" table created.`);
    setBusy(false);

    buildDemoCatalog();
    await loadFunctionList();

    // Restore state in priority order: share link, then the previous session,
    // then the first demo so new users see output immediately.
    const shared = parseShareHash();
    if (shared) {
      if (shared.edges) {
        edgesInput.value = shared.edges;
        templateSelect.value = "custom";
        await rebuildEdgesTable(shared.edges);
      }
      if (shared.sql) {
        sqlEl.value = shared.sql;
        updateHighlight();
      }
      await runQuery();
      return;
    }

    const savedEdges = readLocal("onager_edges_text");
    if (savedEdges && savedEdges.trim() && savedEdges.trim() !== GRAPH_TEMPLATES.kite) {
      try {
        await rebuildEdgesTable(savedEdges);
        edgesInput.value = savedEdges;
        templateSelect.value = "custom";
      } catch (e) {
        console.warn("Ignoring saved edges from a previous session:", e);
      }
    }

    const savedSql = readLocal("onager_editor_sql");
    if (savedSql && savedSql.trim()) {
      sqlEl.value = savedSql;
      updateHighlight();
      await runQuery();
    } else {
      selectDemo(0, false);
      await runQuery();
    }
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

// localStorage wrappers tolerant of unavailable storage (private browsing).
function saveLocal(key, value) {
  try {
    localStorage.setItem(key, value);
  } catch (e) {
    // Storage may be unavailable; the value simply is not persisted.
  }
}

function readLocal(key) {
  try {
    return localStorage.getItem(key);
  } catch (e) {
    return null;
  }
}

function saveEditorSql() {
  saveLocal("onager_editor_sql", sqlEl.value);
}

// URL-safe base64 for share links, UTF-8 aware.
function b64urlEncode(s) {
  const bytes = new TextEncoder().encode(s);
  let bin = "";
  for (const b of bytes) bin += String.fromCharCode(b);
  return btoa(bin).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");
}

function b64urlDecode(s) {
  let padded = s.replace(/-/g, "+").replace(/_/g, "/");
  while (padded.length % 4) padded += "=";
  const bin = atob(padded);
  return new TextDecoder().decode(Uint8Array.from(bin, (c) => c.charCodeAt(0)));
}

// Build a link that restores the current editor query and edge list on load.
function buildShareLink() {
  const params = new URLSearchParams();
  if (sqlEl.value.trim()) params.set("q", b64urlEncode(sqlEl.value));
  if (edgesInput.value.trim()) params.set("e", b64urlEncode(edgesInput.value));
  return `${location.origin}${location.pathname}#${params.toString()}`;
}

function parseShareHash() {
  if (!location.hash || location.hash.length < 2) return null;
  try {
    const params = new URLSearchParams(location.hash.slice(1));
    // "q" carries base64 (share button links); "sql" carries percent-encoded
    // plain text so docs links stay human-readable and easy to generate.
    const sql = params.get("q") ? b64urlDecode(params.get("q")) : params.get("sql");
    const edges = params.get("e") ? b64urlDecode(params.get("e")) : null;
    if (!sql && !edges) return null;
    return { sql, edges };
  } catch (e) {
    console.warn("Ignoring malformed share link:", e);
    return null;
  }
}

// Function browser: the list comes from the loaded extension itself, so it stays
// correct as functions are added or renamed.
let onagerFunctions = [];

async function loadFunctionList() {
  try {
    const res = await conn.query(
      "select distinct function_name from duckdb_functions() where function_name like 'onager%' order by function_name;"
    );
    const vec = res.getChildAt(0);
    onagerFunctions = [];
    for (let i = 0; i < res.numRows; i++) {
      const name = readCell(vec, i);
      if (name != null) onagerFunctions.push(String(name));
    }
  } catch (e) {
    console.warn("Failed to list Onager functions:", e);
  }
  if (onagerFunctions.length > 0) {
    functionSearchEl.placeholder = `Search ${onagerFunctions.length} functions...`;
  }
  functionSearchEl.addEventListener("input", renderFunctionList);
  renderFunctionList();
}

function renderFunctionList() {
  const filter = functionSearchEl.value.trim().toLowerCase();
  const names = onagerFunctions.filter((n) => n.includes(filter));
  functionListEl.replaceChildren();
  if (names.length === 0) {
    const empty = document.createElement("div");
    empty.className = "empty-state";
    empty.textContent =
      onagerFunctions.length === 0 ? "Function list unavailable" : "No matching functions";
    functionListEl.appendChild(empty);
    return;
  }
  for (const name of names) {
    const item = document.createElement("button");
    item.type = "button";
    item.className = "function-item";
    item.textContent = name;
    item.title = `Insert ${name} into the editor`;
    item.addEventListener("click", () => insertIntoEditor(name));
    functionListEl.appendChild(item);
  }
}

function insertIntoEditor(text) {
  const start = sqlEl.selectionStart ?? sqlEl.value.length;
  const end = sqlEl.selectionEnd ?? start;
  sqlEl.value = sqlEl.value.slice(0, start) + text + sqlEl.value.slice(end);
  const pos = start + text.length;
  sqlEl.setSelectionRange(pos, pos);
  updateHighlight();
  saveEditorSql();
  sqlEl.focus();
}

function editDistance(a, b) {
  const m = a.length;
  const n = b.length;
  let prev = Array.from({ length: n + 1 }, (_, j) => j);
  for (let i = 1; i <= m; i++) {
    const cur = [i];
    for (let j = 1; j <= n; j++) {
      cur[j] = a[i - 1] === b[j - 1] ? prev[j - 1] : 1 + Math.min(prev[j - 1], prev[j], cur[j - 1]);
    }
    prev = cur;
  }
  return prev[n];
}

// Suggest the closest registered function for unknown onager_ names in a failed query.
function functionHint(sql) {
  if (onagerFunctions.length === 0) return null;
  const known = new Set(onagerFunctions);
  const tokens = new Set(sql.toLowerCase().match(/onager_[a-z0-9_]+/g) || []);
  for (const token of tokens) {
    if (known.has(token)) continue;
    let best = null;
    let bestDist = 4; // Suggestions further than three edits away are noise.
    for (const name of onagerFunctions) {
      const d = editDistance(token, name);
      if (d < bestDist) {
        bestDist = d;
        best = name;
      }
    }
    if (best) return `${token} is not an Onager function. Did you mean ${best}?`;
  }
  return null;
}

// CSV upload: reuse the edge text pipeline so the file contents stay inspectable
// and editable in the builder textarea.
function setupCsvUpload() {
  loadCsvBtn.addEventListener("click", () => csvFileInput.click());
  csvFileInput.addEventListener("change", async () => {
    const file = csvFileInput.files && csvFileInput.files[0];
    csvFileInput.value = "";
    if (!file) return;
    try {
      const text = await file.text();
      const pairs = parseEdgePairs(text);
      if (pairs.length === 0) {
        throw new Error("No numeric source, target pairs found in the file.");
      }
      edgesInput.value = pairs.map(([s, d]) => `${s}, ${d}`).join("\n");
      templateSelect.value = "custom";
      const ok = await updateGraphFromInput();
      if (ok) setStatus("ready", `Loaded ${pairs.length} edges from ${file.name}.`);
    } catch (err) {
      console.error("Failed to load CSV:", err);
      setStatus("error", `Failed to load edges from ${file.name}.\n` + String(err));
    }
  });
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

// Read one cell as a number. SQL NULL becomes NaN instead of 0, so NULL node ids,
// distances, and communities are not mistaken for real zero values.
function cellNumber(vector, i) {
  const v = readCell(vector, i);
  return v == null ? NaN : Number(v);
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

// Cap displayed cell text so one long value cannot stretch the table rows.
// The full value stays available in the tooltip and in exports.
const MAX_CELL_CHARS = 200;

function formatDuration(ms) {
  return ms < 1000 ? `${Math.max(1, Math.round(ms))} ms` : `${(ms / 1000).toFixed(2)} s`;
}

// Render an Arrow Table by reading column vectors directly.
function renderTable(res, elapsedMs) {
  const fields = res.schema.fields.map((f) => f.name);
  const vectors = fields.map((_, c) => res.getChildAt(c));
  const nrows = res.numRows;
  const timing = elapsedMs == null ? "" : ` Query took ${formatDuration(elapsedMs)}.`;

  if (nrows === 0) {
    resultEl.innerHTML = `<div class="empty-state"><p>No rows returned</p></div>`;
    resultMetaEl.textContent = `0 rows, ${fields.length} columns.${timing}`;
    resultsActions.style.display = "none";
    return;
  }

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
  const shownRows = Math.min(nrows, MAX_TABLE_ROWS);
  for (let i = 0; i < shownRows; i++) {
    const tr = tbody.insertRow();
    for (let c = 0; c < fields.length; c++) {
      const td = tr.insertCell();
      const value = vectors[c] ? readCell(vectors[c], i) : undefined;
      const text = formatValue(value);
      if (text.length > MAX_CELL_CHARS) {
        td.textContent = text.slice(0, MAX_CELL_CHARS) + "…";
        td.title = text;
      } else {
        td.textContent = text;
      }
      if (isNumeric(value)) td.className = "num";
    }
  }

  resultEl.replaceChildren(table);
  const columnsText = `${fields.length} column${fields.length === 1 ? "" : "s"}`;
  resultMetaEl.textContent =
    (nrows > MAX_TABLE_ROWS
      ? `Showing first ${MAX_TABLE_ROWS} of ${nrows} rows, ${columnsText}. Exports include all rows.`
      : `${nrows} row${nrows === 1 ? "" : "s"}, ${columnsText}.`) + timing;
  resultsActions.style.display = "flex";
}

async function runQuery() {
  if (!conn) return false;
  const sql = sqlEl.value.trim();
  if (!sql) return false;
  setBusy(true);
  resultMetaEl.textContent = "Running…";
  try {
    const started = performance.now();
    const res = await conn.query(sql);
    const elapsedMs = performance.now() - started;
    lastResult = res;
    renderTable(res, elapsedMs);
    setStatus("ready", "Query finished.");
    addToHistory(sql);

    // Automatically visualize the results
    await loadGraphDataAndVisualize();
    return true;
  } catch (err) {
    console.error("Onager playground query failed:", err);
    resultEl.replaceChildren();
    resultMetaEl.textContent = "";
    resultsActions.style.display = "none";
    const detail = await scalar("select onager_last_error() as onager_last_error;", "onager_last_error");
    const hint = functionHint(sql);
    setStatus(
      "error",
      "Query failed.\n" +
        formatError(err) +
        (detail ? `\n\nOnager: ${detail}` : "") +
        (hint ? `\n\nHint: ${hint}` : "")
    );
    return false;
  } finally {
    setBusy(false);
  }
}

let activeCategory = 0;

function selectDemo(index, run = true) {
  const category = DEMO_CATEGORIES[activeCategory];
  const demo = category && category.demos[index];
  if (!demo) return;
  sqlEl.value = demo.sql;
  updateHighlight();
  saveEditorSql();
  demoDescEl.textContent = demo.desc;
  for (const btn of demoButtonsEl.children) {
    btn.classList.toggle("active", Number(btn.dataset.index) === index);
  }
  if (run) runQuery();
}

function renderDemoButtons() {
  const category = DEMO_CATEGORIES[activeCategory];
  demoButtonsEl.innerHTML = "";
  category.demos.forEach((demo, i) => {
    const btn = document.createElement("button");
    btn.className = "btn";
    btn.textContent = demo.label;
    btn.dataset.index = String(i);
    btn.addEventListener("click", () => selectDemo(i));
    demoButtonsEl.appendChild(btn);
  });
  demoDescEl.textContent = "";
  demoDocsLinkEl.href = category.docs;
  demoDocsLinkEl.textContent = `Learn more: ${category.label} guide`;
  demoDocsLinkEl.style.display = "inline-block";
}

function buildDemoCatalog() {
  demoCategoryEl.innerHTML = "";
  DEMO_CATEGORIES.forEach((category, i) => {
    const opt = document.createElement("option");
    opt.value = String(i);
    opt.textContent = category.label;
    demoCategoryEl.appendChild(opt);
  });
  demoCategoryEl.addEventListener("change", () => {
    activeCategory = Number(demoCategoryEl.value);
    renderDemoButtons();
  });
  renderDemoButtons();
}

// Parse edge text (one "source, target" pair per line, separated by commas,
// semicolons, or whitespace) into numeric pairs. Non-numeric lines are skipped,
// which also drops CSV header rows.
function parseEdgePairs(text) {
  const pairs = [];
  for (const line of text.split("\n")) {
    const parts = line.split(/[\s,;\t]+/).map(p => p.trim()).filter(Boolean);
    if (parts.length >= 2) {
      const src = parseInt(parts[0], 10);
      const dst = parseInt(parts[1], 10);
      if (!isNaN(src) && !isNaN(dst)) pairs.push([src, dst]);
    }
  }
  return pairs;
}

// Replace the edges table from edge text and persist the text for the next visit.
async function rebuildEdgesTable(text) {
  const pairs = parseEdgePairs(text);
  if (pairs.length === 0) {
    throw new Error("No valid edges found in input. Format should be: source, target");
  }
  const values = pairs.map(([s, d]) => `(${s}::bigint, ${d}::bigint)`);
  await conn.query(
    `create or replace table edges as\nselect * from (values\n  ${values.join(",\n  ")}\n) t(src, dst);`
  );
  saveLocal("onager_edges_text", text);
  return pairs.length;
}

// Recreate sample graph table from custom text input
async function updateGraphFromInput() {
  if (!conn) return;
  const text = edgesInput.value.trim();
  if (!text) return;

  setBusy(true);
  setStatus("loading", "Updating sample graph edges...");

  try {
    const edgeCount = await rebuildEdgesTable(text);

    // If the editor query does not read from the edges table (for example a graph
    // generator call), rerunning it would visualize its own result and hide the
    // updated graph. Point the editor at the new table instead.
    if (!/\bedges\b/i.test(sqlEl.value)) {
      sqlEl.value = "select * from edges;";
      updateHighlight();
    }

    // Rerun the editor query so the table and visualization reflect the new graph.
    const ok = await runQuery();
    if (ok) setStatus("ready", `Sample graph updated with ${edgeCount} edges.`);
    return ok;
  } catch (err) {
    console.error("Failed to update graph:", err);
    setStatus("error", "Failed to update graph.\n" + String(err));
    return false;
  } finally {
    setBusy(false);
  }
}

// Fetch all elements in the default edges table
async function getEdgesTable() {
  try {
    const res = await conn.query("select src, dst from edges;");
    const fields = res.schema.fields.map(f => f.name.toLowerCase());
    const srcIdx = fields.indexOf("src");
    const dstIdx = fields.indexOf("dst");

    const links = [];
    const nodeSet = new Set();
    const nrows = res.numRows;

    const srcVector = res.getChildAt(srcIdx >= 0 ? srcIdx : 0);
    const dstVector = res.getChildAt(dstIdx >= 0 ? dstIdx : 1);

    for (let i = 0; i < nrows; i++) {
      const src = cellNumber(srcVector, i);
      const dst = cellNumber(dstVector, i);
      if (!isNaN(src) && !isNaN(dst)) {
        links.push({ source: src, target: dst });
        nodeSet.add(src);
        nodeSet.add(dst);
      }
    }

    const nodes = Array.from(nodeSet).map(id => ({ id }));
    return { nodes, links };
  } catch (e) {
    console.warn("Failed to query edges table:", e);
    return { nodes: [], links: [] };
  }
}

// Load and visualize the graph (checks query format to determine layout properties)
async function loadGraphDataAndVisualize() {
  if (!conn) return;

  const baseGraph = await getEdgesTable();
  let displayNodes = baseGraph.nodes;
  let displayLinks = baseGraph.links;
  let legendInfo = null;

  if (lastResult && lastResult.numRows > 0) {
    const fields = lastResult.schema.fields.map(f => f.name.toLowerCase());
    const nrows = lastResult.numRows;

    // Case A: Query output contains an edge list itself (e.g. random graph generators)
    const srcName = fields.find(f => ["src", "source", "from", "source_id"].includes(f));
    const dstName = fields.find(f => ["dst", "target", "to", "target_id"].includes(f));

    if (srcName && dstName) {
      const srcColIdx = lastResult.schema.fields.findIndex(f => f.name.toLowerCase() === srcName);
      const dstColIdx = lastResult.schema.fields.findIndex(f => f.name.toLowerCase() === dstName);
      const srcVec = lastResult.getChildAt(srcColIdx);
      const dstVec = lastResult.getChildAt(dstColIdx);

      const links = [];
      const nodeSet = new Set();
      for (let i = 0; i < nrows; i++) {
        const s = cellNumber(srcVec, i);
        const d = cellNumber(dstVec, i);
        if (!isNaN(s) && !isNaN(d)) {
          links.push({ source: s, target: d });
          nodeSet.add(s);
          nodeSet.add(d);
        }
      }
      displayNodes = Array.from(nodeSet).map(id => ({ id }));
      displayLinks = links;
    }
    // Case B: Query output has node attributes (e.g. Centrality PageRank, Louvain communities)
    else {
      const nodeColName = fields.find(f => ["node_id", "node", "vertex", "id"].includes(f));
      if (nodeColName) {
        const nodeColIdx = lastResult.schema.fields.findIndex(f => f.name.toLowerCase() === nodeColName);
        const nodeVec = lastResult.getChildAt(nodeColIdx);

        const communityColName = fields.find(f => ["community", "partition", "cluster", "group"].includes(f));
        // Cover the score column names Onager functions actually return, so node
        // sizing works for every centrality variant, not only PageRank.
        const rankColName = fields.find(f => [
          "rank", "importance", "centrality", "score", "value",
          "betweenness", "closeness", "eigenvector", "katz", "harmonic",
          "triangles", "coefficient",
        ].includes(f));
        const distColName = fields.find(f => ["distance", "cost", "weight"].includes(f));

        const commIdx = communityColName ? lastResult.schema.fields.findIndex(f => f.name.toLowerCase() === communityColName) : -1;
        const rankIdx = rankColName ? lastResult.schema.fields.findIndex(f => f.name.toLowerCase() === rankColName) : -1;
        const distIdx = distColName ? lastResult.schema.fields.findIndex(f => f.name.toLowerCase() === distColName) : -1;

        const commVec = commIdx >= 0 ? lastResult.getChildAt(commIdx) : null;
        const rankVec = rankIdx >= 0 ? lastResult.getChildAt(rankIdx) : null;
        const distVec = distIdx >= 0 ? lastResult.getChildAt(distIdx) : null;

        const nodeProps = new Map();
        let minRank = Infinity, maxRank = -Infinity;
        const communities = new Set();

        for (let i = 0; i < nrows; i++) {
          const nid = cellNumber(nodeVec, i);
          if (isNaN(nid)) continue;

          const props = {};
          if (commVec) {
            const val = cellNumber(commVec, i);
            props.community = isNaN(val) ? 0 : val;
            communities.add(props.community);
          }
          if (rankVec) {
            const val = cellNumber(rankVec, i);
            props.rank = isNaN(val) ? 0 : val;
            if (props.rank < minRank) minRank = props.rank;
            if (props.rank > maxRank) maxRank = props.rank;
          }
          if (distVec) {
            // NULL distance means unreachable, so map it to Infinity.
            const val = cellNumber(distVec, i);
            props.distance = isNaN(val) ? Infinity : val;
          }
          nodeProps.set(nid, props);
        }

        // Map computed properties to base nodes
        displayNodes.forEach(node => {
          const props = nodeProps.get(node.id);
          if (props) {
            node.community = props.community;
            node.rank = props.rank;
            node.distance = props.distance;
          }
        });

        // Color coding for Louvain communities
        if (commVec) {
          const comms = Array.from(communities).sort((a, b) => a - b);
          const palette = ["#7e56c2", "#36a2eb", "#ff6384", "#4bc0c0", "#ffcd56", "#ff9f40", "#9966ff", "#4caf50", "#ff5722", "#00bcd4"];
          legendInfo = {
            title: "Louvain Communities",
            items: comms.map((c, idx) => ({
              label: `Community ${c}`,
              color: palette[idx % palette.length]
            }))
          };
          displayNodes.forEach(node => {
            if (node.community !== undefined) {
              const idx = comms.indexOf(node.community);
              node.color = palette[idx % palette.length];
            }
          });
        }

        // Size scaling for Centrality PageRank
        if (rankVec && minRank !== Infinity && maxRank !== Infinity) {
          const range = maxRank - minRank || 1;
          displayNodes.forEach(node => {
            if (node.rank !== undefined) {
              // Scale size radius (r) between 13 and 28
              node.r = 13 + ((node.rank - minRank) / range) * 15;
            }
          });
        }

        // Distance coloring for Dijkstra Shortest Paths
        if (distVec) {
          legendInfo = {
            title: "Path Distance (from Node 1)",
            items: [
              { label: "Source node", color: "var(--accent)" },
              { label: "Connected", color: "var(--primary)" },
              { label: "Unreachable (∞)", color: "var(--fg-soft)" }
            ]
          };
          displayNodes.forEach(node => {
            if (node.distance === 0) {
              node.color = "var(--accent)";
              node.highlight = true;
            } else if (node.distance === Infinity || node.distance > 999999) {
              node.color = "var(--fg-soft)";
              node.opacity = 0.4;
              // White labels are unreadable on the faded circle in light theme.
              node.labelColor = "var(--fg)";
            } else {
              node.color = "var(--primary)";
            }
          });
        }
      }
    }
  }

  // Ensure default node styles are populated
  displayNodes.forEach(node => {
    if (!node.r) node.r = 15;
    if (!node.color) node.color = "var(--graph-node)";
  });

  const legendEl = document.getElementById("graph-legend");
  const messageEl = document.getElementById("graph-message");

  // Skip layout for very large graphs. The O(n^2) simulation would freeze the page.
  if (displayNodes.length > MAX_GRAPH_NODES) {
    if (simulation) cancelAnimationFrame(simulation);
    graphData = { nodes: [], links: [] };
    const container = svgEl.querySelector("g.graph-elements");
    if (container) container.innerHTML = "";
    legendEl.style.display = "none";
    messageEl.textContent = `Graph has ${displayNodes.length} nodes, which is above the visualization limit of ${MAX_GRAPH_NODES}. Use the table view instead.`;
    messageEl.style.display = "flex";
    return;
  }
  messageEl.style.display = "none";

  // Render Legend
  if (legendInfo && legendInfo.items.length > 0) {
    legendEl.style.display = "flex";
    legendEl.innerHTML = `<strong>${legendInfo.title}</strong>` +
      legendInfo.items.map(item => `
        <div class="legend-item">
          <span class="legend-color" style="background: ${item.color}"></span>
          <span>${item.label}</span>
        </div>
      `).join("");
  } else {
    legendEl.style.display = "none";
  }

  // Settle nodes on canvas
  runGraphSimulation(displayNodes, displayLinks);
}

// Spring embedder / Force-directed layout algorithm
function runGraphSimulation(nodes, links) {
  if (simulation) {
    cancelAnimationFrame(simulation);
  }

  const width = 600;
  const height = 400;

  const nodeMap = new Map();
  nodes.forEach((node, i) => {
    // Preserve layout coordinates if node already has them
    const existing = graphData.nodes.find(n => n.id === node.id);
    if (existing && existing.x !== undefined) {
      node.x = existing.x;
      node.y = existing.y;
    } else {
      const angle = (i / nodes.length) * 2 * Math.PI;
      node.x = width / 2 + 120 * Math.cos(angle) + (Math.random() - 0.5) * 15;
      node.y = height / 2 + 120 * Math.sin(angle) + (Math.random() - 0.5) * 15;
    }
    node.vx = 0;
    node.vy = 0;
    nodeMap.set(node.id, node);
  });

  const mappedLinks = links.map(link => {
    return {
      source: nodeMap.get(link.source),
      target: nodeMap.get(link.target)
    };
  }).filter(link => link.source && link.target);

  graphData = { nodes, links: mappedLinks };

  ticks = 0;
  const maxTicks = 150;

  function step() {
    const k = 0.06; // Spring constant
    const repStrength = 1600; // Repulsion constant
    const gravity = 0.02;

    // 1. Repulsion between all node pairs
    for (let i = 0; i < nodes.length; i++) {
      const n1 = nodes[i];
      for (let j = i + 1; j < nodes.length; j++) {
        const n2 = nodes[j];
        const dx = n2.x - n1.x || 0.1;
        const dy = n2.y - n1.y || 0.1;
        const distSq = dx * dx + dy * dy;
        const dist = Math.sqrt(distSq);

        const force = repStrength / (distSq + 100);
        const fx = (dx / dist) * force;
        const fy = (dy / dist) * force;

        if (n1.fx == null) { n1.vx -= fx; n1.vy -= fy; }
        if (n2.fx == null) { n2.vx += fx; n2.vy += fy; }
      }
    }

    // 2. Attraction along connected links
    for (const link of mappedLinks) {
      const n1 = link.source;
      const n2 = link.target;
      const dx = n2.x - n1.x || 0.1;
      const dy = n2.y - n1.y || 0.1;
      const dist = Math.sqrt(dx * dx + dy * dy);
      const restLen = 80;

      const force = (dist - restLen) * k;
      const fx = (dx / dist) * force;
      const fy = (dy / dist) * force;

      if (n1.fx == null) { n1.vx += fx; n1.vy += fy; }
      if (n2.fx == null) { n2.vx -= fx; n2.vy -= fy; }
    }

    // 3. Gravity center force
    for (const n of nodes) {
      if (n.fx != null) continue;
      n.vx += (width / 2 - n.x) * gravity;
      n.vy += (height / 2 - n.y) * gravity;
    }

    // 4. Position updates & bounds constraints
    const damping = 0.85;
    for (const n of nodes) {
      if (n.fx != null) {
        n.x = n.fx;
        n.y = n.fy;
        n.vx = 0;
        n.vy = 0;
      } else {
        n.vx *= damping;
        n.vy *= damping;
        n.x += n.vx;
        n.y += n.vy;
      }

      n.x = Math.max(25, Math.min(width - 25, n.x));
      n.y = Math.max(25, Math.min(height - 25, n.y));
    }

    drawSvgGraph();

    ticks++;
    if (ticks < maxTicks) {
      simulation = requestAnimationFrame(step);
    }
  }

  step();
}

function drawSvgGraph() {
  let container = svgEl.querySelector("g.graph-elements");
  if (!container) {
    container = document.createElementNS("http://www.w3.org/2000/svg", "g");
    container.classList.add("graph-elements");
    svgEl.appendChild(container);
  } else {
    container.innerHTML = "";
  }

  // Draw links
  graphData.links.forEach(link => {
    const line = document.createElementNS("http://www.w3.org/2000/svg", "line");
    line.setAttribute("x1", link.source.x);
    line.setAttribute("y1", link.source.y);
    line.setAttribute("x2", link.target.x);
    line.setAttribute("y2", link.target.y);
    line.classList.add("graph-edge-line");
    container.appendChild(line);
  });

  // Draw nodes
  graphData.nodes.forEach(node => {
    const g = document.createElementNS("http://www.w3.org/2000/svg", "g");

    const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    circle.setAttribute("cx", node.x);
    circle.setAttribute("cy", node.y);
    circle.setAttribute("r", node.r);
    circle.setAttribute("data-id", node.id);
    circle.classList.add("graph-node-circle");
    circle.style.fill = node.color;

    if (node.highlight) circle.classList.add("highlight");
    if (node.opacity) circle.style.opacity = node.opacity;

    g.appendChild(circle);

    // Node label ID
    const text = document.createElementNS("http://www.w3.org/2000/svg", "text");
    text.setAttribute("x", node.x);
    text.setAttribute("y", node.y);
    text.classList.add("graph-node-label");
    text.textContent = node.id;
    text.style.fill = node.labelColor || "#ffffff";
    g.appendChild(text);

    // Visual helper for property value (PageRank score or Dijkstra distance)
    let propVal = "";
    if (node.rank !== undefined) propVal = node.rank.toFixed(3);
    else if (node.distance !== undefined) {
      propVal = node.distance === Infinity || node.distance > 999999 ? "∞" : `d:${node.distance}`;
    }

    if (propVal) {
      const valLabel = document.createElementNS("http://www.w3.org/2000/svg", "text");
      valLabel.setAttribute("x", node.x);
      valLabel.setAttribute("y", node.y + node.r + 12);
      valLabel.classList.add("graph-node-val");
      valLabel.textContent = propVal;
      g.appendChild(valLabel);
    }

    container.appendChild(g);
  });
}

function fitGraphView() {
  if (graphData.nodes.length === 0) return;

  let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
  graphData.nodes.forEach(n => {
    if (n.x < minX) minX = n.x;
    if (n.x > maxX) maxX = n.x;
    if (n.y < minY) minY = n.y;
    if (n.y > maxY) maxY = n.y;
  });

  const w = maxX - minX;
  const h = maxY - minY;
  const cx = minX + w / 2;
  const cy = minY + h / 2;

  const viewW = Math.max(w * 1.5, 200);
  const viewH = Math.max(h * 1.5, 150);

  const aspect = 600 / 400;
  let finalW = viewW;
  let finalH = viewH;

  if (viewW / viewH > aspect) {
    finalH = viewW / aspect;
  } else {
    finalW = viewH * aspect;
  }

  const x = cx - finalW / 2;
  const y = cy - finalH / 2;
  svgEl.setAttribute("viewBox", `${x} ${y} ${finalW} ${finalH}`);
}

// Drag functionality (pointer events cover mouse, touch, and pen input)
let dragOffset = { x: 0, y: 0 };

// Convert screen coordinates to SVG coordinates based on the current viewBox.
function svgPointFromEvent(e) {
  const rect = svgEl.getBoundingClientRect();
  const viewBox = svgEl.getAttribute("viewBox").split(" ").map(Number);
  return {
    x: viewBox[0] + ((e.clientX - rect.left) * viewBox[2]) / rect.width,
    y: viewBox[1] + ((e.clientY - rect.top) * viewBox[3]) / rect.height,
  };
}

function setupDragBehavior() {
  svgEl.addEventListener("pointerdown", (e) => {
    if (!e.target.classList.contains("graph-node-circle")) return;
    const nodeId = e.target.dataset.id;
    draggedNode = graphData.nodes.find(n => String(n.id) === String(nodeId));
    if (!draggedNode) return;

    // Keep the grab point fixed relative to the node center so it does not jump.
    const p = svgPointFromEvent(e);
    dragOffset = { x: draggedNode.x - p.x, y: draggedNode.y - p.y };
    draggedNode.fx = draggedNode.x;
    draggedNode.fy = draggedNode.y;
    svgEl.setPointerCapture(e.pointerId);
    e.preventDefault();

    // Revive force animation
    ticks = 0;
    runGraphSimulation(graphData.nodes, graphData.links.map(l => ({ source: l.source.id, target: l.target.id })));
  });

  svgEl.addEventListener("pointermove", (e) => {
    if (!draggedNode) return;
    const p = svgPointFromEvent(e);
    draggedNode.fx = p.x + dragOffset.x;
    draggedNode.fy = p.y + dragOffset.y;

    // Revive ticks to force recalculation during drag
    ticks = 0;
  });

  const endDrag = (e) => {
    if (!draggedNode) return;
    draggedNode.fx = null;
    draggedNode.fy = null;
    draggedNode = null;
    if (svgEl.hasPointerCapture(e.pointerId)) {
      svgEl.releasePointerCapture(e.pointerId);
    }
  };
  svgEl.addEventListener("pointerup", endDrag);
  svgEl.addEventListener("pointercancel", endDrag);
}

// Theme handling
function initTheme() {
  const themeToggle = document.getElementById("theme-toggle");
  let theme = null;
  try {
    theme = localStorage.getItem("onager_theme");
  } catch (e) {
    // Storage may be unavailable (private browsing); fall back to the system preference.
  }
  if (!theme) {
    theme = window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
  }
  setTheme(theme);
  themeToggle.addEventListener("click", () => {
    const curTheme = document.body.classList.contains("dark-theme") ? "dark" : "light";
    setTheme(curTheme === "dark" ? "light" : "dark");
  });
}

function setTheme(theme) {
  document.body.classList.toggle("dark-theme", theme === "dark");
  try {
    localStorage.setItem("onager_theme", theme);
  } catch (e) {
    // Storage may be unavailable; the theme still applies for this session.
  }
}

// Tabs toggling
function setupTabs() {
  tabTable.addEventListener("click", () => setTab("table"));
  tabGraph.addEventListener("click", () => setTab("graph"));
}

function setTab(tab) {
  activeTab = tab;
  const isTable = tab === "table";
  tabTable.classList.toggle("active", isTable);
  tabGraph.classList.toggle("active", !isTable);
  tabTable.setAttribute("aria-selected", isTable ? "true" : "false");
  tabGraph.setAttribute("aria-selected", isTable ? "false" : "true");
  tableContainer.classList.toggle("active", isTable);
  graphContainer.classList.toggle("active", !isTable);
  if (!isTable) fitGraphView();
}

// Exporters
function setupExporters() {
  exportCsvBtn.addEventListener("click", exportCSV);
  exportJsonBtn.addEventListener("click", exportJSON);
  fitGraphBtn.addEventListener("click", fitGraphView);
}

function exportCSV() {
  if (!lastResult) return;
  const fields = lastResult.schema.fields.map(f => f.name);
  const vectors = fields.map((_, c) => lastResult.getChildAt(c));
  const nrows = lastResult.numRows;

  let csv = fields.map(f => `"${f.replace(/"/g, '""')}"`).join(",") + "\n";
  for (let i = 0; i < nrows; i++) {
    const row = [];
    for (let c = 0; c < fields.length; c++) {
      const val = vectors[c] ? readCell(vectors[c], i) : "";
      const formatted = val === null || val === undefined ? "" : formatValue(val);
      if (typeof formatted === "string" && (formatted.includes(",") || formatted.includes('"') || formatted.includes("\n"))) {
        row.push(`"${formatted.replace(/"/g, '""')}"`);
      } else {
        row.push(formatted);
      }
    }
    csv += row.join(",") + "\n";
  }
  downloadFile(csv, "query_results.csv", "text/csv");
}

function exportJSON() {
  if (!lastResult) return;
  const fields = lastResult.schema.fields.map(f => f.name);
  const vectors = fields.map((_, c) => lastResult.getChildAt(c));
  const nrows = lastResult.numRows;

  const rows = [];
  for (let i = 0; i < nrows; i++) {
    const obj = {};
    for (let c = 0; c < fields.length; c++) {
      const val = vectors[c] ? readCell(vectors[c], i) : null;
      obj[fields[c]] = val;
    }
    rows.push(obj);
  }
  downloadFile(JSON.stringify(rows, null, 2), "query_results.json", "application/json");
}

function downloadFile(content, filename, contentType) {
  const blob = new Blob([content], { type: contentType });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}

// Run a generator preset, fill the edges input with the generated edge list, and
// rebuild the sample graph from it so the user can inspect and edit the edges.
async function loadGeneratorTemplate(key) {
  if (!conn) return;
  const gen = GENERATOR_TEMPLATES[key];

  setBusy(true);
  setStatus("loading", `Generating ${gen.label} graph...`);

  try {
    const res = await conn.query(gen.sql);
    const srcVec = res.getChildAt(0);
    const dstVec = res.getChildAt(1);
    const lines = [];
    for (let i = 0; i < res.numRows; i++) {
      const s = cellNumber(srcVec, i);
      const d = cellNumber(dstVec, i);
      if (!isNaN(s) && !isNaN(d)) lines.push(`${s}, ${d}`);
    }
    if (lines.length === 0) {
      throw new Error("The generator returned no edges.");
    }
    edgesInput.value = lines.join("\n");
    const ok = await updateGraphFromInput();
    if (ok) {
      setStatus("ready", `Generated ${gen.label} graph with ${lines.length} edges.`);
    }
  } catch (err) {
    console.error("Failed to generate graph:", err);
    setStatus("error", "Failed to generate graph.\n" + String(err));
  } finally {
    setBusy(false);
  }
}

// Graph Templates
function setupGraphTemplateListeners() {
  templateSelect.addEventListener("change", () => {
    // Button enablement is owned by setBusy, so only load the template text here.
    const key = templateSelect.value;
    if (GRAPH_TEMPLATES[key]) {
      edgesInput.value = GRAPH_TEMPLATES[key];
      updateGraphFromInput();
    } else if (GENERATOR_TEMPLATES[key]) {
      loadGeneratorTemplate(key);
    }
  });

  updateGraphBtn.addEventListener("click", updateGraphFromInput);
}

// Query History
function addToHistory(sql) {
  sql = sql.trim();
  if (!sql) return;
  queryHistory = queryHistory.filter(item => item !== sql);
  queryHistory.unshift(sql);
  queryHistory = queryHistory.slice(0, MAX_HISTORY_ITEMS);

  try {
    localStorage.setItem("onager_query_history", JSON.stringify(queryHistory));
  } catch (e) {
    // Storage may be unavailable; history stays in memory for this session.
  }
  renderHistory();
}

function loadHistory() {
  try {
    const saved = localStorage.getItem("onager_query_history");
    if (saved) {
      const parsed = JSON.parse(saved);
      if (Array.isArray(parsed)) {
        // Trim in case a previously saved history is longer than the current cap.
        queryHistory = parsed.filter(item => typeof item === "string").slice(0, MAX_HISTORY_ITEMS);
      }
    }
  } catch (e) {
    // ignore
  }
  renderHistory();
}

function renderHistory() {
  const historyList = document.getElementById("query-history");
  historyList.replaceChildren();

  if (queryHistory.length === 0) {
    const empty = document.createElement("div");
    empty.className = "empty-state";
    empty.textContent = "No queries run yet";
    historyList.appendChild(empty);
    return;
  }

  // Build items with textContent so SQL containing HTML characters renders literally.
  for (const sql of queryHistory) {
    const item = document.createElement("button");
    item.type = "button";
    item.className = "history-item";
    item.title = sql;
    const compact = sql.replace(/\s+/g, " ");
    item.textContent = compact.length > 45 ? `${compact.substring(0, 45)}...` : compact;
    item.addEventListener("click", () => {
      sqlEl.value = sql;
      updateHighlight();
      sqlEl.focus();
    });
    historyList.appendChild(item);
  }
}

// Event Listeners
runBtn.addEventListener("click", runQuery);
clearSqlBtn.addEventListener("click", () => {
  sqlEl.value = "";
  updateHighlight();
  saveEditorSql();
  sqlEl.focus();
});
shareBtn.addEventListener("click", async () => {
  const url = buildShareLink();
  // Reflect the link in the address bar so it can also be copied from there.
  history.replaceState(null, "", url);
  try {
    await navigator.clipboard.writeText(url);
    setStatus("ready", "Share link copied to clipboard.");
  } catch (e) {
    setStatus("ready", "Share link placed in the address bar. Copy it from there.");
  }
});
resetBtn.addEventListener("click", async () => {
  setBusy(true);
  try {
    await conn.query(SAMPLE_EDGES);
    edgesInput.value = GRAPH_TEMPLATES.kite;
    templateSelect.value = "kite";
    saveLocal("onager_edges_text", GRAPH_TEMPLATES.kite);
    // Same guard as updateGraphFromInput: a query that does not read from the
    // edges table would visualize its own result instead of the reset graph.
    if (!/\bedges\b/i.test(sqlEl.value)) {
      sqlEl.value = "select * from edges;";
      updateHighlight();
    }
    const ok = await runQuery();
    if (ok) setStatus("ready", "Sample edges table reset to default Krackhardt Kite.");
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

window.addEventListener("error", (e) => {
  console.error("Onager playground uncaught error:", e.error || e.message);
});
window.addEventListener("unhandledrejection", (e) => {
  console.error("Onager playground unhandled rejection:", e.reason);
});

function setupHighlighting() {
  sqlEl.addEventListener("scroll", () => {
    sqlBackdropEl.scrollTop = sqlEl.scrollTop;
    sqlBackdropEl.scrollLeft = sqlEl.scrollLeft;
  });
  sqlEl.addEventListener("input", () => {
    updateHighlight();
    saveEditorSql();
    sqlBackdropEl.scrollTop = sqlEl.scrollTop;
    sqlBackdropEl.scrollLeft = sqlEl.scrollLeft;
  });
}

function updateHighlight() {
  let text = sqlEl.value;
  if (text.endsWith("\n")) {
    text += " ";
  }
  sqlHighlightEl.textContent = text;
  if (window.Prism) {
    window.Prism.highlightElement(sqlHighlightEl);
  }
}

init();
