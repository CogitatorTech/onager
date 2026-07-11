import subprocess
import json
import os
import pytest
import networkx as nx

DUCKDB_PATH = "./build/release/duckdb"
EXTENSION_PATH = "build/release/extension/onager/onager.duckdb_extension"

def run_query(sql):
    cmd = [DUCKDB_PATH, "-light-mode", "-json", "-c", f"load '{EXTENSION_PATH}'; {sql}"]
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
    sql_edges_setup = f"create temporary table edges as select * from (values {edge_values}) t(src, dst);"

    # A weighted graph: 1-2 (weight 1.0), 2-3 (2.0), 3-1 (1.5), 3-4 (3.0)
    weighted_edges = [(1, 2, 1.0), (2, 3, 2.0), (3, 1, 1.5), (3, 4, 3.0)]
    w_edge_values = ", ".join(f"({u}::bigint, {v}::bigint, {w}::double)" for u, v, w in weighted_edges)
    sql_weighted_setup = f"create temporary table weighted_edges as select * from (values {w_edge_values}) t(src, dst, weight);"

    return edges, sql_edges_setup, weighted_edges, sql_weighted_setup

# =============================================================================
# 1. Centrality Tests
# =============================================================================

def test_pagerank_directed(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.DiGraph(edges)
    nx_pagerank = nx.pagerank(G, alpha=0.85, tol=1e-6)
    
    query = f"{sql_edges_setup} select node_id, rank from onager_ctr_pagerank((select src, dst from edges), directed := true) order by node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_pagerank)
    for row in db_results:
        node = row["node_id"]
        assert abs(row["rank"] - nx_pagerank[node]) < 1e-4

def test_pagerank_undirected(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_pagerank = nx.pagerank(G, alpha=0.85, tol=1e-6)
    
    query = f"{sql_edges_setup} select node_id, rank from onager_ctr_pagerank((select src, dst from edges), directed := false) order by node_id;"
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
    
    query = f"{sql_edges_setup} select node_id, in_degree, out_degree from onager_ctr_degree((select src, dst from edges), directed := true) order by node_id;"
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
    
    query = f"{sql_edges_setup} select node_id, betweenness from onager_ctr_betweenness((select src, dst from edges), normalized := true) order by node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_bet)
    for row in db_results:
        node = row["node_id"]
        assert abs(row["betweenness"] - nx_bet[node]) < 1e-4

def test_closeness_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_close = nx.closeness_centrality(G)
    
    query = f"{sql_edges_setup} select node_id, closeness from onager_ctr_closeness((select src, dst from edges)) order by node_id;"
    db_results = run_query(query)
    
    assert len(db_results) == len(nx_close)
    for row in db_results:
        node = row["node_id"]
        assert abs(row["closeness"] - nx_close[node]) < 1e-4

def test_eigenvector_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_eig = nx_eigenvector = nx.eigenvector_centrality(G, max_iter=1000, tol=1e-6)
    
    query = f"{sql_edges_setup} select node_id, eigenvector from onager_ctr_eigenvector((select src, dst from edges)) order by node_id;"
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
    
    query = f"{sql_edges_setup} select node_id, katz from onager_ctr_katz((select src, dst from edges), alpha := 0.1) order by node_id;"
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
    
    query = f"{sql_edges_setup} select node_id, harmonic from onager_ctr_harmonic((select src, dst from edges)) order by node_id;"
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
    
    query = f"{sql_edges_setup} select node_id from onager_ctr_voterank((select src, dst from edges), num_seeds := 2);"
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
    
    query = f"{sql_edges_setup} select node_id, component from onager_cmm_components((select src, dst from edges)) order by node_id;"
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
    query = f"{sql_edges_setup} select node_id, community from onager_cmm_louvain((select src, dst from edges), seed := 42) order by node_id;"
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
    query = f"{sql_edges_setup} select node_id, label from onager_cmm_label_prop((select src, dst from edges)) order by node_id;"
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
    
    query = f"{sql_edges_setup} select node1, node2, coefficient from onager_lnk_jaccard((select src, dst from edges)) order by node1, node2;"
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
    
    query = f"{sql_edges_setup} select node1, node2, score from onager_lnk_adamic_adar((select src, dst from edges)) order by node1, node2;"
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
    
    query = f"{sql_edges_setup} select node1, node2, score from onager_lnk_pref_attach((select src, dst from edges)) order by node1, node2;"
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
    
    query = f"{sql_edges_setup} select node1, node2, score from onager_lnk_resource_alloc((select src, dst from edges)) order by node1, node2;"
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
    
    query = f"{sql_edges_setup} select node1, node2, count from onager_lnk_common_neighbors((select src, dst from edges)) order by node1, node2;"
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
    select 
        (select density from onager_mtr_density((select src, dst from edges))) as density,
        (select diameter from onager_mtr_diameter((select src, dst from edges))) as diameter,
        (select radius from onager_mtr_radius((select src, dst from edges))) as radius,
        (select avg_clustering from onager_mtr_avg_clustering((select src, dst from edges))) as avg_clustering,
        (select avg_path_length from onager_mtr_avg_path_length((select src, dst from edges))) as avg_path_length,
        (select transitivity from onager_mtr_transitivity((select src, dst from edges))) as transitivity;
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
    
    query = f"{sql_edges_setup} select node_id, distance from onager_pth_dijkstra((select src, dst from edges), source := 1) order by node_id;"
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
    
    query = f"{sql_weighted_setup} select node_id, distance from onager_pth_bellman_ford((select src, dst, weight from weighted_edges), source := 1) order by node_id;"
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
    
    query = f"{sql_weighted_setup} select src, dst, distance from onager_pth_floyd_warshall((select src, dst, weight from weighted_edges)) order by src, dst;"
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
    
    query = f"{sql_weighted_setup} select src, dst, weight from onager_mst_kruskal((select src, dst, weight from weighted_edges));"
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
    
    query = f"{sql_weighted_setup} select src, dst, weight from onager_mst_prim((select src, dst, weight from weighted_edges));"
    db_results = run_query(query)
    
    db_weight = sum(row["weight"] for row in db_results)
    assert abs(db_weight - nx_weight) < 1e-4
    assert len(db_results) == len(G.nodes) - 1

def test_ego_graph(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    
    # nx.ego_graph(G, 1, radius=1)
    nx_ego = nx.ego_graph(G, 1, radius=1)
    
    query = f"{sql_edges_setup} select src, dst from onager_sub_ego_graph((select src, dst from edges), center := 1, radius := 1) order by src, dst;"
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
    create temporary table filter_nodes (node bigint);
    insert into filter_nodes values (1), (2), (3);
    select src, dst from onager_sub_induced((
        select e.src, e.dst, f.node 
        from edges e cross join filter_nodes f
    )) order by src, dst;
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
    select 
        s.node_id, 
        s.rank as seq_rank, 
        p.rank as par_rank 
    from onager_ctr_pagerank((select src, dst from edges)) s
    join onager_par_pagerank((select src, dst from edges)) p
      on s.node_id = p.node_id
    order by s.node_id;
    """
    db_results = run_query(query)
    assert len(db_results) > 0
    for row in db_results:
        assert abs(row["seq_rank"] - row["par_rank"]) < 1e-6

def test_parallel_pagerank_weighted(setup_graphs):
    _, _, weighted_edges, sql_weighted_setup = setup_graphs
    G = nx.Graph()
    for u, v, w in weighted_edges:
        G.add_edge(u, v, weight=w)
    nx_pagerank = nx.pagerank(G, alpha=0.85, tol=1e-6)

    query = f"{sql_weighted_setup} select node_id, rank from onager_par_pagerank((select src, dst, weight from weighted_edges)) order by node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_pagerank)
    for row in db_results:
        node = row["node_id"]
        assert abs(row["rank"] - nx_pagerank[node]) < 1e-4

def test_parallel_bfs(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    
    query = f"""
    {sql_edges_setup}
    select count(*) as cnt 
    from onager_par_bfs((select src, dst from edges), source := 1);
    """
    db_results = run_query(query)[0]
    assert db_results["cnt"] == 4

def test_parallel_components(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    
    query = f"""
    {sql_edges_setup}
    select 
        s.node_id, 
        s.component as seq_comp, 
        p.component as par_comp 
    from onager_cmm_components((select src, dst from edges)) s
    join onager_par_components((select src, dst from edges)) p
      on s.node_id = p.node_id
    order by s.node_id;
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
    create temporary table pedges as select * from (values
      (1::bigint, 2::bigint, 1::bigint, 0.7::double),
      (2, 3, 4, 0.3),
      (3, 1, null, null),
      (3, 4, null, null)
    ) t(src, dst, pnode, pweight);
    select node_id, score from onager_ctr_personalized_pagerank((
      select src, dst, pnode, pweight from pedges
    )) order by node_id;
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
        query = f"{sql_edges_setup} select node_id, centrality from onager_ctr_local_reaching((select src, dst from edges), distance := {distance}) order by node_id;"
        db_results = run_query(query)

        assert len(db_results) == len(expected)
        for row in db_results:
            assert row["centrality"] == expected[row["node_id"]]

def test_laplacian_centrality(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_lap = nx.laplacian_centrality(G, normalized=False)

    query = f"{sql_edges_setup} select node_id, centrality from onager_ctr_laplacian((select src, dst from edges)) order by node_id;"
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
    "create temporary table bridge_edges as select * from (values "
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

    query = f"{BRIDGE_SETUP} select node_id, community from onager_cmm_girvan_newman((select src, dst from bridge_edges), communities := 2) order by node_id;"
    db_results = run_query(query)

    assert partition_from_rows(db_results, "community") == nx_split

def test_infomap_recovers_bridge_partition():
    G = nx.Graph(BRIDGE_EDGES)
    nx_split = {frozenset(c) for c in next(nx.community.girvan_newman(G))}

    query = f"{BRIDGE_SETUP} select node_id, community from onager_cmm_infomap((select src, dst from bridge_edges), seed := 42) order by node_id;"
    db_results = run_query(query)

    assert partition_from_rows(db_results, "community") == nx_split

def test_spectral_partition_validity():
    G = nx.Graph(BRIDGE_EDGES)

    query = f"{BRIDGE_SETUP} select node_id, community from onager_cmm_spectral((select src, dst from bridge_edges), k := 2, seed := 42) order by node_id;"
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

    query = f"{sql_edges_setup} select node_id, triangles from onager_mtr_triangles((select src, dst from edges)) order by node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_tri)
    for row in db_results:
        assert row["triangles"] == nx_tri[row["node_id"]]

def test_assortativity(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_assort = nx.degree_assortativity_coefficient(G)

    query = f"{sql_edges_setup} select assortativity from onager_mtr_assortativity((select src, dst from edges));"
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

    query = f"{sql_edges_setup} select node_id from onager_trv_bfs((select src, dst from edges), source := 1);"
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

    query = f"{sql_edges_setup} select node_id from onager_trv_dfs((select src, dst from edges), source := 1);"
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

    query = f"{sql_weighted_setup} select node_id, distance from onager_pth_dijkstra((select src, dst, weight from weighted_edges), source := 1) order by node_id;"
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
    create temporary table neg_edges as select * from (values {edge_values}) t(src, dst, weight);
    select node_id, distance from onager_pth_bellman_ford((select src, dst, weight from neg_edges), source := 1, directed := true) order by node_id;
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

    query = f"{sql_edges_setup} select node_id from onager_apx_max_clique((select src, dst from edges)) order by node_id;"
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

    query = f"{sql_edges_setup} select node_id from onager_apx_independent_set((select src, dst from edges)) order by node_id;"
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

    query = f"{sql_edges_setup} select node_id from onager_apx_vertex_cover((select src, dst from edges)) order by node_id;"
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
    create temporary table tsp_edges as select * from (values {edge_values}) t(src, dst, weight);
    select "order", node_id from onager_apx_tsp((select src, dst, weight from tsp_edges), start := 1) order by "order";
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
    db_results = run_query(f"select src, dst from onager_gen_erdos_renyi({n}, 1.0, seed := 42);")
    assert len(db_results) == nx_complete == n * (n - 1) // 2
    for row in db_results:
        assert 0 <= row["src"] < n and 0 <= row["dst"] < n
        assert row["src"] != row["dst"]

def test_barabasi_albert_tree():
    # With m = 1 both implementations grow a tree on n nodes.
    n = 8
    nx_count = nx.barabasi_albert_graph(n, 1, seed=42).number_of_edges()
    db_results = run_query(f"select src, dst from onager_gen_barabasi_albert({n}, 1, seed := 42);")
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

    db_results = run_query(f"select src, dst from onager_gen_watts_strogatz({n}, {k}, 0.0, seed := 42);")
    db_edges = {(min(row["src"], row["dst"]), max(row["src"], row["dst"])) for row in db_results}

    assert db_edges == nx_edges

def test_watts_strogatz_edge_count():
    # Rewiring preserves the edge count of the ring lattice.
    n, k = 10, 4
    nx_count = nx.watts_strogatz_graph(n, k, 0.3, seed=1).number_of_edges()
    db_results = run_query(f"select count(*) as c from onager_gen_watts_strogatz({n}, {k}, 0.3, seed := 42);")
    assert db_results[0]["c"] == nx_count == n * k // 2

# =============================================================================
# 14. Subgraph Operations
# =============================================================================

def test_k_hop(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)

    for k in (1, 2):
        expected = set(nx.single_source_shortest_path_length(G, 1, cutoff=k))
        query = f"{sql_edges_setup} select node_id from onager_sub_k_hop((select src, dst from edges), start := 1, k := {k}) order by node_id;"
        db_results = run_query(query)
        assert {row["node_id"] for row in db_results} == expected

# =============================================================================
# 15. Additional Parallel Algorithms
# =============================================================================

def test_parallel_shortest_paths(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_dist = nx.single_source_shortest_path_length(G, 1)

    query = f"{sql_edges_setup} select node_id, distance from onager_par_shortest_paths((select src, dst from edges), source := 1) order by node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_dist)
    for row in db_results:
        assert row["distance"] == nx_dist[row["node_id"]]

def test_parallel_shortest_paths_multi(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)

    query = f"{sql_edges_setup} select source, node_id, distance from onager_par_shortest_paths((select src, dst from edges), sources := [1, 4]) order by source, node_id;"
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

    query = f"{sql_edges_setup} select source, node_id from onager_par_bfs((select src, dst from edges), sources := [1, 4]) order by source, node_id;"
    db_results = run_query(query)

    for source in (1, 4):
        reachable = set(nx.node_connected_component(G, source))
        visited = {row["node_id"] for row in db_results if row["source"] == source}
        assert visited == reachable

def test_parallel_clustering(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_clust = nx.clustering(G)

    query = f"{sql_edges_setup} select node_id, coefficient from onager_par_clustering((select src, dst from edges)) order by node_id;"
    db_results = run_query(query)

    assert len(db_results) == len(nx_clust)
    for row in db_results:
        assert abs(row["coefficient"] - nx_clust[row["node_id"]]) < 1e-9

def test_parallel_triangles(setup_graphs):
    edges, sql_edges_setup, _, _ = setup_graphs
    G = nx.Graph(edges)
    nx_tri = nx.triangles(G)

    query = f"{sql_edges_setup} select node_id, triangles from onager_par_triangles((select src, dst from edges)) order by node_id;"
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

    # Wrapping each stateful scalar call in `create table as` forces it to
    # run exactly once while keeping its result out of the JSON output.
    setup = ["create temporary table _g as select onager_create_graph('diff_registry', true) as v;"]
    for i, node in enumerate(sorted(G.nodes)):
        setup.append(f"create temporary table _n{i} as select onager_add_node('diff_registry', {node}) as v;")
    for i, (u, v) in enumerate(reg_edges):
        setup.append(f"create temporary table _e{i} as select onager_add_edge('diff_registry', {u}, {v}, 1.0) as v;")

    query = f"""
    {' '.join(setup)}
    select onager_node_count('diff_registry') as node_count,
           onager_edge_count('diff_registry') as edge_count,
           onager_node_in_degree('diff_registry', 3) as in_3,
           onager_node_out_degree('diff_registry', 3) as out_3,
           onager_node_in_degree('diff_registry', 1) as in_1,
           onager_node_out_degree('diff_registry', 1) as out_1,
           onager_list_graphs() like '%"diff_registry"%' as listed;
    """
    db = run_query(query)[0]

    assert db["node_count"] == G.number_of_nodes()
    assert db["edge_count"] == G.number_of_edges()
    assert db["in_3"] == G.in_degree(3)
    assert db["out_3"] == G.out_degree(3)
    assert db["in_1"] == G.in_degree(1)
    assert db["out_1"] == G.out_degree(1)
    assert db["listed"] is True
