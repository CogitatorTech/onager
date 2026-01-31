//! Shared FFI utilities.
//!
//! Error handling and utility functions shared across FFI modules.

use std::cell::RefCell;
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::panic;

use crate::graph;

/// Wraps an FFI function body with catch_unwind to prevent panics from crossing FFI boundary.
/// Returns the provided error_value if a panic occurs.
///
/// # Safety
/// This function catches panics and converts them to error values, preventing undefined behavior
/// when Rust panics would otherwise propagate across the C FFI boundary.
pub fn catch_unwind_ffi<F, T>(error_value: T, f: F) -> T
where
    F: FnOnce() -> T + panic::UnwindSafe,
{
    match panic::catch_unwind(f) {
        Ok(result) => result,
        Err(panic_info) => {
            // Try to extract a message from the panic
            let msg = if let Some(s) = panic_info.downcast_ref::<&str>() {
                format!("Internal panic: {}", s)
            } else if let Some(s) = panic_info.downcast_ref::<String>() {
                format!("Internal panic: {}", s)
            } else {
                "Internal panic (unknown cause)".to_string()
            };
            set_last_error(&msg);
            error_value
        }
    }
}

/// Helper macro for wrapping FFI function bodies with catch_unwind.
/// Use AssertUnwindSafe for closures that capture mutable state.
#[macro_export]
macro_rules! ffi_catch_unwind {
    ($error_val:expr, $body:expr) => {
        $crate::ffi::common::catch_unwind_ffi($error_val, std::panic::AssertUnwindSafe(|| $body))
    };
}

/// Version string for the extension.
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

// Thread-local last error message.
// Each thread has its own error state, preventing concurrent queries from
// overwriting each other's errors.
thread_local! {
    static LAST_ERROR: RefCell<Option<CString>> = const { RefCell::new(None) };
}

/// Sets the last error message.
pub fn set_last_error(msg: &str) {
    let c_string = CString::new(msg).unwrap_or_else(|_| {
        // Safety: "unknown error" contains no NUL bytes
        unsafe { CString::from_vec_unchecked(b"unknown error".to_vec()) }
    });
    LAST_ERROR.with(|cell| {
        *cell.borrow_mut() = Some(c_string);
    });
}

/// Clears the last error message.
pub fn clear_last_error() {
    LAST_ERROR.with(|cell| {
        *cell.borrow_mut() = None;
    });
}

/// Returns the last error message, or null if no error is set.
/// Note: The returned pointer is only valid until the next call to
/// set_last_error or clear_last_error on the same thread.
#[no_mangle]
pub extern "C" fn onager_last_error() -> *const c_char {
    LAST_ERROR.with(|cell| {
        cell.borrow()
            .as_ref()
            .map(|s| s.as_ptr())
            .unwrap_or(std::ptr::null())
    })
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
    crate::ffi_catch_unwind!(-1, {
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
    })
}

/// Drops a graph with the given name.
/// # Safety
/// The name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_drop_graph(name: *const c_char) -> i32 {
    clear_last_error();
    crate::ffi_catch_unwind!(-1, {
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
    })
}

/// Returns a JSON array of all graph names.
#[no_mangle]
pub extern "C" fn onager_list_graphs() -> *mut c_char {
    clear_last_error();
    crate::ffi_catch_unwind!(std::ptr::null_mut(), {
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
    })
}

/// Adds a node to the specified graph.
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_add_node(graph_name: *const c_char, node_id: i64) -> i32 {
    clear_last_error();
    crate::ffi_catch_unwind!(-1, {
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
    })
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
    crate::ffi_catch_unwind!(-1, {
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
    })
}

/// Returns the number of nodes in the graph.
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_node_count(graph_name: *const c_char) -> i64 {
    clear_last_error();
    crate::ffi_catch_unwind!(-1, {
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
    })
}

/// Returns the number of edges in the graph.
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_edge_count(graph_name: *const c_char) -> i64 {
    clear_last_error();
    crate::ffi_catch_unwind!(-1, {
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
    })
}

/// Returns the in-degree of a node in the named graph.
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_graph_node_in_degree(graph_name: *const c_char, node: i64) -> i64 {
    clear_last_error();
    crate::ffi_catch_unwind!(-1, {
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
    })
}

/// Returns the out-degree of a node in the named graph.
/// # Safety
/// The graph_name pointer must be a valid null-terminated C string.
#[no_mangle]
pub unsafe extern "C" fn onager_graph_node_out_degree(graph_name: *const c_char, node: i64) -> i64 {
    clear_last_error();
    crate::ffi_catch_unwind!(-1, {
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
    })
}
