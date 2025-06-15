load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_library(
    name = "server-lib",
    hdrs = ["server.h"],
    copts = select({
        "//:release": [
            "-O3",
            "-fomit-frame-pointer",
            "-fno-strict-aliasing",
        ],
        "//conditions:default": [
            "-O0",
            "-Wall",
            "-Wextra",
            "-g",
        ],
    }),
)

cc_binary(
    name = "server",
    srcs = ["server.c"],
    deps = [":server-lib"],
    copts = select({
        "//:release": [
            "-O3",
            "-fomit-frame-pointer",
            "-fno-strict-aliasing",
        ],
        "//conditions:default": [
            "-O0",
            "-Wall",
            "-Wextra",
            "-g",
        ],
    }),
    visibility = ["//visibility:public"],
)