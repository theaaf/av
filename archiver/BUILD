filegroup(
    name = "sources",
    srcs = ["main.cpp"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "archiver",
    srcs = [":sources"],
    deps = [
        "//lib",
        "@args//:args",
    ],
    visibility = ["//visibility:public"],
)
