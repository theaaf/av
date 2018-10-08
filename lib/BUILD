filegroup(
    name = "sources",
    srcs = glob(["*.cpp"], exclude=["*_test.cpp"]),
    visibility = ["//visibility:public"],
)

filegroup(
    name = "headers",
    srcs = glob(["*.hpp"], exclude=["*_test.cpp"]),
    visibility = ["//visibility:public"],
)


cc_library(
    name = "lib",
    deps = [
        "@asio//:asio",
        "@ffmpeg//:avformat",
        "@ffmpeg//:swscale",
        "@fmt//:fmt",
        "@rtmpdump//:librtmp",
        "@aws//:aws",
        "@json//:json",
        "@simple_web_server//:lib",
        "//lib/h264:lib",
    ],
    srcs = [":sources"],
    hdrs = [":headers"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    deps = ["//lib:lib", "@com_google_googletest//:gtest_main"],
    srcs = glob(["*_test.cpp", "*_test.hpp"]),
    linkstatic = True,
    tags = ["external"],
    timeout = "moderate",
)
