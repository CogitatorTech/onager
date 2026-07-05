import subprocess
import json
import os
import pytest
import networkx as nx

DUCKDB_PATH = "./build/release/duckdb"
EXTENSION_PATH = "build/release/extension/onager/onager.duckdb_extension"

def run_query(sql):
    cmd = [DUCKDB_PATH, "-light-mode", "-json", "-c", f"LOAD '{EXTENSION_PATH}'; {sql}"]
    res = subprocess.run(cmd, capture_output=True, text=True, check=True)
    if not res.stdout.strip():
        return []
    return json.loads(res.stdout)

def l2_normalize(d):
    norm = sum(v**2 for v in d.values())**0.5
    if norm == 0:
        return d
    return {k: v / norm for k, v in d.items()}

@pytest.fixture(scope="module")
def setup_graphs():
    # A simple undirected graph: 1-2, 2-3, 3-1 (triangle) plus 3-4 (pendant)
    edges = [(1, 2), (2, 3), (3, 1), (3, 4)]
    edge_values = ", ".join(f"({u}::bigint, {v}::bigint)" for u, v in edges)
    sql_edges_setup = f"CREATE TEMPORARY TABLE edges AS SELECT * FROM (VALUES {edge_values}) t(src, dst);"

    # A weighted graph: 1-2 (weight 1.0), 2-3 (2.0), 3-1 (1.5), 3-4 (3.0)
    weighted_edges = [(1, 2, 1.0), (2, 3, 2.0), (3, 1, 1.5), (3, 4, 3.0)]
    w_edge_values = ", ".join(f"({u}::bigint, {v}::bigint, {w}::double)" for u, v, w in weighted_edges)
    sql_weighted_setup = f"CREATE TEMPORARY TABLE weighted_edges AS SELECT * FROM (VALUES {w_edge_values}) t(src, dst, weight);"

    return edges, sql_edges_setup, weighted_edges, sql_weighted_setup

# =============================================================================
# 1. Centrality Tests
# =============================================================================

