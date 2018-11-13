filegroup(
    name = "headers",
    srcs = glob(["*.hpp"]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "fastmemory",
    srcs = glob(["*.cpp"], exclude = ["*_test.cpp"]),
    hdrs = [":headers"],
    visibility = ["//visibility:public"],
    deps = ["@decklink//:DeckLinkAPIDispatch", "@decklink//:DVPAPI", "@decklink//:cuda"],
)

cc_test(
    name = "test",
    deps = ["//lib/fastmemory:fastmemory", "@com_google_googletest//:gtest_main"],
    srcs = glob (["**/*_test.cpp",]),
    linkstatic = True,
    timeout = "short",
)