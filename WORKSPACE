load('@bazel_tools//tools/build_defs/repo:git.bzl', 'new_git_repository')

new_git_repository(
    name = "rtmpdump",
    remote = "git@github.aaf.cloud:av/rtmpdump.git",
    commit = "5a8cb962157aa5c7ecd5dbfda9d1a62c4f3e4e8f",
    build_file_content = """
cc_library(
    name = "librtmp",
    hdrs = glob(["librtmp/*.h"]),
    srcs = glob(["librtmp/*.c"]),
    includes = ["."],
    visibility = ["//visibility:public"],
    defines = ["NO_CRYPTO=1"],
)
""",
)

new_http_archive(
    name = "asio",
    urls = ["https://sourceforge.net/projects/asio/files/asio/1.10.6%20%28Stable%29/asio-1.10.6.tar.gz"],
    sha256 = "70345ca9e372a119c361be5e7f846643ee90997da8f88ec73f7491db96e24bbe",
    strip_prefix = "asio-1.10.6",
    build_file_content = """
cc_library(
    name = "asio",
    hdrs = glob(["include/**/*.hpp", "include/**/*.ipp"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    defines = ["ASIO_STANDALONE=1"],
)
""",
)

new_http_archive(
    name = "fmt",
    urls = ["https://github.com/fmtlib/fmt/archive/4.1.0.tar.gz"],
    sha256 = "46628a2f068d0e33c716be0ed9dcae4370242df135aed663a180b9fd8e36733d",
    strip_prefix = "fmt-4.1.0",
    build_file_content = """
cc_library(
    name = "fmt",
    srcs = glob(["fmt/*.cc"]),
    hdrs = glob(["fmt/*.h"]),
    includes = ["."],
    visibility = ["//visibility:public"],
)
""",
)

http_archive(
    name = "com_google_googletest",
    url = "https://github.com/google/googletest/archive/82febb8eafc0425601b0d46567dc66c7750233ff.tar.gz",
    sha256 = "306bfa9e5849804ae4a8b4046417412b97f79d9cd67740ba3adf7a0f985260ce",
    strip_prefix = "googletest-82febb8eafc0425601b0d46567dc66c7750233ff",
)