def test_pagerank_directed(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.DiGraph(edges)
    nx_pagerank = nx.pagerank(G, alpha=0.85, tol=1e-6)
    
    query = f"{sql_edges_setup} SELECT node_id, rank FROM onager_ctr_pagerank((SELECT src, dst FROM edges), directed := true) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_pagerank)
    for row in db_results:
        node = row["node_id"]
        assert abs(row["rank"] - nx_pagerank[node]) < 1e-4

def test_pagerank_undirected(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_pagerank = nx.pagerank(G, alpha=0.85, tol=1e-6)
    
    query = f"{sql_edges_setup} SELECT node_id, rank FROM onager_ctr_pagerank((SELECT src, dst FROM edges), directed := false) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_pagerank)
    for row in db_results:
        node = row["node_id"]
        assert abs(row["rank"] - nx_pagerank[node]) < 1e-4

def test_degree_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.DiGraph(edges)
    n = len(G.nodes)
    nx_in = dict(G.in_degree())
    nx_out = dict(G.out_degree())
    
    query = f"{sql_edges_setup} SELECT node_id, in_degree, out_degree FROM onager_ctr_degree((SELECT src, dst FROM edges), directed := true) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == n
    for row in db_results:
        node = row["node_id"]
        assert abs(row["in_degree"] - nx_in[node]) < 1e-5
        assert abs(row["out_degree"] - nx_out[node]) < 1e-5

def test_betweenness_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_bet = nx.betweenness_centrality(G, normalized=True)
    
    query = f"{sql_edges_setup} SELECT node_id, betweenness FROM onager_ctr_betweenness((SELECT src, dst FROM edges), normalized := true) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_bet)
    for row in db_results:
        node = row["node_id"]
        assert abs(row["betweenness"] - nx_bet[node]) < 1e-4

def test_closeness_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_close = nx.closeness_centrality(G)
    
    query = f"{sql_edges_setup} SELECT node_id, closeness FROM onager_ctr_closeness((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_close)
    for row in db_results:
        node = row["node_id"]
        assert abs(row["closeness"] - nx_close[node]) < 1e-4

def test_eigenvector_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_eig = nx_eigenvector = nx.eigenvector_centrality(G, max_iter=1000, tol=1e-6)
    
    query = f"{sql_edges_setup} SELECT node_id, eigenvector FROM onager_ctr_eigenvector((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_eig)
    
    # L2 normalize both vectors for robust comparison
    db_eig_dict = {row["node_id"]: row["eigenvector"] for row in db_results}
    norm_db = l2_normalize(db_eig_dict)
    norm_nx = l2_normalize(nx_eig)
    
    for node in norm_nx:
        assert abs(norm_db[node] - norm_nx[node]) < 1e-4

def test_katz_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_katz = nx.katz_centrality(G, alpha=0.1, max_iter=1000, tol=1e-6)
    
    query = f"{sql_edges_setup} SELECT node_id, katz FROM onager_ctr_katz((SELECT src, dst FROM edges), alpha := 0.1) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_katz)
    
    db_katz_dict = {row["node_id"]: row["katz"] for row in db_results}
    norm_db = l2_normalize(db_katz_dict)
    norm_nx = l2_normalize(nx_katz)
    
    for node in norm_nx:
        assert abs(norm_db[node] - norm_nx[node]) < 1e-4

def test_harmonic_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_harm = nx.harmonic_centrality(G)
    
    query = f"{sql_edges_setup} SELECT node_id, harmonic FROM onager_ctr_harmonic((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_harm)
    for row in db_results:
        node = row["node_id"]
        assert abs(row["harmonic"] - nx_harm[node]) < 1e-4

def test_voterank(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    # voterank in nx returns a list of nodes
    nx_vote = nx.voterank(G, number_of_nodes=2)
    
    query = f"{sql_edges_setup} SELECT node_id FROM onager_ctr_voterank((SELECT src, dst FROM edges), num_seeds := 2);"
    db_results = run_query(query)
    
    db_vote = [row["node_id"] for row in db_results]
    assert db_vote == nx_vote

# =============================================================================
# 2. Community Detection Tests
# =============================================================================

def test_connected_components(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_components = [set(c) for c in nx.connected_components(G)]
    
    query = f"{sql_edges_setup} SELECT node_id, component FROM onager_cmm_components((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)
    
    db_groups = {}
    for row in db_results:
        node = row["node_id"]
        comp = row["component"]
        db_groups.setdefault(comp, set()).add(node)
    
    db_components = list(db_groups.values())
    
    assert len(db_components) == len(nx_components)
    for db_c in db_components:
        assert any(db_c == nx_c for nx_c in nx_components)

def test_louvain(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    # Louvain partitions the nodes
    query = f"{sql_edges_setup} SELECT node_id, community FROM onager_cmm_louvain((SELECT src, dst FROM edges), seed := 42) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(G.nodes)
    db_comm = {}
    for row in db_results:
        db_comm.setdefault(row["community"], set()).add(row["node_id"])
    
    # Check partition validity
    all_nodes = set().union(*db_comm.values())
    assert all_nodes == set(G.nodes)

def test_label_prop(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    query = f"{sql_edges_setup} SELECT node_id, label FROM onager_cmm_label_prop((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(G.nodes)
    db_labels = {}
    for row in db_results:
        db_labels.setdefault(row["label"], set()).add(row["node_id"])
    
    all_nodes = set().union(*db_labels.values())
    assert all_nodes == set(G.nodes)

# =============================================================================
# 3. Link Prediction Tests
# =============================================================================

def test_jaccard_link(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    # compute all node pairs where u < v
    pairs = [(u, v) for u in G.nodes for v in G.nodes if u < v]
    nx_jaccard = {(u, v): val for u, v, val in nx.jaccard_coefficient(G, pairs)}
    
    query = f"{sql_edges_setup} SELECT node1, node2, coefficient FROM onager_lnk_jaccard((SELECT src, dst FROM edges)) ORDER BY node1, node2;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_jaccard)
    for row in db_results:
        u, v = row["node1"], row["node2"]
        # Ensure canonical ordering
        pair = (min(u, v), max(u, v))
        assert abs(row["coefficient"] - nx_jaccard[pair]) < 1e-4

def test_adamic_adar_link(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    pairs = [(u, v) for u in G.nodes for v in G.nodes if u < v]
    nx_adamic = {(u, v): val for u, v, val in nx.adamic_adar_index(G, pairs)}
    
    query = f"{sql_edges_setup} SELECT node1, node2, score FROM onager_lnk_adamic_adar((SELECT src, dst FROM edges)) ORDER BY node1, node2;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_adamic)
    for row in db_results:
        u, v = row["node1"], row["node2"]
        pair = (min(u, v), max(u, v))
        assert abs(row["score"] - nx_adamic[pair]) < 1e-4

def test_pref_attach_link(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    pairs = [(u, v) for u in G.nodes for v in G.nodes if u < v]
    nx_pref = {(u, v): val for u, v, val in nx.preferential_attachment(G, pairs)}
    
    query = f"{sql_edges_setup} SELECT node1, node2, score FROM onager_lnk_pref_attach((SELECT src, dst FROM edges)) ORDER BY node1, node2;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_pref)
    for row in db_results:
        u, v = row["node1"], row["node2"]
        pair = (min(u, v), max(u, v))
        assert abs(row["score"] - nx_pref[pair]) < 1e-4

def test_resource_alloc_link(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    pairs = [(u, v) for u in G.nodes for v in G.nodes if u < v]
    nx_res = {(u, v): val for u, v, val in nx.resource_allocation_index(G, pairs)}
    
    query = f"{sql_edges_setup} SELECT node1, node2, score FROM onager_lnk_resource_alloc((SELECT src, dst FROM edges)) ORDER BY node1, node2;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_res)
    for row in db_results:
        u, v = row["node1"], row["node2"]
        pair = (min(u, v), max(u, v))
        assert abs(row["score"] - nx_res[pair]) < 1e-4

def test_common_neighbors_link(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    pairs = [(u, v) for u in G.nodes for v in G.nodes if u < v]
    nx_cn = {(u, v): len(list(nx.common_neighbors(G, u, v))) for u, v in pairs}
    
    query = f"{sql_edges_setup} SELECT node1, node2, count FROM onager_lnk_common_neighbors((SELECT src, dst FROM edges)) ORDER BY node1, node2;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_cn)
    for row in db_results:
        u, v = row["node1"], row["node2"]
        pair = (min(u, v), max(u, v))
        assert row["count"] == nx_cn[pair]

# =============================================================================
# 4. Graph Metrics Tests
# =============================================================================

def test_graph_metrics(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    
    nx_density = nx.density(G)
    nx_diameter = nx.diameter(G)
    nx_radius = nx.radius(G)
    nx_avg_clustering = nx.average_clustering(G)
    nx_avg_path = nx.average_shortest_path_length(G)
    nx_transitivity = nx.transitivity(G)
    
    query = f"""
    {sql_edges_setup}
    SELECT 
        (SELECT density FROM onager_mtr_density((SELECT src, dst FROM edges))) as density,
        (SELECT diameter FROM onager_mtr_diameter((SELECT src, dst FROM edges))) as diameter,
        (SELECT radius FROM onager_mtr_radius((SELECT src, dst FROM edges))) as radius,
        (SELECT avg_clustering FROM onager_mtr_avg_clustering((SELECT src, dst FROM edges))) as avg_clustering,
        (SELECT avg_path_length FROM onager_mtr_avg_path_length((SELECT src, dst FROM edges))) as avg_path_length,
        (SELECT transitivity FROM onager_mtr_transitivity((SELECT src, dst FROM edges))) as transitivity;
    """
    db_results = run_query(query)[0]
    
    assert abs(db_results["density"] - nx_density) < 1e-5
    assert db_results["diameter"] == nx_diameter
    assert db_results["radius"] == nx_radius
    assert abs(db_results["avg_clustering"] - nx_avg_clustering) < 1e-5
    assert abs(db_results["avg_path_length"] - nx_avg_path) < 1e-5
    assert abs(db_results["transitivity"] - nx_transitivity) < 1e-5

# =============================================================================
# 5. Traversal and Pathfinding Tests
# =============================================================================

def test_dijkstra(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_dist = nx.single_source_dijkstra_path_length(G, 1)
    
    query = f"{sql_edges_setup} SELECT node_id, distance FROM onager_pth_dijkstra((SELECT src, dst FROM edges), source := 1) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_dist)
    for row in db_results:
        node = row["node_id"]
        assert row["distance"] == nx_dist[node]

def test_bellman_ford(setup_graphs):
    _, _, weighted_edges, sql_weighted_setup = setup_graphs
    # NetworkX graph
    G = nx.Graph()
    for u, v, w in weighted_edges:
        G.add_edge(u, v, weight=w)
    
    nx_dist = nx.single_source_bellman_ford_path_length(G, 1)
    
    query = f"{sql_weighted_setup} SELECT node_id, distance FROM onager_pth_bellman_ford((SELECT src, dst, weight FROM weighted_edges), source := 1) ORDER BY node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_dist)
    for row in db_results:
        node = row["node_id"]
        assert row["distance"] == nx_dist[node]

def test_floyd_warshall(setup_graphs):
    _, _, weighted_edges, sql_weighted_setup = setup_graphs
    G = nx.Graph()
    for u, v, w in weighted_edges:
        G.add_edge(u, v, weight=w)
        
    nx_apsp = nx.floyd_warshall(G)
    
    query = f"{sql_weighted_setup} SELECT src, dst, distance FROM onager_pth_floyd_warshall((SELECT src, dst, weight FROM weighted_edges)) ORDER BY src, dst;"
    db_results = run_query(query)
    
    # We should have all-pairs distance excluding self loops (12 combinations for 4 nodes)
    assert len(db_results) == 12
    for row in db_results:
        u, v = row["src"], row["dst"]
        assert u != v
        assert abs(row["distance"] - nx_apsp[u][v]) < 1e-5

# =============================================================================
# 6. Spanning Tree and Subgraphs
# =============================================================================

def test_kruskal_mst(setup_graphs):
    _, _, weighted_edges, sql_weighted_setup = setup_graphs
    G = nx.Graph()
    for u, v, w in weighted_edges:
        G.add_edge(u, v, weight=w)
        
    nx_mst = nx.minimum_spanning_tree(G, algorithm="kruskal")
    nx_weight = nx_mst.size(weight="weight")
    
    query = f"{sql_weighted_setup} SELECT src, dst, weight FROM onager_mst_kruskal((SELECT src, dst, weight FROM weighted_edges));"
    db_results = run_query(query)
    
    db_weight = sum(row["weight"] for row in db_results)
    assert abs(db_weight - nx_weight) < 1e-4
    assert len(db_results) == len(G.nodes) - 1

def test_prim_mst(setup_graphs):
    _, _, weighted_edges, sql_weighted_setup = setup_graphs
    G = nx.Graph()
    for u, v, w in weighted_edges:
        G.add_edge(u, v, weight=w)
        
    nx_mst = nx.minimum_spanning_tree(G, algorithm="prim")
    nx_weight = nx_mst.size(weight="weight")
    
    query = f"{sql_weighted_setup} SELECT src, dst, weight FROM onager_mst_prim((SELECT src, dst, weight FROM weighted_edges));"
    db_results = run_query(query)
    
    db_weight = sum(row["weight"] for row in db_results)
    assert abs(db_weight - nx_weight) < 1e-4
    assert len(db_results) == len(G.nodes) - 1

def test_ego_graph(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    
    # nx.ego_graph(G, 1, radius=1)
    nx_ego = nx.ego_graph(G, 1, radius=1)
    
    query = f"{sql_edges_setup} SELECT src, dst FROM onager_sub_ego_graph((SELECT src, dst FROM edges), center := 1, radius := 1) ORDER BY src, dst;"
    db_results = run_query(query)
    
    # Compare edge sets (unordered, canonicalized)
    nx_edges = { (min(u, v), max(u, v)) for u, v in nx_ego.edges() }
    db_edges = { (min(row["src"], row["dst"]), max(row["src"], row["dst"])) for row in db_results }
    
    assert db_edges == nx_edges

def test_induced_subgraph(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    
    selected = [1, 2, 3]
    nx_sub = nx.induced_subgraph(G, selected)
    
    # In onager_sub_induced, we pass a subquery that has 3 columns: e.src, e.dst, f.node
    query = f"""
    {sql_edges_setup}
    CREATE TEMPORARY TABLE filter_nodes (node bigint);
    INSERT INTO filter_nodes VALUES (1), (2), (3);
    SELECT src, dst FROM onager_sub_induced((
        SELECT e.src, e.dst, f.node 
        FROM edges e CROSS JOIN filter_nodes f
    )) ORDER BY src, dst;
    """
    db_results = run_query(query)
    
    nx_edges = { (min(u, v), max(u, v)) for u, v in nx_sub.edges() }
    db_edges = { (min(row["src"], row["dst"]), max(row["src"], row["dst"])) for row in db_results }
    
    assert db_edges == nx_edges

# =============================================================================
# 7. Parallel Counterparts Verification
# =============================================================================

def test_parallel_pagerank(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    
    # Verify parallel PageRank returns same results as scalar/sequential counterpart
    query = f"""
    {sql_edges_setup}
    SELECT 
        s.node_id, 
        s.rank as seq_rank, 
        p.rank as par_rank 
    FROM onager_ctr_pagerank((SELECT src, dst FROM edges)) s
    JOIN onager_par_pagerank((SELECT src, dst FROM edges)) p
      ON s.node_id = p.node_id
    ORDER BY s.node_id;
    """
    db_results = run_query(query)
    assert len(db_results) > 0
    for row in db_results:
        assert abs(row["seq_rank"] - row["par_rank"]) < 1e-6

def test_parallel_bfs(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    
    query = f"""
    {sql_edges_setup}
    SELECT count(*) as cnt 
    FROM onager_par_bfs((SELECT src, dst FROM edges), source := 1);
    """
    db_results = run_query(query)[0]
    assert db_results["cnt"] == 4

def test_parallel_components(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    
    query = f"""
    {sql_edges_setup}
    SELECT 
        s.node_id, 
        s.component as seq_comp, 
        p.component as par_comp 
    FROM onager_cmm_components((SELECT src, dst FROM edges)) s
    JOIN onager_par_components((SELECT src, dst FROM edges)) p
      ON s.node_id = p.node_id
    ORDER BY s.node_id;
    """
    db_results = run_query(query)
    assert len(db_results) > 0
    # Both partitionings should have same number of groups
    seq_groups = len(set(row["seq_comp"] for row in db_results))
    par_groups = len(set(row["par_comp"] for row in db_results))
    assert seq_groups == par_groups
