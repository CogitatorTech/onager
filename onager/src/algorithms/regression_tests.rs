//! # Check (GitHub Issue #3)

#[cfg(test)]
mod tests {
    use crate::algorithms::centrality::compute_pagerank;
    use crate::algorithms::community::{compute_connected_components, compute_louvain};

    /// Generate a random graph edge list with the given number of nodes.
    /// Creates approximately 10 edges per node on average.
    fn generate_graph_edges(num_nodes: i64) -> (Vec<i64>, Vec<i64>) {
        let mut src = Vec::new();
        let mut dst = Vec::new();
        let edges_per_node = 10;
        let total_edges = (num_nodes * edges_per_node) as usize;

        // Create a connected backbone first (node 0 -> 1 -> 2 -> ... -> n-1)
        for i in 0..(num_nodes - 1) {
            src.push(i);
            dst.push(i + 1);
        }

        // Add random edges using a simple LCG for determinism
        let mut seed: u64 = 42;
        for _ in 0..(total_edges - (num_nodes - 1) as usize) {
            seed = seed.wrapping_mul(1103515245).wrapping_add(12345);
            let s = (seed % num_nodes as u64) as i64;
            seed = seed.wrapping_mul(1103515245).wrapping_add(12345);
            let d = (seed % num_nodes as u64) as i64;
            if s != d {
                src.push(s);
                dst.push(d);
            }
        }

        (src, dst)
    }

    #[test]
    fn test_pagerank_10k_nodes() {
        let (src, dst) = generate_graph_edges(10_000);
        let result = compute_pagerank(&src, &dst, &[], 0.85, 20, true);
        assert!(result.is_ok(), "PageRank should succeed on 10k nodes");
        let pr = result.unwrap();
        assert_eq!(pr.node_ids.len(), 10_000, "Should return all 10k nodes");
        assert_eq!(pr.ranks.len(), 10_000, "Should return 10k ranks");
    }

    #[test]
    fn test_pagerank_15k_nodes() {
        let (src, dst) = generate_graph_edges(15_000);
        let result = compute_pagerank(&src, &dst, &[], 0.85, 20, true);
        assert!(result.is_ok(), "PageRank should succeed on 15k nodes");
        let pr = result.unwrap();
        assert_eq!(pr.node_ids.len(), 15_000, "Should return all 15k nodes");
    }

    #[test]
    fn test_connected_components_10k_nodes() {
        let (src, dst) = generate_graph_edges(10_000);
        let result = compute_connected_components(&src, &dst);
        assert!(
            result.is_ok(),
            "Connected components should succeed on 10k nodes"
        );
        let cc = result.unwrap();
        assert_eq!(cc.node_ids.len(), 10_000, "Should return all 10k nodes");
        // Since we create a connected backbone, should be 1 component
        let unique_components: std::collections::HashSet<_> =
            cc.component_ids.iter().copied().collect();
        assert_eq!(
            unique_components.len(),
            1,
            "Graph should be fully connected"
        );
    }

    #[test]
    fn test_connected_components_15k_nodes() {
        let (src, dst) = generate_graph_edges(15_000);
        let result = compute_connected_components(&src, &dst);
        assert!(
            result.is_ok(),
            "Connected components should succeed on 15k nodes"
        );
        let cc = result.unwrap();
        assert_eq!(cc.node_ids.len(), 15_000, "Should return all 15k nodes");
    }

    #[test]
    fn test_louvain_10k_nodes() {
        let (src, dst) = generate_graph_edges(10_000);
        let result = compute_louvain(&src, &dst, Some(42));
        assert!(result.is_ok(), "Louvain should succeed on 10k nodes");
        let lv = result.unwrap();
        assert_eq!(lv.node_ids.len(), 10_000, "Should return all 10k nodes");
    }

    #[test]
    fn test_exact_boundary_12288_nodes() {
        // This is the exact threshold where crashes were reported
        let (src, dst) = generate_graph_edges(12_288);
        let result = compute_pagerank(&src, &dst, &[], 0.85, 20, true);
        assert!(
            result.is_ok(),
            "PageRank should succeed at exact 12288 node boundary"
        );
        let pr = result.unwrap();
        assert_eq!(pr.node_ids.len(), 12_288);
    }

    #[test]
    fn test_just_above_boundary_12289_nodes() {
        // Just above the boundary
        let (src, dst) = generate_graph_edges(12_289);
        let result = compute_pagerank(&src, &dst, &[], 0.85, 20, true);
        assert!(
            result.is_ok(),
            "PageRank should succeed just above 12288 boundary"
        );
        let pr = result.unwrap();
        assert_eq!(pr.node_ids.len(), 12_289);
    }

    #[test]
    fn test_connected_components_50k_nodes() {
        let (src, dst) = generate_graph_edges(50_000);
        let result = compute_connected_components(&src, &dst);
        assert!(
            result.is_ok(),
            "Connected components should succeed on 50k nodes (GitHub #3 regression test)"
        );
        let cc = result.unwrap();
        assert_eq!(cc.node_ids.len(), 50_000, "Should return all 50k nodes");
    }

    #[test]
    fn test_pagerank_50k_nodes() {
        let (src, dst) = generate_graph_edges(50_000);
        // Use fewer iterations for faster test execution
        let result = compute_pagerank(&src, &dst, &[], 0.85, 10, true);
        assert!(
            result.is_ok(),
            "PageRank should succeed on 50k nodes (GitHub #3 regression test)"
        );
        let pr = result.unwrap();
        assert_eq!(pr.node_ids.len(), 50_000, "Should return all 50k nodes");
    }

    #[test]
    fn test_louvain_20k_nodes() {
        let (src, dst) = generate_graph_edges(20_000);
        let result = compute_louvain(&src, &dst, Some(42));
        assert!(
            result.is_ok(),
            "Louvain should succeed on 20k nodes (GitHub #3 regression test)"
        );
        let lv = result.unwrap();
        assert_eq!(lv.node_ids.len(), 20_000, "Should return all 20k nodes");
    }
}
