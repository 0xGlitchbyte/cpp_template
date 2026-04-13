# C++ Style Guide

The guiding philosophy is **TigerStyle** (from TigerBeetle: https://github.com/tigerbeetle/tigerbeetle/blob/main/docs/TIGER_STYLE.md), adapted for C++.

## Design Goals (in order)

1. Safety
2. Performance
3. Developer experience

## Design Philosophy

- Simplicity is the hardest revision. The first attempt is easy; the simple version takes multiple passes. Spend mental energy upfront in design — an hour of design is worth weeks in production.
- Use a minimum of excellent abstractions. Every abstraction risks being leaky. Use only the ones that make the best sense of the domain.
- Assertions encode understanding. Build a precise mental model of the code first. Encode that model as assertions. Write code and comments to justify the model to a reviewer. Fuzzing is the final line of defense, not the first.

---

## Style Rules

### Memory Model
- Statically allocate all memory at startup — fixed-size arrays, preallocated buffers.
- Use arena allocators for grouped lifetime allocations where static sizing requires runtime information.
- Reserve `unique_ptr` for the rare case where dynamic allocation is unavoidable, justified in a comment explaining the constraint.

### Language Features

**Data & Functions:**
- Structs for all data types
- Free functions for all logic
- `const` on every binding by default
- `enum class` for type-safe enumerations
- `constexpr` for compile-time constants, `consteval` for guaranteed compile-time-only evaluation
- `static_assert` for compile-time invariant verification

**Ownership & References:**
- Pointers for structs over 16 bytes — makes indirection visible at the call site
- References for types 16 bytes or under where nullability is irrelevant
- `explicit` on single-argument constructors — all conversions are intentional
- `= delete` on copy constructor/assignment for resource-owning types — all copies are intentional
- Disambiguate function parameters. When two parameters share the same type, wrap them in a struct to name them. A function taking two `u32` arguments is a bug waiting to happen — the caller can swap them silently.

**Error Handling:**
- `std::expected<T, E>` for functions that return a value or an error (C++23, equivalent to Rust's `Result<T, E>`)
- `std::optional<T>` for value-or-nothing semantics (equivalent to Rust's `Option<T>`, Zig's `?T`)
- `Error` is an `enum class : u32` per project — the compiler catches typos and invalid error codes
- Errors are operational (expected, handled by caller). Assertions catch programmer errors (unexpected, crash immediately).
- `attempt()` macro for error propagation — unwraps the value or returns the error to the caller (equivalent to Rust's `?` operator)
- No exceptions. Exceptions create invisible control flow, require runtime support (stack unwinding, RTTI), and cannot be traced by reading code top to bottom. `std::expected` and `ensure()` cover operational errors and programmer errors respectively.

**`std::expected` Access Pattern:**
- Always check `.has_value()` before accessing the value via `operator*`.
- `.value()` throws `std::bad_expected_access` — treat it as banned.
- `operator*` is UB on error — the `.has_value()` check prevents this.
- Monadic operations (`.and_then()`, `.transform()`, `.or_else()`) are permitted for chaining. These take callables.

**Assertions:**
- `ensure()` for all TigerStyle assertions. Always on, survives release builds, crashes immediately via `__builtin_trap()`.
- `assert()` from `<cassert>` is controlled by `NDEBUG` and disabled in release builds. Use only for debug-only checks you genuinely want stripped.
- Default to `ensure()` for everything. `assert()` is the exception, not the rule.

**Safety Attributes:**
- `[[nodiscard]]` on all error-returning functions — compiler warns on unhandled return values
- `[[noreturn]]` on functions that abort or panic
- `[[maybe_unused]]` when intentionally ignoring a value

**Output & Diagnostics:**
- `std::println` for formatted output — type-safe (C++23)
- `__builtin_unreachable()` to mark code paths that must never execute

**Views & References:**
- `std::string_view` for borrowed string data (equivalent to Rust's `&str`)
- `std::span<T>` for borrowed contiguous data (equivalent to Rust's `&[T]`)
- `std::byte` for raw memory distinct from integers

**Aliases & Generics:**
- `using` for type aliases
- Namespaces for scoping
- Function overloading for same-operation-different-type convenience
- Simple generic structs via templates where fully transparent

**Scope Management:**
- `defer()` macro for scope-exit cleanup (equivalent to Zig's `defer`)

### Governing Principle

Write explicit code. Use lambdas, templates, and monadic operations only where they improve safety or performance over the plain alternative. Application code stays plain. Infrastructure (`base.h`) and error chaining (`std::expected` monadic ops) are the justified uses.

The test for any feature: **can you trace every line of execution by reading the code top to bottom?**

The C++ standard library is used for **vocabulary types** — `std::optional`, `std::expected`, `std::unique_ptr`, `std::move`, `std::string_view`, `std::span`, `std::byte`, `std::println`. STL **containers** (`vector`, `map`) and **algorithms** (`<algorithm>`, `<numeric>`) are replaced by hand-written, statically-allocated equivalents where the data layout and bounds are fully visible.

### `base.h` Exceptions

`base.h` contains four macros (`ensure`, `defer`, `attempt`, and their helpers) that use features reserved only for these definitions: a class with destructor, lambdas, `auto`, macros, and a clang statement expression. All are confined to `base.h`. Call sites are one traceable line each.

### Naming Convention (Rust-style)

| Category | Convention | Example |
|---|---|---|
| Structs | PascalCase | `RingBuffer` |
| Enum class | PascalCase | `Error` |
| Enum variants | PascalCase | `Error::BufferFull` |
| Functions | snake_case | `ring_buffer_push` |
| Variables | snake_case | `count_previous` |
| Constants | snake_case | `buffer_capacity` |
| Type aliases | PascalCase | `PopResult` |
| Namespaces | snake_case | `ring_buffer` |
| Macros | snake_case | `ensure`, `defer`, `attempt` |

Additional naming rules:
- Get the nouns and verbs right. Take time to find the perfect name. Great names capture what a thing is or does and provide a crisp mental model.
- Full words — abbreviations are reserved for loop indices in sort functions or matrix calculations.
- Readable capitalization for acronyms: `HttpServer`, `VsrState` — not `HTTPServer`, `VSRState`.
- Units and qualifiers last, sorted by descending significance: `latency_ms_max`.
- Related names share character count for visual alignment: `source`/`target` over `src`/`dest`.
- Helper functions prefixed with calling function name: `read_sector_callback`.
- Callbacks go last in the parameter list. Mirrors invocation order.
- One name, one meaning. Names carry a single meaning across the entire codebase, independent of context.
- Infuse names with meaning. `arena` over `allocator` when the allocation strategy matters. The name tells the reader how the resource behaves.
- Nouns over participles for identifiers used outside code. `pipeline` over `preparing` — nouns compose clearly as section headers and in conversation.

### Struct Ordering
Fields first, types second, functions third. Order by access frequency. Important things near the top.

### Limits
- 70 lines per function (hard limit — test against real code, raise if consistently broken by well-structured functions). Ideal function shape is the inverse hourglass: few parameters, simple return type, meaty logic between the braces.
- When splitting functions: centralize state manipulation. The parent function keeps relevant state in local variables. Helpers compute what needs to change and return the delta. The parent applies it. Leaf functions stay pure.
- 100 columns per line (hard limit)
- Zero dependencies beyond the C++ standard library
- Zero technical debt
- Minimize tools. Standardize on Zig for build, scripts, and automation. The right tool is often the tool you already use — adding tools has higher cost than people appreciate.
- 4 spaces indentation
- Braces on all `if` statements unless single line
- Use `clang-format` with a project `.clang-format` config enforcing the style rules (100 columns, 4 spaces, brace style). Automate formatting so it requires zero thought.

### Pointers vs Copies

- Default to immutable/copied. Mutate only when the function's purpose requires it.
- A function that **changes something** receives a pointer to the original.
- A function that **reads or computes from something** receives a copy (or const pointer if large).
- The function's purpose determines the signature. Purpose first, signature follows.
- Structs over 16 bytes pass as `const*` — indirection is visible at the call site.

---

## TigerStyle Principles

**Control Plane vs Data Plane:**
- Separate validation from processing. The control plane validates batch boundaries, checks invariants, handles errors. The data plane processes items with minimal branching.
- Assertions are dense in the control plane — validate once per batch. The data plane relies on pre-validated bounds and the type system instead of runtime checks.
- This is how high assertion density coexists with high performance. Two checks per batch, not one check per item.

**Assertions:**
- Minimum two `ensure()` calls per function — preconditions, postconditions, or invariants.
- Assert the **positive space** (what you expect) AND the **negative space** (what must never happen). The boundary between valid and invalid is where bugs live.
- **Pair assertions** — enforce the same property at two different code paths. Assert validity before writing AND after reading.
- Split compound assertions: `ensure(a); ensure(b);` over `ensure(a && b);`. Precise failure info.
- Use single-line `if` to assert implications: `if (a) ensure(b);`.
- Use blatantly true assertions as documentation where the condition is critical and surprising. `ensure(buffer_capacity > 0)` is stronger than a comment because it is machine-checked.

**Control Flow:**
- Put a limit on everything. Loops, queues, buffers, retries, timeouts, batch sizes — if it can grow, it has a cap. If it can repeat, it has a max.
- Every loop has a fixed upper bound. Bounded loops guarantee termination.
- No recursion. All executions that should be bounded must be bounded. Use explicit iteration with a stack if tree/graph traversal is needed.
- State invariants positively: `if (index < length)` over `if (index >= length)`.
- Split compound conditions into nested `if/else` branches — each branch is clear.
- Consider whether a standalone `if` needs a matching `else`. If the `if` handles the positive space, the `else` handles or asserts the negative space. A lone `if` with no `else` is correct only when the negative space genuinely requires no action.
- Push `if`s up, `for`s down. Centralize control flow in parent functions, keep leaf functions pure.
- Run at your own pace. When interacting with external entities, the program drives its own control flow. React to events in batches on your schedule, not one-at-a-time on theirs. This keeps control flow bounded and enables batching.

**Memory Safety:**
- Zero all buffers on init and after read — prevents buffer bleeds (stale data leaks).
- Smallest possible scope for all variables. Calculate and check variables close to where they're used. A gap in time or space between checking a value and using it is where bugs hide (POCPOU — place-of-check to place-of-use, the cousin of TOCTOU).
- In-place initialization via out pointer for large structs. In-place initialization is viral — if any field is initialized in-place, the entire container struct must be initialized in-place too.
- One variable, one source of truth. Duplicating a variable or aliasing it increases the probability of state going out of sync.
- Group resource acquisition and cleanup together. Place `defer()` immediately after the resource it guards, separated by newlines, to make leaks visible at a glance.

**Performance:**
- Amortize network, disk, memory, and CPU costs by batching accesses. Per-item overhead disappears when work is done per-batch.
- Let the CPU sprint. Be predictable. Give the CPU large contiguous chunks of work. Avoid interleaving unrelated operations.
- Extract hot loops into standalone functions with primitive arguments. Pass `u32 count`, `u32 capacity` — not `const RingBuffer*`. The compiler can cache primitives in registers without proving struct fields are unaliased. A human reader can spot redundant computations more easily.
- Prefer power-of-two sizes for buffers and capacities — bitwise ops replace division and modulo (1 cycle vs 20-90 cycles). Enforce with `static_assert(is_power_of_two(...))`.
- When division is necessary, show intent: assert exact divisibility (`ensure(a % b == 0)`) before dividing, or document whether the result truncates or rounds. Restructure to multiply where possible (`a > threshold * b` over `a / b > threshold`).
- Explicitly pass options to library functions at the call site. Spelling out every argument avoids latent bugs if defaults change upstream, and makes the call site self-documenting.

**Return Types:**
- Simpler return types reduce dimensionality at the call site. `void` is simpler than `bool`, `bool` is simpler than `u32`, `u32` is simpler than `std::optional<u32>`, `std::optional` is simpler than `std::expected<u32, Error>`. Each level adds a branch the caller must handle. Use the simplest type that captures the function's contract.

**Documentation:**
- File-level comment explaining goal, methodology, and back-of-envelope design sketch.
- Comments say **why**, with the specific bug or failure mode they prevent.
- Comments are sentences. Capital letter, full stop, space after `//`. End-of-line comments can be phrases without punctuation.
- `main` goes first — forward declarations enable this. Entry point is the most important function.
- Write descriptive commit messages. The commit message is read. PR descriptions are invisible in `git blame`.

**Off-By-One Prevention:**
- Index, count, and size are conceptually distinct types. Index is 0-based, count is 1-based, size is count × unit. Variable names make the distinction clear. Mixing them is the primary source of off-by-one errors.

---

## Testing Strategy

- Tests in `test/` directory, one test file per module.
- Each test function named `test_<module>_<behavior>` — e.g., `test_ring_buffer_push_to_full`.
- Test functions return `void`, use `ensure()` (always-on) for all checks.
- Every test covers both valid input AND invalid input (positive and negative space).
- Each test file and each test function opens with a comment explaining goal and methodology. Helps the reader get oriented or skip sections without diving into implementation.
- `main()` in `test/main.cpp` calls all test functions explicitly. A list of function calls. No test framework, no test discovery.
- Fuzzing: deferred. When needed, write a separate fuzz target per module.

---

## Header/Source Separation

Start with `.cpp` files. Extract a header only when two files need the same declarations. Structure when the code demands it, not preemptively.

Directory structure:
```
include/
    base.h          — portable foundation (header-only, always present)
src/
    main.cpp        — entry point
    ring_buffer.cpp — module (single file until extraction needed)
test/
    main.cpp        — test runner
    ring_buffer.cpp — tests for ring_buffer
```

- `#pragma once` on every header.
- Headers contain: struct definitions, enum definitions, function declarations, constants, static_asserts.
- Sources contain: function definitions.
- Extract a header when a second file needs the same declarations. Split when a file exceeds ~200 lines.
- `include/` can be empty at project start aside from `base.h`.

---

## base.h — Portable Foundation

Reusable across all projects. Contains only what has been needed in two or more contexts.

See [`include/base.h`](include/base.h) for the full implementation.

### Design Rationale

- `ensure()` is the primary assertion. Always on. Crashes via `__builtin_trap()`. Use for all TigerStyle assertions. `assert()` is debug-only and disabled by `NDEBUG` in release — use only when you genuinely want the check stripped.
- `attempt()` propagates errors through `std::expected`. Equivalent to Rust's `?`. Uses a clang statement expression — available everywhere Zig cross-compiles to. Tied to clang; if the project ever leaves Zig, rewrite as a two-argument portable macro.
- `defer()` provides Zig-style scope-exit cleanup. Four style exceptions (class, destructor, lambda, auto, macro) are confined to this definition.
- `std::expected<T, E>` is the standardized error handling type. Access via `.has_value()` + `operator*`. `.value()` throws — treat it as banned.
- `Error` is per-project as an `enum class : u32`. The compiler enforces correctness.
- `is_power_of_two` is `consteval` — guaranteed compile-time only. Usable in `static_assert`.
- `cache_line_bytes` and `page_bytes` verify struct sizing at compile time.
- `<utility>` is included for `std::move` and `std::unexpected`.
- Macro helper names (`defer_cat_`, `defer_cat`) are snake_case per naming convention.

---

## build.zig

Target: Zig 0.15.2. Pure C++ project (no Zig source files). C++23 standard.

Key features:
- `standardTargetOptions` / `standardOptimizeOption` for cross-compilation and optimization
- C++ sanitizer support via `-Dsanitize` option
- Compiler flags: `-std=c++23`, `-Wall`, `-Wextra`, `-Wpedantic`
- `link_libc = true`, `link_libcpp = true` for C++ standard library
- Separate executable targets for `src/` and `test/`
- `compile_commands.json` generation via `zig build cdb` for clangd/Neovim LSP
- `zig build run` to compile and run
- `zig build test` to compile and run tests

### C++23 on Zig 0.15.2

Zig 0.15.2's bundled clang/libc++ supports C++23 with one exception:
- `std::expected<T, E>` — **works**
- `std::println` — **works**
- `std::unreachable()` — **use `__builtin_unreachable()` instead** (clang built-in, always available)

**Known issue:** `compile_commands.json` points clangd at system headers rather than Zig's bundled libc++. This causes false errors in the LSP for C++23 features that exist in Zig's libc++ but are absent from the system's. This is a clangd configuration issue. `zig build` compiles correctly. Fix by getting Zig's internal include paths (visible via `zig c++ -v`) into `compile_commands.json`.

### Zig Build System Notes
- The build system is the least-documented part of Zig. The real documentation is the `std.Build` source code, `zig init` scaffold, and community examples.
- The build API changes across versions. Examples from 6 months ago may be broken.
- `zig init` generates a working `build.zig` scaffold — the intended starting point.

### Compiler Flags
- `-Wall`: most common warnings (uninitialized variables, unused values, implicit fallthrough).
- `-Wextra`: additional warnings `-Wall` misses (unused parameters, sign comparison, empty bodies).
- `-Wpedantic`: warns on anything outside strict ISO C++23 compliance. Keeps code portable.
- Together they catch the broadest set of likely bugs at zero runtime cost.

### compile_commands.json
Required for clangd in Neovim. Generated by the `cdb` step in `build.zig` which writes the file directly using `addUpdateSourceFiles`. Contains compilation flags and file paths so the language server can parse the C++ files.

---

## Back-of-Envelope Performance Sketches

Napkin math before writing code. The grid:

| | Bandwidth | Latency |
|---|---|---|
| **Network** | data/sec throughput | round trip time |
| **Disk** | read/write throughput | seek/fsync time |
| **Memory** | data movement rate | cache miss penalty |
| **CPU** | ops/sec | branch mispredict cost |

### Key Latency Numbers
- L1 cache: 0.5ns
- L2 cache: 7ns
- Main memory: 100ns
- NVMe sequential 1MB: 100μs
- SSD sequential 1MB: 300μs
- Datacenter round trip: 500μs
- HDD sequential 1MB: 6ms
- Disk seek: 10ms

### Application
Optimize for the slowest resource first (network → disk → memory → CPU), adjusted for frequency. A memory cache miss at 100ns happening 1M times outweighs a disk fsync at 10ms happening once. The goal is to land within an order of magnitude — the exponent matters, the coefficient is noise. Design for 90% of the global maximum before writing code.

---

## Reference Example — Ring Buffer

A complete TigerStyle C++ ring buffer exists as a working example demonstrating: static allocation, `static_assert` on constants (power-of-two, cache line fit), bitmask wrapping, `std::expected<T, E>` error handling, `enum class Error`, pair assertions with `ensure()`, bounded loops, in-place initialization via out pointer, buffer bleed protection (zeroing on init and after pop), forward declarations with `main` first, `attempt()` for error propagation, and all TigerStyle formatting rules.
