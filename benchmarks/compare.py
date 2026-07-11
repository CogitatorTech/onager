#!/usr/bin/env python3
"""Benchmark comparison running the same graph algorithms through Onager (DuckDB) and NetworkX.
"""

import os
import sys
import time
import json
import subprocess
import statistics
import networkx as nx

# Deterministic LCG to generate a stable synthetic graph
class Lcg:
    def __init__(self, seed: int) -> None:
        self.state = seed & 0xFFFFFFFFFFFFFFFF

    def next(self) -> int:
        self.state = (self.state * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return self.state >> 16

def generate_graph(nodes: int, edges: int) -> list[tuple[int, int]]:
    rng = Lcg(42)
    seen = set()
    out = []
    while len(out) < edges:
        s = rng.next() % nodes
        t = rng.next() % nodes
        if s == t:
            continue
        a, b = (s, t) if s < t else (t, s)
        if (a, b) in seen:
            continue
        seen.add((a, b))
        out.append((a, b))
    return out

DUCKDB_PATH = "build/release/duckdb"
EXTENSION_PATH = "build/release/extension/onager/onager.duckdb_extension"
DB_FILE = "benchmark_temp.db"

def run_query(sql):
    cmd = [DUCKDB_PATH, "-json", DB_FILE]
    sql_with_load = f"load '{EXTENSION_PATH}';\n{sql}"
    res = subprocess.run(cmd, input=sql_with_load, capture_output=True, text=True)
    if res.returncode != 0:
        raise Exception(f"DuckDB Error: {res.stderr}\nOutput: {res.stdout}")
    if not res.stdout.strip():
        return []
    try:
        return json.loads(res.stdout)
    except json.JSONDecodeError:
        # If there are multiple JSON outputs or no JSON output, return raw stdout
        return res.stdout

def run_benchmark():
    num_nodes = 1000
    num_edges = 5000
    reps = 5

    # Clean up any existing temp db
    if os.path.exists(DB_FILE):
        os.remove(DB_FILE)

    print(f"Generating synthetic graph with {num_nodes} nodes and {num_edges} edges...")
    edges = generate_graph(num_nodes, num_edges)

    # Initialize NetworkX graph
    nx_graph = nx.Graph()
    nx_graph.add_edges_from(edges)
    for n in range(num_nodes):
        nx_graph.add_node(n)

    # Initialize DuckDB tables
    setup_sql = "create table edges(src BIGINT, dst BIGINT);\n"
    setup_sql += "insert into edges values " + ", ".join(f"({u}, {v})" for u, v in edges) + ";\n"

    # Define weighted edges for Dijkstra/MST
    weighted_edges = [(u, v, 1.0) for u, v in edges]
    setup_sql += "create table weighted_edges(src BIGINT, dst BIGINT, weight DOUBLE);\n"
    setup_sql += "insert into weighted_edges values " + ", ".join(f"({u}, {v}, {w})" for u, v, w in weighted_edges) + ";\n"
    
    nx_weighted_graph = nx.Graph()
    for u, v, w in weighted_edges:
        nx_weighted_graph.add_edge(u, v, weight=w)
    for n in range(num_nodes):
        nx_weighted_graph.add_node(n)

    run_query(setup_sql)

    print("Running benchmarks (reps = {})...".format(reps))
    print("| Algorithm | Library | Median Time | Status |")
    print("| --- | --- | --- | --- |")

    results = []

    def time_it(func, *args, **kwargs):
        timings = []
        for _ in range(reps):
            start = time.perf_counter()
            func(*args, **kwargs)
            timings.append(time.perf_counter() - start)
        return statistics.median(timings)

    # 1. Connected Components
    def run_nx_cc():
        return [sorted(list(c)) for c in nx.connected_components(nx_graph)]

    def run_onager_cc():
        return run_query("select node_id, component from onager_cmm_components((select src, dst from edges)) order by node_id;")

    nx_cc_res = run_nx_cc()
    onager_cc_res = run_onager_cc()
    
    nx_components = {}
    for comp_id, comp in enumerate(nx_cc_res):
        for node in comp:
            nx_components[node] = comp_id
    onager_components = {int(row["node_id"]): int(row["component"]) for row in onager_cc_res}
    
    cc_valid = True
    for node, comp_id in nx_components.items():
        if node not in onager_components:
            cc_valid = False
            break
    if cc_valid:
        nx_groups = {}
        for node, c_id in nx_components.items():
            nx_groups.setdefault(c_id, []).append(node)
        for nodes in nx_groups.values():
            onager_comp = onager_components[nodes[0]]
            if not all(onager_components[n] == onager_comp for n in nodes):
                cc_valid = False
                break
                
    status = "OK" if cc_valid else "VALIDATION_FAILED"
    t_nx = time_it(run_nx_cc)
    t_onager = time_it(run_onager_cc)
    print(f"| Connected Components | NetworkX | {t_nx*1000:.2f} ms | {status} |")
    print(f"| Connected Components | Onager | {t_onager*1000:.2f} ms | {status} |")
    results.append(("Connected Components", t_nx, t_onager, status))

    # 2. PageRank
    def run_nx_pr():
        return nx.pagerank(nx_graph, alpha=0.85, tol=1e-6)

    def run_onager_pr():
        return run_query("select node_id, rank from onager_ctr_pagerank((select src, dst from edges), damping := 0.85, directed := false) order by node_id;")

    nx_pr_res = run_nx_pr()
    onager_pr_res = run_onager_pr()
    pr_valid = len(nx_pr_res) == len(onager_pr_res)
    if pr_valid:
        for row in onager_pr_res:
            node = int(row["node_id"])
            if abs(float(row["rank"]) - nx_pr_res[node]) > 1e-4:
                pr_valid = False
                break
    status = "OK" if pr_valid else "VALIDATION_FAILED"
    t_nx = time_it(run_nx_pr)
    t_onager = time_it(run_onager_pr)
    print(f"| PageRank | NetworkX | {t_nx*1000:.2f} ms | {status} |")
    print(f"| PageRank | Onager | {t_onager*1000:.2f} ms | {status} |")
    results.append(("PageRank", t_nx, t_onager, status))

    # 3. Degree Centrality
    def run_nx_deg():
        return nx.degree_centrality(nx_graph)

    def run_onager_deg():
        return run_query("select node_id, out_degree from onager_ctr_degree((select src, dst from edges), directed := false) order by node_id;")

    nx_deg_res = run_nx_deg()
    onager_deg_res = run_onager_deg()
    deg_valid = len(nx_deg_res) == len(onager_deg_res)
    if deg_valid:
        n = len(nx_graph.nodes)
        for row in onager_deg_res:
            node = int(row["node_id"])
            norm_deg = float(row["out_degree"]) / (n - 1) if n > 1 else 0.0
            if abs(norm_deg - nx_deg_res[node]) > 1e-5:
                deg_valid = False
                break
    status = "OK" if deg_valid else "VALIDATION_FAILED"
    t_nx = time_it(run_nx_deg)
    t_onager = time_it(run_onager_deg)
    print(f"| Degree Centrality | NetworkX | {t_nx*1000:.2f} ms | {status} |")
    print(f"| Degree Centrality | Onager | {t_onager*1000:.2f} ms | {status} |")
    results.append(("Degree Centrality", t_nx, t_onager, status))

    # 4. Betweenness Centrality
    def run_nx_bet():
        return nx.betweenness_centrality(nx_graph, normalized=True)

    def run_onager_bet():
        return run_query("select node_id, betweenness from onager_ctr_betweenness((select src, dst from edges), normalized := true) order by node_id;")

    nx_bet_res = run_nx_bet()
    onager_bet_res = run_onager_bet()
    bet_valid = len(nx_bet_res) == len(onager_bet_res)
    if bet_valid:
        for row in onager_bet_res:
            node = int(row["node_id"])
            if abs(float(row["betweenness"]) - nx_bet_res[node]) > 1e-4:
                bet_valid = False
                break
    status = "OK" if bet_valid else "VALIDATION_FAILED"
    t_nx = time_it(run_nx_bet)
    t_onager = time_it(run_onager_bet)
    print(f"| Betweenness Centrality | NetworkX | {t_nx*1000:.2f} ms | {status} |")
    print(f"| Betweenness Centrality | Onager | {t_onager*1000:.2f} ms | {status} |")
    results.append(("Betweenness Centrality", t_nx, t_onager, status))

    # 5. Closeness Centrality
    def run_nx_close():
        return nx.closeness_centrality(nx_graph)

    def run_onager_close():
        return run_query("select node_id, closeness from onager_ctr_closeness((select src, dst from edges)) order by node_id;")

    nx_close_res = run_nx_close()
    onager_close_res = run_onager_close()
    close_valid = len(nx_close_res) == len(onager_close_res)
    if close_valid:
        for row in onager_close_res:
            node = int(row["node_id"])
            if abs(float(row["closeness"]) - nx_close_res[node]) > 1e-4:
                close_valid = False
                break
    status = "OK" if close_valid else "VALIDATION_FAILED"
    t_nx = time_it(run_nx_close)
    t_onager = time_it(run_onager_close)
    print(f"| Closeness Centrality | NetworkX | {t_nx*1000:.2f} ms | {status} |")
    print(f"| Closeness Centrality | Onager | {t_onager*1000:.2f} ms | {status} |")
    results.append(("Closeness Centrality", t_nx, t_onager, status))

    # 6. BFS reachability
    start_node = 0
    def run_nx_bfs():
        return list(nx.bfs_edges(nx_graph, source=start_node))

    def run_onager_bfs():
        return run_query(f"select node_id from onager_trv_bfs((select src, dst from edges), source := {start_node}) order by node_id;")

    nx_bfs_res = run_nx_bfs()
    onager_bfs_res = run_onager_bfs()
    nx_reachable = set([start_node])
    for u, v in nx_bfs_res:
        nx_reachable.add(v)
    onager_reachable = set(int(row["node_id"]) for row in onager_bfs_res)
    bfs_valid = nx_reachable == onager_reachable
    status = "OK" if bfs_valid else "VALIDATION_FAILED"
    t_nx = time_it(run_nx_bfs)
    t_onager = time_it(run_onager_bfs)
    print(f"| BFS Reachability | NetworkX | {t_nx*1000:.2f} ms | {status} |")
    print(f"| BFS Reachability | Onager | {t_onager*1000:.2f} ms | {status} |")
    results.append(("BFS Reachability", t_nx, t_onager, status))

    # 7. Kruskal MST weight
    def run_nx_mst():
        return list(nx.minimum_spanning_edges(nx_weighted_graph, algorithm='kruskal', data=True))

    def run_onager_mst():
        return run_query("select src, dst, weight from onager_mst_kruskal((select src, dst, weight from weighted_edges));")

    nx_mst_res = run_nx_mst()
    onager_mst_res = run_onager_mst()
    nx_weight_sum = sum(edge[2]['weight'] for edge in nx_mst_res)
    onager_weight_sum = sum(float(row["weight"]) for row in onager_mst_res)
    mst_valid = abs(nx_weight_sum - onager_weight_sum) < 1e-4
    status = "OK" if mst_valid else "VALIDATION_FAILED"
    t_nx = time_it(run_nx_mst)
    t_onager = time_it(run_onager_mst)
    print(f"| Kruskal MST | NetworkX | {t_nx*1000:.2f} ms | {status} |")
    print(f"| Kruskal MST | Onager | {t_onager*1000:.2f} ms | {status} |")
    results.append(("Kruskal MST", t_nx, t_onager, status))

    # 8. Jaccard Coefficient
    def run_nx_jaccard():
        return list(nx.jaccard_coefficient(nx_graph))

    def run_onager_jaccard():
        return run_query("select node1, node2, coefficient from onager_lnk_jaccard((select src, dst from edges)) order by node1, node2;")

    nx_jac_res = sorted(run_nx_jaccard(), key=lambda x: (x[0], x[1]))
    onager_jac_res = run_onager_jaccard()
    jac_valid = True
    nx_jac_dict = {(u, v): coef for u, v, coef in nx_jac_res}
    for row in onager_jac_res:
        u, v, coef = int(row["node1"]), int(row["node2"]), float(row["coefficient"])
        nx_coef = nx_jac_dict.get((u, v)) or nx_jac_dict.get((v, u))
        if nx_coef is not None and abs(coef - nx_coef) > 1e-4:
            jac_valid = False
            break
    status = "OK" if jac_valid else "VALIDATION_FAILED"
    t_nx = time_it(run_nx_jaccard)
    t_onager = time_it(run_onager_jaccard)
    print(f"| Jaccard Link Prediction | NetworkX | {t_nx*1000:.2f} ms | {status} |")
    print(f"| Jaccard Link Prediction | Onager | {t_onager*1000:.2f} ms | {status} |")
    results.append(("Jaccard Link Prediction", t_nx, t_onager, status))

    # Print summary
    print("\n### Benchmark Summary")
    print("| Algorithm | NetworkX (ms) | Onager (ms) | Speedup (x) |")
    print("| --- | --- | --- | --- |")
    for name, nx_t, onager_t, status in results:
        speedup = nx_t / onager_t if onager_t > 0 else 0
        print(f"| {name} | {nx_t*1000:.2f} ms | {onager_t*1000:.2f} ms | {speedup:.1f}x |")

    # Clean up temp db
    if os.path.exists(DB_FILE):
        os.remove(DB_FILE)

if __name__ == "__main__":
    run_benchmark()
