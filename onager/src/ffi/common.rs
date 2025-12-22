//! Shared FFI utilities.
//!
//! Error handling and utility functions shared across FFI modules.

use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::sync::atomic::{AtomicPtr, Ordering};

use once_cell::sync::Lazy;

use crate::graph;

/// Version string for the extension.
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Thread-local last error message.
pub static LAST_ERROR: Lazy<AtomicPtr<c_char>> = Lazy::new(|| AtomicPtr::new(std::ptr::null_mut()));

/// Sets the last error message.
pub fn set_last_error(msg: &str) {
    // Use from_vec_unchecked for fallback since "unknown error" has no NUL bytes
    let c_string = CString::new(msg).unwrap_or_else(|_| {
        // Safety: "unknown error" contains no NUL bytes
        unsafe { CString::from_vec_unchecked(b"unknown error".to_vec()) }
    });
    let ptr = c_string.into_raw();
    let old = LAST_ERROR.swap(ptr, Ordering::SeqCst);
    if !old.is_null() {
        unsafe {
            drop(CString::from_raw(old));
        }
    }
}

/// Clears the last error message.
pub fn clear_last_error() {
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
/// # Safety
/// The pointer must have been allocated by Onager (via CString::into_raw).
#[no_mangle]
pub unsafe extern "C" fn onager_free(ptr: *mut c_char) {
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
/// # Safety
/// The name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_create_graph(name: *const c_char, directed: bool) -> i32 {
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
/// # Safety
/// The name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_drop_graph(name: *const c_char) -> i32 {
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
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_add_node(graph_name: *const c_char, node_id: i64) -> i32 {
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
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_add_edge(
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
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_node_count(graph_name: *const c_char) -> i64 {
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
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_edge_count(graph_name: *const c_char) -> i64 {
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

/// Returns the in-degree of a node in the named graph.
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_graph_node_in_degree(graph_name: *const c_char, node: i64) -> i64 {
    clear_last_error();
    let name = match unsafe { CStr::from_ptr(graph_name) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_last_error("Invalid UTF-8 in graph name");
            return -1;
        }
    };
    match graph::get_node_in_degree(name, node) {
        Ok(degree) => degree as i64,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}

/// Returns the out-degree of a node in the named graph.
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_graph_node_out_degree(graph_name: *const c_char, node: i64) -> i64 {
    clear_last_error();
    let name = match unsafe { CStr::from_ptr(graph_name) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            set_last_error("Invalid UTF-8 in graph name");
            return -1;
        }
    };
    match graph::get_node_out_degree(name, node) {
        Ok(degree) => degree as i64,
        Err(e) => {
            set_last_error(&e.to_string());
            -1
        }
    }
}
