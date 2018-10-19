filegroup(
    name = "headers",
    srcs = glob(["*.hpp"], exclude=["*_test.cpp"]),
    visibility = ["//visibility:public"],
)

genrule(
    name = "headers_prefixed",
    srcs = [":headers"],
    outs = [
        "h26x/h264.hpp",
        "h26x/h265.hpp",
        "h26x/seq_parameter_set.hpp",
        "h26x/bitstream.hpp",
        "h26x/error.hpp",
        "h26x/nal_unit.hpp",
    ],
    cmd = """
        cp $(locations headers) $$(dirname $(location h26x/h264.hpp))
    """,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "lib",
    srcs = glob(["*.cpp", "*.hpp"], exclude=["*_test.cpp"]),
    hdrs = [":headers"],
    includes = ["."],
    include_prefix = "h26x",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    deps = ["//lib/h26x:lib", "@com_google_googletest//:gtest_main"],
    srcs = glob(["**/*_test.cpp", "**/*_test.hpp"]),
    linkstatic = True,
    timeout = "short",
)
