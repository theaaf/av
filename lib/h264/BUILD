cc_library(
    name = "lib",
    srcs = glob(["*.cpp", "*.hpp"], exclude=["*_test.cpp"]),
    hdrs = glob(["*.hpp"], exclude=["*_test.cpp"]),
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
