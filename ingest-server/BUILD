filegroup(
    name = "sources",
    srcs = ["main.cpp"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "ingest-server",
    srcs = [":sources"],
    deps = [
        "//lib",
        "@args//:args",
    ],
    visibility = ["//visibility:public"],
)
