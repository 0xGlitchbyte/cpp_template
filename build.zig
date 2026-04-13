const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const sanitize = b.option(std.zig.SanitizeC, "sanitize", "Enable c/c++ sanitizer");

    const cflags: []const []const u8 = &.{
        "-std=c++23",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
    };

    const exe = b.addExecutable(.{
        .name = "c_cpp_template",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
            .sanitize_c = sanitize,
        }),
    });

    exe.root_module.addIncludePath(b.path("include"));

    exe.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{"main.cpp"},
        .flags = cflags,
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the program");
    run_step.dependOn(&run_cmd.step);

    // Tests
    const test_exe = b.addExecutable(.{
        .name = "c_cpp_template_test",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
            .sanitize_c = sanitize,
        }),
    });

    test_exe.root_module.addIncludePath(b.path("include"));

    test_exe.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{
            "main.cpp",
        },
        .flags = cflags,
    });

    b.installArtifact(test_exe);

    const test_cmd = b.addRunArtifact(test_exe);
    test_cmd.step.dependOn(b.getInstallStep());
    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&test_cmd.step);

    const cdb_step = b.step("cdb", "Generate compile_commands.json");

    const src_files = [_][]const u8{
        "src/main.cpp",
    };
    const test_files = [_][]const u8{
        "test/main.cpp",
    };

    var cdb_content: std.ArrayListUnmanaged(u8) = .empty;
    const writer = cdb_content.writer(b.allocator);

    writer.writeAll("[\n") catch @panic("OOM");

    var first = true;
    for (src_files ++ test_files) |file| {
        if (!first) writer.writeAll(",\n") catch @panic("OOM");
        first = false;
        writer.print(
            \\  {{
            \\    "directory": ".",
            \\    "file": "{s}",
            \\    "arguments": ["clang++", "-std=c++23", "-Wall", "-Wextra", "-Wpedantic", "-Iinclude", "{s}"]
            \\  }}
        , .{ file, file }) catch @panic("OOM");
    }

    writer.writeAll("\n]\n") catch @panic("OOM");

    const update = b.addUpdateSourceFiles();
    update.addBytesToSource(cdb_content.items, "compile_commands.json");
    cdb_step.dependOn(&update.step);
}
