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

# =============================================================================
# 8. Personalized and Other Centrality
# =============================================================================

def test_personalized_pagerank(setup_graphs):
    edges, _, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_ppr = nx.pagerank(G, alpha=0.85, personalization={1: 0.7, 4: 0.3}, tol=1e-6)

    # Personalization pairs ride along in columns 3 and 4; rows without an
    # entry are padded with NULLs.
    query = """
    CREATE TEMPORARY TABLE pedges AS SELECT * FROM (VALUES
      (1::bigint, 2::bigint, 1::bigint, 0.7::double),
      (2, 3, 4, 0.3),
      (3, 1, NULL, NULL),
      (3, 4, NULL, NULL)
    ) t(src, dst, pnode, pweight);
    SELECT node_id, score FROM onager_ctr_personalized_pagerank((
      SELECT src, dst, pnode, pweight FROM pedges
    )) ORDER BY node_id;
    """
    db_results = run_query(query)

    assert len(db_results) == len(nx_ppr)
    for row in db_results:
        assert abs(row["score"] - nx_ppr[row["node_id"]]) < 1e-4

def test_local_reaching_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)

    # Onager's local reaching centrality counts the nodes reachable within
    # the given distance, including the node itself.
    for distance in (1, 2):
        expected = {
            v: float(len(nx.single_source_shortest_path_length(G, v, cutoff=distance)))
            for v in G
        }
        query = f"{sql_edges_setup} SELECT node_id, centrality FROM onager_ctr_local_reaching((SELECT src, dst FROM edges), distance := {distance}) ORDER BY node_id;"
        db_results = run_query(query)

        assert len(db_results) == len(expected)
        for row in db_results:
            assert row["centrality"] == expected[row["node_id"]]

def test_laplacian_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_lap = nx.laplacian_centrality(G, normalized=False)

    query = f"{sql_edges_setup} SELECT node_id, centrality FROM onager_ctr_laplacian((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_lap)
    for row in db_results:
        assert abs(row["centrality"] - nx_lap[row["node_id"]]) < 1e-9

# =============================================================================
# 9. Additional Community Detection
# =============================================================================

# Two triangles joined by a single bridge edge: the natural two-community split.
BRIDGE_EDGES = [(1, 2), (2, 3), (3, 1), (4, 5), (5, 6), (6, 4), (3, 4)]
BRIDGE_SETUP = (
    "CREATE TEMPORARY TABLE bridge_edges AS SELECT * FROM (VALUES "
    "(1::bigint, 2::bigint), (2, 3), (3, 1), (4, 5), (5, 6), (6, 4), (3, 4)"
    ") t(src, dst);"
)

def partition_from_rows(rows, comm_key):
    groups = {}
    for row in rows:
        groups.setdefault(row[comm_key], set()).add(row["node_id"])
    return {frozenset(g) for g in groups.values()}

def test_girvan_newman():
    G = nx.Graph(BRIDGE_EDGES)
    nx_split = {frozenset(c) for c in next(nx.community.girvan_newman(G))}

    query = f"{BRIDGE_SETUP} SELECT node_id, community FROM onager_cmm_girvan_newman((SELECT src, dst FROM bridge_edges), communities := 2) ORDER BY node_id;"
    db_results = run_query(query)

    assert partition_from_rows(db_results, "community") == nx_split

def test_infomap_recovers_bridge_partition():
    G = nx.Graph(BRIDGE_EDGES)
    nx_split = {frozenset(c) for c in next(nx.community.girvan_newman(G))}

    query = f"{BRIDGE_SETUP} SELECT node_id, community FROM onager_cmm_infomap((SELECT src, dst FROM bridge_edges), seed := 42) ORDER BY node_id;"
    db_results = run_query(query)

    assert partition_from_rows(db_results, "community") == nx_split

def test_spectral_partition_validity():
    G = nx.Graph(BRIDGE_EDGES)

    query = f"{BRIDGE_SETUP} SELECT node_id, community FROM onager_cmm_spectral((SELECT src, dst FROM bridge_edges), k := 2, seed := 42) ORDER BY node_id;"
    db_results = run_query(query)

    # Spectral clustering quality varies with the k-means initialization, so
    # only the partition shape is checked: every node appears exactly once
    # and at most k communities are produced.
    assert len(db_results) == len(G.nodes)
    partition = partition_from_rows(db_results, "community")
    assert set().union(*partition) == set(G.nodes)
    assert len(partition) <= 2

