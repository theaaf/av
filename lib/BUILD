cc_library(
    name = "lib",
    deps = ["@asio//:asio", "@fmt//:fmt", "@rtmpdump//:librtmp", "@aws//:aws"],
    srcs = glob(["**/*.cpp"], exclude=["**/*_test.cpp"]),
    hdrs = glob(["**/*.hpp"]),
    visibility = ["//main:__pkg__"],
)

cc_test(
    name = "test",
    deps = ["//lib:lib", "@com_google_googletest//:gtest_main"],
    srcs = glob(["**/*_test.cpp"]),
)
