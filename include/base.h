// base.h
// Portable foundation for TigerStyle C++.
// Zero dependencies beyond the C++ standard library.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <expected>
#include <optional>
#include <print>
#include <span>
#include <string_view>
#include <utility>

// --- Primitive types ---

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

// --- Error handling ---
//
// Each project defines its own Error as an enum class : u32.
// Use std::expected<T, E> for functions that can fail.
// Use std::optional<T> for value-or-nothing semantics.
//
// Access pattern for std::expected:
//   if (result.has_value()) use *result
//   else handle result.error()
//
// .value() throws std::bad_expected_access — treat it as banned.
// operator* is UB on error — the .has_value() check prevents this.

// --- ensure (always-on assert) ---
//
// ensure() is the primary assertion mechanism. It survives release
// builds regardless of NDEBUG. __builtin_trap() generates a hardware
// trap — immediate crash, no unwind, no exception. The debugger
// catches it at the exact line.
//
// Use ensure() for all TigerStyle assertions.
// Use assert() only for debug-only checks you want stripped in release.

#define ensure(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "ensure failed: %s\n  %s:%d\n", \
                #cond, __FILE__, __LINE__); \
            __builtin_trap(); \
        } \
    } while (0)

// --- attempt (error propagation) ---
//
// Unwraps a std::expected value or returns the error to the caller.
// Equivalent to Rust's ? operator.
//
// Uses a clang statement expression (__extension__({...})), which is
// available everywhere Zig cross-compiles to (Zig bundles clang).
// If the project ever leaves Zig's build system, rewrite as a
// two-argument macro for portability.
//
// Usage:
//   auto value = attempt(some_fallible_function());

#define attempt(expr) \
    __extension__({ \
        auto _attempt_result = (expr); \
        if (!_attempt_result.has_value()) \
            return std::unexpected(_attempt_result.error()); \
        std::move(*_attempt_result); \
    })

// --- defer (scope guard) ---
//
// Runs cleanup code when the enclosing scope exits.
// Equivalent to Zig's defer.
//
// Uses four features confined to this definition:
//   1. Class with destructor — runs code at scope exit.
//   2. Lambda — captures arbitrary cleanup code.
//   3. auto — lambda types are unnameable.
//   4. Macro — generates unique variable names per scope.
//
// Call sites are fully traceable: you see the defer,
// you know what runs and when. Control flow is explicit.
//
// Usage:
//   FILE* file = fopen("data.bin", "rb");
//   ensure(file != nullptr);
//   defer(fclose(file));

template<typename F>
struct DeferGuard {
    F func;
    ~DeferGuard() { func(); }
    DeferGuard(const DeferGuard&) = delete;
    DeferGuard& operator=(const DeferGuard&) = delete;
};

template<typename F>
DeferGuard<F> defer_create(F f) { return DeferGuard<F>{f}; }

#define defer_cat_(a, b) a##b
#define defer_cat(a, b) defer_cat_(a, b)
#define defer(code) \
    auto defer_cat(defer_, __LINE__) = defer_create([&]() { code; })

// --- Compile-time helpers ---

consteval bool is_power_of_two(u64 value) {
    return value > 0 && (value & (value - 1)) == 0;
}

// --- Platform constants ---

constexpr u32 cache_line_bytes = 64;
constexpr u32 page_bytes       = 4096;

// --- Compile-time verification ---

static_assert(sizeof(f32) == 4, "f32 must be 4 bytes.");
static_assert(sizeof(f64) == 8, "f64 must be 8 bytes.");
