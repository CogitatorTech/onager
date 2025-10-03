//! FFI bindings for Onager extension.
//!
//! This module provides C-compatible functions for the DuckDB C++ extension
//! to call into the Rust graph operations.

use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::sync::atomic::{AtomicPtr, Ordering};

use once_cell::sync::Lazy;

use crate::graph;

/// Version string for the extension.
const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Thread-local last error message.
static LAST_ERROR: Lazy<AtomicPtr<c_char>> = Lazy::new(|| AtomicPtr::new(std::ptr::null_mut()));

/// Sets the last error message.
fn set_last_error(msg: &str) {
    let c_string = CString::new(msg).unwrap_or_else(|_| CString::new("unknown error").unwrap());
    let ptr = c_string.into_raw();
    let old = LAST_ERROR.swap(ptr, Ordering::SeqCst);
    if !old.is_null() {
        // Free the old error string
        unsafe {
            drop(CString::from_raw(old));
        }
    }
}

/// Clears the last error message.
fn clear_last_error() {
    let old = LAST_ERROR.swap(std::ptr::null_mut(), Ordering::SeqCst);
    if !old.is_null() {
        unsafe {
            drop(CString::from_raw(old));
        }
    }
}

/// Returns the last error message, or null if no error is set.
#[no_mangle]
pub extern "C" fn onager_last_error() -> *const c_char {
    LAST_ERROR.load(Ordering::SeqCst) as *const c_char
}

/// Frees a string allocated by Onager.
#[no_mangle]
pub extern "C" fn onager_free(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe {
            drop(CString::from_raw(ptr));
        }
    }
}

/// Returns the version string, caller must free with onager_free.
#[no_mangle]
pub extern "C" fn onager_get_version() -> *mut c_char {
    CString::new(VERSION)
        .map(|s| s.into_raw())
        .unwrap_or(std::ptr::null_mut())
}

/// Creates a new graph with the given name.
/// Returns 0 on success, -1 on error.
#[no_mangle]
pub extern "C" fn onager_create_graph(name: *const c_char, directed: bool) -> i32 {
    clear_last_error();
    
    let name = match unsafe { CStr::from_ptr(name) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_last_error("Invalid UTF-8 in graph name");
            return -1;
        }
    };

    match graph::create_graph(name, directed) {
        Ok(()) => 0,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Drops a graph with the given name.
/// Returns 0 on success, -1 on error.
#[no_mangle]
pub extern "C" fn onager_drop_graph(name: *const c_char) -> i32 {
    clear_last_error();
    
    let name = match unsafe { CStr::from_ptr(name) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_last_error("Invalid UTF-8 in graph name");
            return -1;
        }
    };

    match graph::drop_graph(name) {
        Ok(()) => 0,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Returns a JSON array of all graph names.
/// Caller must free with onager_free.
#[no_mangle]
pub extern "C" fn onager_list_graphs() -> *mut c_char {
    clear_last_error();
    
    let graphs = graph::list_graphs();
    let json = match serde_json::to_string(&graphs) {
        Ok(s) => s,
        Err(e) => {
            set_last_error(&e.to_string());
            return std::ptr::null_mut();
        }
    };

    CString::new(json)
        .map(|s| s.into_raw())
        .unwrap_or(std::ptr::null_mut())
}

/// Adds a node to the specified graph.
/// Returns 0 on success, -1 on error.
#[no_mangle]
pub extern "C" fn onager_add_node(graph_name: *const c_char, node_id: i64) -> i32 {
    clear_last_error();
    
    let name = match unsafe { CStr::from_ptr(graph_name) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_last_error("Invalid UTF-8 in graph name");
            return -1;
        }
    };

    match graph::add_node(name, node_id) {
        Ok(()) => 0,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Adds an edge to the specified graph.
/// Returns 0 on success, -1 on error.
#[no_mangle]
pub extern "C" fn onager_add_edge(
    graph_name: *const c_char,
    src: i64,
    dst: i64,
    weight: f64,
) -> i32 {
    clear_last_error();
    
    let name = match unsafe { CStr::from_ptr(graph_name) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_last_error("Invalid UTF-8 in graph name");
            return -1;
        }
    };

    match graph::add_edge(name, src, dst, weight) {
        Ok(()) => 0,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Returns the number of nodes in the graph.
/// Returns -1 on error.
#[no_mangle]
pub extern "C" fn onager_node_count(graph_name: *const c_char) -> i64 {
    clear_last_error();
    
    let name = match unsafe { CStr::from_ptr(graph_name) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_last_error("Invalid UTF-8 in graph name");
            return -1;
        }
    };

    match graph::node_count(name) {
        Ok(count) => count as i64,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Returns the number of edges in the graph.
/// Returns -1 on error.
#[no_mangle]
pub extern "C" fn onager_edge_count(graph_name: *const c_char) -> i64 {
    clear_last_error();
    
    let name = match unsafe { CStr::from_ptr(graph_name) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_last_error("Invalid UTF-8 in graph name");
            return -1;
        }
    };

    match graph::edge_count(name) {
        Ok(count) => count as i64,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
