load('@bazel_tools//tools/build_defs/repo:git.bzl', 'new_git_repository')

new_http_archive(
    name = "rtmpdump",
    urls = ["https://github.aaf.cloud/av/rtmpdump/archive/5a8cb962157aa5c7ecd5dbfda9d1a62c4f3e4e8f.tar.gz"],
    sha256 = "29e8e76e8629f8dbd80b41c7042e35124d8255125df0dfb597e21563eec578bd",
    strip_prefix = "rtmpdump-5a8cb962157aa5c7ecd5dbfda9d1a62c4f3e4e8f",
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
    urls = [
        "https://s3.amazonaws.com/aaf-av-public-mirrors/asio-1.10.6.tar.gz",
        "https://sourceforge.net/projects/asio/files/asio/1.10.6%20%28Stable%29/asio-1.10.6.tar.gz"
    ],
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

new_http_archive(
    name = "args",
    urls = ["https://github.com/Taywee/args/archive/6.2.0.tar.gz"],
    sha256 = "8e374ae0efeab6134a7d6b205903f79e3873e6b99a3ae284833931140a5ba1df",
    strip_prefix = "args-6.2.0",
    build_file_content = """
cc_library(
    name = "args",
    hdrs = glob(["args.hxx"]),
    includes = ["."],
    visibility = ["//visibility:public"],
)
""",
)

new_http_archive(
    name = "json",
    urls = ["https://github.com/nlohmann/json/releases/download/v3.1.2/include.zip"],
    sha256 = "495362ee1b9d03d9526ba9ccf1b4a9c37691abe3a642ddbced13e5778c16660c",
    build_file_content = """
cc_library(
    name = "json",
    hdrs = glob(["include/nlohmann/**/*.hpp"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)
""",
)

new_http_archive(
    name = "openssl",
    url = "https://github.com/openssl/openssl/archive/OpenSSL_1_1_0h.tar.gz",
    sha256 = "f56dd7d81ce8d3e395f83285bd700a1098ed5a4cb0a81ce9522e41e6db7e0389",
    strip_prefix = "openssl-OpenSSL_1_1_0h",
    build_file = "third-party/openssl/BUILD",
)

http_archive(
    name = "com_google_googletest",
    url = "https://github.com/google/googletest/archive/82febb8eafc0425601b0d46567dc66c7750233ff.tar.gz",
    sha256 = "306bfa9e5849804ae4a8b4046417412b97f79d9cd67740ba3adf7a0f985260ce",
    strip_prefix = "googletest-82febb8eafc0425601b0d46567dc66c7750233ff",
)

new_http_archive(
    name = "aws",
    urls = ["https://github.com/aws/aws-sdk-cpp/archive/1.4.30.tar.gz"],
    sha256 = "54097ad7ece5e87f628864dd33d1760b0a4d0a4920b1f431871c7a6d6b8dd8ca",
    strip_prefix = "aws-sdk-cpp-1.4.30",
    build_file = "third-party/aws/BUILD",
)

new_http_archive(
    name = "ffmpeg",
    url = "https://github.com/FFmpeg/FFmpeg/archive/n3.4.2.tar.gz",
    sha256 = "d079c68dc19a0223239a152ffc2b67ef1e9d3144e4d2c2563380dc59dccf33e5",
    strip_prefix = "FFmpeg-n3.4.2",
    build_file = "third-party/ffmpeg/BUILD",
)

new_http_archive(
    name = "yasm",
    url = "http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz",
    sha256 = "3dce6601b495f5b3d45b59f7d2492a340ee7e84b5beca17e48f862502bd5603f",
    strip_prefix = "yasm-1.3.0",
    build_file = "third-party/yasm/BUILD",
)
