# cpp_template

C++17 project template that uses **Zig** as the build system. Zig handles compilation, linking against libc/libc++, and generating `compile_commands.json` for editor LSP support.

## Prerequisites

- [Zig](https://ziglang.org/download/) >= 0.15.2

## Project Structure

```
.
├── build.zig          # Build configuration (targets, flags, steps)
├── build.zig.zon      # Package metadata and dependencies
├── src/
│   └── main.cpp       # Application entry point
├── include/           # Header files (added to include path automatically)
└── test/              # Test sources
```

## Usage

```bash
# Build
zig build

# Build and run
zig build run

# Run tests
zig build test

# Pass args to the program
zig build run -- arg1 arg2
```

## Editor/LSP Setup

Generate `compile_commands.json` so clangd knows about your files and flags:

```bash
zig build cdb
```

This writes `compile_commands.json` to the project root. Re-run it after adding new source files.

## Sanitizers

Enable AddressSanitizer/UndefinedBehaviorSanitizer:

```bash
zig build run -Dsanitize=address
zig build run -Dsanitize=undefined
```

## Adding Source Files

1. Add `.cpp` files to `src/`
2. Register them in `build.zig` under `exe.root_module.addCSourceFiles` (and `test_exe` if needed)
3. Re-run `zig build cdb` to update `compile_commands.json`

## Using as a Template

```bash
gh repo create my-project --template 0xGlitchbyte/cpp_template
```

After cloning, regenerate the package fingerprint:

```bash
# Delete the .fingerprint field in build.zig.zon, then:
zig build
# Zig will generate a new fingerprint automatically
```
