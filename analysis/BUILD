load('@bazel_compilation_database//:aspects.bzl', 'compilation_database')

compilation_database(
    name = "compile_commands_json",
    targets = [
        "//ingest-server",
    ],
    exec_root = "DUMMYROOT",
)

config_setting(
    name = "clang_osx",
    constraint_values = [
        "@bazel_tools//platforms:osx",
    ],
)

config_setting(
    name = "clang_linux",
    constraint_values = [
        "@bazel_tools//platforms:linux",
    ],
)

sh_library(
    name = "analyze_lib",
    data = [
        "compile_commands.json",
        "//ingest-server:sources",
        "//lib:sources",
        "//lib:headers",
        "//lib:lib",
        "@aws//:headers",
        "@asio//:headers",
        "@args//:headers",
        "@json//:headers",
        "@simple_web_server//:headers",
        "@simple_web_server//:headers_prefixed",
        "@fmt//:headers",
        "@openssl//:headers",
        "@rtmpdump//:headers",
        "@ffmpeg//:ffmpeg-build",
        "@ffmpeg//:headers",
        "//lib/h26x:headers_prefixed",
    ] + select({
        ":clang_osx":  ["@clang_osx//:clang_check"],
        ":clang_linux":["@clang_linux//:clang_check"]
    }),
)

sh_binary(
    name = "clang-check",
    srcs = ["clang-checker.sh"],
    deps = [":analyze_lib",],
)

sh_binary(
    name = "clang-tidy",
    srcs = ["clang-tidy.sh"],
    deps = [":analyze_lib"],
)