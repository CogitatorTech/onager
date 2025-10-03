include_directories(${CMAKE_CURRENT_LIST_DIR}/onager/bindings/include)

duckdb_extension_load(onager
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
    LOAD_TESTS
)

# Manually link the pre-built Rust static library into the generated extension targets.
# The Rust library is built beforehand by the Makefile (cargo build --release --features duckdb_extension).
set(ONAGER_RUST_LIB ${CMAKE_CURRENT_LIST_DIR}/onager/target/release/libonager.a)

# Collect candidate paths in priority order
set(_ONAGER_RUST_CANDIDATES)

# 1. If CARGO_TARGET_DIR env var set (relative like target/<triple> or absolute) add its release path
if(DEFINED ENV{CARGO_TARGET_DIR})
    set(_CTD $ENV{CARGO_TARGET_DIR})
    # Normalize possible relative path (cargo executed from onager crate dir)
    if(NOT IS_ABSOLUTE "${_CTD}")
        set(_CTD_FULL ${CMAKE_CURRENT_LIST_DIR}/onager/${_CTD})
    else()
        set(_CTD_FULL ${_CTD})
    endif()
    list(APPEND _ONAGER_RUST_CANDIDATES ${_CTD_FULL}/release/libonager.a ${_CTD_FULL}/release/onager.lib)
endif()

# 2. Target-triple specific directory if Rust_CARGO_TARGET defined
if(DEFINED Rust_CARGO_TARGET AND NOT "${Rust_CARGO_TARGET}" STREQUAL "")
    list(APPEND _ONAGER_RUST_CANDIDATES
        ${CMAKE_CURRENT_LIST_DIR}/onager/target/${Rust_CARGO_TARGET}/release/libonager.a
        ${CMAKE_CURRENT_LIST_DIR}/onager/target/${Rust_CARGO_TARGET}/release/onager.lib)
endif()

# 3. Default host target release dir (pre-CARGO_TARGET_DIR layout)
list(APPEND _ONAGER_RUST_CANDIDATES ${CMAKE_CURRENT_LIST_DIR}/onager/target/release/libonager.a ${CMAKE_CURRENT_LIST_DIR}/onager/target/release/onager.lib)

# Select first existing candidate
foreach(_cand IN LISTS _ONAGER_RUST_CANDIDATES)
    if(EXISTS ${_cand})
        set(ONAGER_RUST_LIB ${_cand})
        message(STATUS "[onager] Selected Rust static library: ${ONAGER_RUST_LIB}")
        break()
    endif()
endforeach()

# If the expected default path does not exist (common on Windows MSVC or when using target triples),
# try alternate locations and naming conventions.
if(NOT EXISTS ${ONAGER_RUST_LIB})
    # Look for MSVC-style static library name (onager.lib) in the release root
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/onager/target/release/onager.lib)
        set(ONAGER_RUST_LIB ${CMAKE_CURRENT_LIST_DIR}/onager/target/release/onager.lib)
    else()
        # Glob any target-triple subdirectory build products (first match wins)
        file(GLOB _ONAGER_ALT_LIBS
            "${CMAKE_CURRENT_LIST_DIR}/onager/target/*/release/libonager.a"
            "${CMAKE_CURRENT_LIST_DIR}/onager/target/*/release/onager.lib"
        )
        list(LENGTH _ONAGER_ALT_LIBS _ALT_COUNT)
        if(_ALT_COUNT GREATER 0)
            list(GET _ONAGER_ALT_LIBS 0 ONAGER_RUST_LIB)
            message(STATUS "[onager] Using alternate discovered Rust library: ${ONAGER_RUST_LIB}")
        endif()
    endif()
endif()

if (EXISTS ${ONAGER_RUST_LIB})
    message(STATUS "[onager] Found Rust library at: ${ONAGER_RUST_LIB}")

    # Create an imported target for the Rust library
    add_library(onager_rust STATIC IMPORTED GLOBAL)
    if(UNIX)
        # We always use pthread, dl, and m on Unix
        set(_ONAGER_RUST_LINK_LIBS "pthread;dl;m")
    else()
        set(_ONAGER_RUST_LINK_LIBS "")
    endif()
    set_target_properties(onager_rust PROPERTIES
        IMPORTED_LOCATION ${ONAGER_RUST_LIB}
        INTERFACE_LINK_LIBRARIES "${_ONAGER_RUST_LINK_LIBS}"
    )

    # Add the Rust library to global link libraries so it gets linked to everything
    if(UNIX)
        link_libraries(${ONAGER_RUST_LIB} pthread dl m)
    else()
        link_libraries(${ONAGER_RUST_LIB})
        if(WIN32)
            # Explicitly add Windows system libraries required by Rust dependencies (mio, std I/O paths)
            # Nt* symbols come from ntdll; others from userenv/dbghelp; bcrypt often pulled by crypto backends.
            set(_ONAGER_WIN_SYSTEM_LIBS ntdll userenv dbghelp bcrypt)
            link_libraries(${_ONAGER_WIN_SYSTEM_LIBS})
            target_link_libraries(onager_rust INTERFACE ${_ONAGER_WIN_SYSTEM_LIBS})
        endif()
    endif()

    if(TARGET onager_extension)
        target_link_libraries(onager_extension onager_rust)
        message(STATUS "[onager] Linked Rust library to onager_extension")
    endif()

    if(TARGET onager_loadable_extension)
        target_link_libraries(onager_loadable_extension onager_rust)
        message(STATUS "[onager] Linked Rust library to onager_loadable_extension")
    endif()

    if(UNIX)
        add_link_options($<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${ONAGER_RUST_LIB}>)
        add_link_options($<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-lpthread>)
        add_link_options($<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-ldl>)
        add_link_options($<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-lm>)
    else()
        add_link_options($<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${ONAGER_RUST_LIB}>)
    endif()
else()
    message(WARNING "[onager] Expected Rust static library not found at ${ONAGER_RUST_LIB}. Build Rust crate first.")
endif()
