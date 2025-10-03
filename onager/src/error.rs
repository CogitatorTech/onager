//! Error types for Onager extension.

use thiserror::Error;

/// Result type alias for Onager operations.
pub type Result<T> = std::result::Result<T, OnagerError>;

/// Errors that can occur in Onager operations.
#[derive(Error, Debug)]
pub enum OnagerError {
    /// Graph with the given name was not found.
    #[error("Graph not found: {0}")]
    GraphNotFound(String),

    /// Graph with the given name already exists.
    #[error("Graph already exists: {0}")]
    GraphAlreadyExists(String),

    /// Node was not found in the graph.
    #[error("Node not found: {0}")]
    NodeNotFound(i64),

    /// Invalid argument provided.
    #[error("Invalid argument: {0}")]
    InvalidArgument(String),

    /// Internal error from graphina library.
    #[error("Graph error: {0}")]
    GraphError(String),

    /// Serialization error.
    #[error("Serialization error: {0}")]
    SerializationError(String),
}

impl From<serde_json::Error> for OnagerError {
    fn from(err: serde_json::Error) -> Self {
        OnagerError::SerializationError(err.to_string())
    }
}