# =============================================================================
# 10. Additional Metrics
# =============================================================================

def test_triangles(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_tri = nx.triangles(G)

    query = f"{sql_edges_setup} SELECT node_id, triangles FROM onager_mtr_triangles((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_tri)
    for row in db_results:
        assert row["triangles"] == nx_tri[row["node_id"]]

def test_assortativity(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_assort = nx.degree_assortativity_coefficient(G)

    query = f"{sql_edges_setup} SELECT assortativity FROM onager_mtr_assortativity((SELECT src, dst FROM edges));"
    db_results = run_query(query)

    assert len(db_results) == 1
    assert abs(db_results[0]["assortativity"] - nx_assort) < 1e-9

# =============================================================================
# 11. Traversal Order and Weighted Paths
# =============================================================================

def test_bfs_traversal(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_dist = nx.single_source_shortest_path_length(G, 1)

    query = f"{sql_edges_setup} SELECT node_id FROM onager_trv_bfs((SELECT src, dst FROM edges), source := 1);"
    db_results = run_query(query)
    order = [row["node_id"] for row in db_results]

    # A valid BFS order visits every reachable node exactly once, starts at
    # the source, and never visits a farther node before a nearer one.
    assert order[0] == 1
    assert sorted(order) == sorted(nx_dist)
    depths = [nx_dist[v] for v in order]
    assert depths == sorted(depths)

def test_dfs_traversal(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    reachable = set(nx.node_connected_component(G, 1))

    query = f"{sql_edges_setup} SELECT node_id FROM onager_trv_dfs((SELECT src, dst FROM edges), source := 1);"
    db_results = run_query(query)
    order = [row["node_id"] for row in db_results]

    assert order[0] == 1
    assert set(order) == reachable
    assert len(order) == len(reachable)

def test_dijkstra_weighted(setup_graphs):
    _, _, weighted_edges, sql_weighted_setup = setup_graphs
    G = nx.Graph()
    for u, v, w in weighted_edges:
        G.add_edge(u, v, weight=w)
    nx_dist = nx.single_source_dijkstra_path_length(G, 1)

    query = f"{sql_weighted_setup} SELECT node_id, distance FROM onager_pth_dijkstra((SELECT src, dst, weight FROM weighted_edges), source := 1) ORDER BY node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_dist)
    for row in db_results:
        assert abs(row["distance"] - nx_dist[row["node_id"]]) < 1e-9

def test_bellman_ford_negative_directed():
    neg_edges = [(1, 2, 2.0), (2, 3, -1.0), (1, 3, 3.0)]
    G = nx.DiGraph()
    for u, v, w in neg_edges:
        G.add_edge(u, v, weight=w)
    nx_dist = nx.single_source_bellman_ford_path_length(G, 1)

    edge_values = ", ".join(f"({u}::bigint, {v}::bigint, {w}::double)" for u, v, w in neg_edges)
    query = f"""
    CREATE TEMPORARY TABLE neg_edges AS SELECT * FROM (VALUES {edge_values}) t(src, dst, weight);
    SELECT node_id, distance FROM onager_pth_bellman_ford((SELECT src, dst, weight FROM neg_edges), source := 1, directed := true) ORDER BY node_id;
    """
    db_results = run_query(query)

    assert len(db_results) == len(nx_dist)
    for row in db_results:
        assert abs(row["distance"] - nx_dist[row["node_id"]]) < 1e-9

# =============================================================================
# 12. Approximation Algorithms
# =============================================================================

def test_max_clique(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    exact_clique_size = max(len(c) for c in nx.find_cliques(G))

    query = f"{sql_edges_setup} SELECT node_id FROM onager_apx_max_clique((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)
    clique = [row["node_id"] for row in db_results]

    # The returned nodes must be pairwise adjacent, and on this small graph
    # the greedy approximation finds the exact maximum clique.
    for i, u in enumerate(clique):
        for v in clique[i + 1:]:
            assert G.has_edge(u, v)
    assert len(clique) == exact_clique_size

def test_independent_set(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    exact_alpha = max(len(c) for c in nx.find_cliques(nx.complement(G)))

    query = f"{sql_edges_setup} SELECT node_id FROM onager_apx_independent_set((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)
    indep = [row["node_id"] for row in db_results]

    for i, u in enumerate(indep):
        for v in indep[i + 1:]:
            assert not G.has_edge(u, v)
    assert len(indep) == exact_alpha

def test_vertex_cover(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    exact_alpha = max(len(c) for c in nx.find_cliques(nx.complement(G)))
    optimal_cover_size = len(G.nodes) - exact_alpha

    query = f"{sql_edges_setup} SELECT node_id FROM onager_apx_vertex_cover((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)
    cover = {row["node_id"] for row in db_results}

    # Every edge must have at least one endpoint in the cover, and the
    # approximation is within twice the optimum.
    for u, v in G.edges:
        assert u in cover or v in cover
    assert optimal_cover_size <= len(cover) <= 2 * optimal_cover_size

def test_tsp_greedy():
    # A complete weighted graph on 4 nodes; greedy nearest-neighbor from the
    # same start node should produce the same tour cost as NetworkX.
    tsp_edges = [(1, 2, 1.0), (2, 3, 2.0), (3, 1, 1.5), (3, 4, 3.0), (4, 1, 2.5), (2, 4, 1.2)]
    G = nx.Graph()
    for u, v, w in tsp_edges:
        G.add_edge(u, v, weight=w)
    nx_cycle = nx.approximation.greedy_tsp(G, source=1)
    nx_cost = sum(G[nx_cycle[i]][nx_cycle[i + 1]]["weight"] for i in range(len(nx_cycle) - 1))

    edge_values = ", ".join(f"({u}::bigint, {v}::bigint, {w}::double)" for u, v, w in tsp_edges)
    query = f"""
    CREATE TEMPORARY TABLE tsp_edges AS SELECT * FROM (VALUES {edge_values}) t(src, dst, weight);
    SELECT "order", node_id FROM onager_apx_tsp((SELECT src, dst, weight FROM tsp_edges), start := 1) ORDER BY "order";
    """
    db_results = run_query(query)
    tour = [row["node_id"] for row in db_results]

    # The tour starts and ends at the start node and visits every other node
    # exactly once.
    assert tour[0] == 1 and tour[-1] == 1
    assert sorted(tour[:-1]) == sorted(G.nodes)
    db_cost = sum(G[tour[i]][tour[i + 1]]["weight"] for i in range(len(tour) - 1))
    assert abs(db_cost - nx_cost) < 1e-9

# =============================================================================
# 13. Graph Generators
# =============================================================================

def test_erdos_renyi_boundaries():
    # p = 1.0 yields the complete graph, matching NetworkX exactly.
    n = 6
    nx_complete = nx.gnp_random_graph(n, 1.0, seed=42).number_of_edges()
    db_results = run_query(f"SELECT src, dst FROM onager_gen_erdos_renyi({n}, 1.0, seed := 42);")
    assert len(db_results) == nx_complete == n * (n - 1) // 2
    for row in db_results:
        assert 0 <= row["src"] < n and 0 <= row["dst"] < n
        assert row["src"] != row["dst"]

def test_barabasi_albert_tree():
    # With m = 1 both implementations grow a tree on n nodes.
    n = 8
    nx_count = nx.barabasi_albert_graph(n, 1, seed=42).number_of_edges()
    db_results = run_query(f"SELECT src, dst FROM onager_gen_barabasi_albert({n}, 1, seed := 42);")
    assert len(db_results) == nx_count == n - 1

    G = nx.Graph((row["src"], row["dst"]) for row in db_results)
    assert nx.is_tree(G)
    assert len(G.nodes) == n

def test_watts_strogatz_ring_lattice():
    # With beta = 0 no edges are rewired, so the result is the deterministic
    # ring lattice that NetworkX also builds.
    n, k = 10, 4
    W = nx.watts_strogatz_graph(n, k, 0.0)
    nx_edges = {(min(u, v), max(u, v)) for u, v in W.edges()}

    db_results = run_query(f"SELECT src, dst FROM onager_gen_watts_strogatz({n}, {k}, 0.0, seed := 42);")
    db_edges = {(min(row["src"], row["dst"]), max(row["src"], row["dst"])) for row in db_results}

    assert db_edges == nx_edges

def test_watts_strogatz_edge_count():
    # Rewiring preserves the edge count of the ring lattice.
    n, k = 10, 4
    nx_count = nx.watts_strogatz_graph(n, k, 0.3, seed=1).number_of_edges()
    db_results = run_query(f"SELECT count(*) AS c FROM onager_gen_watts_strogatz({n}, {k}, 0.3, seed := 42);")
    assert db_results[0]["c"] == nx_count == n * k // 2

# =============================================================================
# 14. Subgraph Operations
# =============================================================================

def test_k_hop(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)

    for k in (1, 2):
        expected = set(nx.single_source_shortest_path_length(G, 1, cutoff=k))
        query = f"{sql_edges_setup} SELECT node_id FROM onager_sub_k_hop((SELECT src, dst FROM edges), start := 1, k := {k}) ORDER BY node_id;"
        db_results = run_query(query)
        assert {row["node_id"] for row in db_results} == expected

# =============================================================================
# 15. Additional Parallel Algorithms
# =============================================================================

def test_parallel_shortest_paths(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_dist = nx.single_source_shortest_path_length(G, 1)

    query = f"{sql_edges_setup} SELECT node_id, distance FROM onager_par_shortest_paths((SELECT src, dst FROM edges), source := 1) ORDER BY node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_dist)
    for row in db_results:
        assert row["distance"] == nx_dist[row["node_id"]]

def test_parallel_shortest_paths_multi(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)

    query = f"{sql_edges_setup} SELECT source, node_id, distance FROM onager_par_shortest_paths((SELECT src, dst FROM edges), sources := [1, 4]) ORDER BY source, node_id;"
    db_results = run_query(query)

    for source in (1, 4):
        nx_dist = nx.single_source_shortest_path_length(G, source)
        rows = [row for row in db_results if row["source"] == source]
        assert len(rows) == len(nx_dist)
        for row in rows:
            assert row["distance"] == nx_dist[row["node_id"]]

def test_parallel_bfs_multi(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)

    query = f"{sql_edges_setup} SELECT source, node_id FROM onager_par_bfs((SELECT src, dst FROM edges), sources := [1, 4]) ORDER BY source, node_id;"
    db_results = run_query(query)

    for source in (1, 4):
        reachable = set(nx.node_connected_component(G, source))
        visited = {row["node_id"] for row in db_results if row["source"] == source}
        assert visited == reachable

def test_parallel_clustering(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_clust = nx.clustering(G)

    query = f"{sql_edges_setup} SELECT node_id, coefficient FROM onager_par_clustering((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_clust)
    for row in db_results:
        assert abs(row["coefficient"] - nx_clust[row["node_id"]]) < 1e-9

def test_parallel_triangles(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_tri = nx.triangles(G)

    query = f"{sql_edges_setup} SELECT node_id, triangles FROM onager_par_triangles((SELECT src, dst FROM edges)) ORDER BY node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_tri)
    for row in db_results:
        assert row["triangles"] == nx_tri[row["node_id"]]

# =============================================================================
# 16. Graph Registry Scalar Functions
# =============================================================================

def test_registry_scalars_match_networkx():
    reg_edges = [(1, 2), (2, 3), (1, 3)]
    G = nx.DiGraph(reg_edges)

    # CREATE TABLE AS forces each stateful scalar call to run exactly once
    # while keeping its result out of the JSON output.
    setup = ["CREATE TEMPORARY TABLE _g AS SELECT onager_create_graph('diff_registry', true) AS v;"]
    for i, node in enumerate(sorted(G.nodes)):
        setup.append(f"CREATE TEMPORARY TABLE _n{i} AS SELECT onager_add_node('diff_registry', {node}) AS v;")
    for i, (u, v) in enumerate(reg_edges):
        setup.append(f"CREATE TEMPORARY TABLE _e{i} AS SELECT onager_add_edge('diff_registry', {u}, {v}, 1.0) AS v;")

    query = f"""
    {' '.join(setup)}
    SELECT onager_node_count('diff_registry') AS node_count,
           onager_edge_count('diff_registry') AS edge_count,
           onager_node_in_degree('diff_registry', 3) AS in_3,
           onager_node_out_degree('diff_registry', 3) AS out_3,
           onager_node_in_degree('diff_registry', 1) AS in_1,
           onager_node_out_degree('diff_registry', 1) AS out_1,
           onager_list_graphs() LIKE '%"diff_registry"%' AS listed;
    """
    db = run_query(query)[0]

    assert db["node_count"] == G.number_of_nodes()
    assert db["edge_count"] == G.number_of_edges()
    assert db["in_3"] == G.in_degree(3)
    assert db["out_3"] == G.out_degree(3)
    assert db["in_1"] == G.in_degree(1)
    assert db["out_1"] == G.out_degree(1)
    assert db["listed"] is True
