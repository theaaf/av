filegroup(
    name = "headers",
    srcs = glob(["*.hpp"], exclude=["*_test.cpp"]),
    visibility = ["//visibility:public"],
)

genrule(
    name = "headers_prefixed",
    srcs = [":headers"],
    outs = [
        "h264/h264.hpp",
        "h264/seq_parameter_set.hpp",
        "h264/bitstream.hpp",
        "h264/error.hpp",
        "h264/nal_unit.hpp",
    ],
    cmd = """
        cp $(locations headers) $$(dirname $(location h264/h264.hpp))
    """,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "lib",
    srcs = glob(["*.cpp", "*.hpp"], exclude=["*_test.cpp"]),
    hdrs = [":headers"],
    includes = ["."],
    include_prefix = "h264",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    deps = ["//lib/h264:lib", "@com_google_googletest//:gtest_main"],
    srcs = glob(["**/*_test.cpp", "**/*_test.hpp"]),
    linkstatic = True,
    timeout = "short",
)
