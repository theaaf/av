cc_library(
    name = "lib",
    deps = [
        "@asio//:asio",
        "@ffmpeg//:avformat",
        "@fmt//:fmt",
        "@rtmpdump//:librtmp",
        "@aws//:aws",
        "@json//:json",
        "//lib/h264:lib",
    ],
    srcs = glob(["*.cpp"], exclude=["*_test.cpp"]),
    hdrs = glob(["*.hpp"], exclude=["*_test.cpp"]),
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    deps = ["//lib:lib", "@com_google_googletest//:gtest_main"],
    srcs = glob(["*_test.cpp", "*_test.hpp"]),
    linkstatic = True,
    tags = ["external"],
    timeout = "short",
)
