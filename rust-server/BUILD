load("@crates//:defs.bzl", "all_crate_deps")
load("@rules_rust//rust:defs.bzl", "rust_binary")

rust_binary(
    name = "server",
    srcs = ["src/main.rs", "src/server.rs"],
    deps = all_crate_deps(normal = True),
    proc_macro_deps = all_crate_deps(proc_macro = True),
    rustc_flags = select({
        "//:release": [
            "-Copt-level=3",
            "-Ccodegen-units=1",
            "-Cpanic=abort",
        ],
        "//conditions:default": [
            "-Copt-level=0",
        ],
    }),
    visibility = ["//visibility:public"],
)