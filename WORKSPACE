workspace(name = "av")

load('@bazel_tools//tools/build_defs/repo:git.bzl', 'new_git_repository')

http_archive(
    name = "bazel_compilation_database",
    url = "https://github.com/grailbio/bazel-compilation-database/archive/0.2.3.tar.gz",
    sha256 = "23372eaeac4306c6885a10270b6387c8cc06826736b117821391fb5c0983255b",
    strip_prefix = "bazel-compilation-database-0.2.3",
)

http_archive(
    name = "decklink",
    url = "https://github.aaf.cloud/av/Decklink-SDK/archive/bazel-0.7.tar.gz",
    sha256 = "13850a92cdae0391f1ccb1f268ba78da7b8888d8e7858f11ab8919810fd70e36",
    strip_prefix = "Decklink-SDK-bazel-0.7",
)

new_http_archive(
    name = "rtmpdump",
    urls = ["https://github.aaf.cloud/av/rtmpdump/archive/5a8cb962157aa5c7ecd5dbfda9d1a62c4f3e4e8f.tar.gz"],
    sha256 = "29e8e76e8629f8dbd80b41c7042e35124d8255125df0dfb597e21563eec578bd",
    strip_prefix = "rtmpdump-5a8cb962157aa5c7ecd5dbfda9d1a62c4f3e4e8f",
    build_file_content = """
filegroup(
    name = "headers",
    srcs = glob(["librtmp/*.h"]),
    visibility = ["//visibility:public"],
)
cc_library(
    name = "librtmp",
    hdrs = [":headers"],
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
filegroup(
    name = "headers",
    srcs = glob(["include/**/*.hpp", "include/**/*.ipp"]),
    visibility = ["//visibility:public"],
)
cc_library(
    name = "asio",
    hdrs = [":headers"],
    includes = ["include"],
    visibility = ["//visibility:public"],
    defines = ["ASIO_STANDALONE=1"],
)
""",
)

new_http_archive(
    name = "simple_web_server",
    urls = ["https://github.com/eidheim/Simple-Web-Server/archive/bd9c1192bba119ca34eb01e2ad1e0d979c5514c5.tar.gz"],
    sha256 = "cda4d9a9ba120424d91b11247f087e400a020a9e057d754966d0df4b0d8049d8",
    strip_prefix = "Simple-Web-Server-bd9c1192bba119ca34eb01e2ad1e0d979c5514c5",
    build_file_content = """
filegroup(
    name = "headers",
    srcs = glob(["*.hpp"]),
    visibility = ["//visibility:public"],
)
cc_library(
    name = "lib",
    hdrs = [":headers"],
    includes = ["."],
    include_prefix = "simple-web-server",
    defines = ["USE_STANDALONE_ASIO=1"],
    visibility = ["//visibility:public"],
)
genrule(
    name = "headers_prefixed",
    srcs = [":headers", "server_http.hpp"],
    outs = ["simple-web-server/server_http.hpp"],
    cmd = '''
        cp $(locations headers)  $$(dirname $(location simple-web-server/server_http.hpp))
    ''',
    visibility = ["//visibility:public"],
)
""",
)

new_http_archive(
    name = "fmt",
    urls = ["https://github.com/fmtlib/fmt/archive/4.1.0.tar.gz"],
    sha256 = "46628a2f068d0e33c716be0ed9dcae4370242df135aed663a180b9fd8e36733d",
    strip_prefix = "fmt-4.1.0",
    build_file_content = """
filegroup(
    name = "headers",
    srcs = glob(["fmt/*.h"]),
    visibility = ["//visibility:public"],
)
cc_library(
    name = "fmt",
    srcs = glob(["fmt/*.cc"]),
    hdrs = [":headers"],
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
filegroup(
    name = "headers",
    srcs = glob(["args.hxx"]),
    visibility = ["//visibility:public"],
)
cc_library(
    name = "args",
    hdrs = [":headers"],
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
filegroup(
    name = "headers",
    srcs = glob(["include/nlohmann/**/*.hpp"]),
    visibility = ["//visibility:public"],
)
cc_library(
    name = "json",
    hdrs = [":headers"],
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
    urls = ["https://github.com/aws/aws-sdk-cpp/archive/1.6.21.tar.gz"],
    sha256 = "c3be0ee3ed3cf3ee5bd94f478fa6612d1557edd56d9d177f1b0aa39d4e8aabc4",
    strip_prefix = "aws-sdk-cpp-1.6.21",
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
    name = "nasm",
    url = "https://www.nasm.us/pub/nasm/releasebuilds/2.13.03/nasm-2.13.03.tar.gz",
    sha256 = "23e1b679d64024863e2991e5c166e19309f0fe58a9765622b35bd31be5b2cc99",
    strip_prefix = "nasm-2.13.03",
    build_file = "third-party/nasm/BUILD",
)

new_http_archive(
    name = "x264",
    url = "https://s3.amazonaws.com/aaf-av-public-mirrors/x264-snapshot-20180418-2245-stable.tar.bz2",
    sha256 = "a492452bb35a3cb8eb27b53ef41ac32e3876366625d6399922970932333df372",
    strip_prefix = "x264-snapshot-20180418-2245-stable",
    build_file = "third-party/x264/BUILD",
)

new_http_archive(
    name = "x265",
    url = "http://ftp.videolan.org/pub/videolan/x265/x265_2.8.tar.gz",
    sha256 = "6e59f9afc0c2b87a46f98e33b5159d56ffb3558a49d8e3d79cb7fdc6b7aaa863",
    strip_prefix = "x265_2.8",
    build_file = "third-party/x265/BUILD",
)

new_http_archive(
    name = "clang_osx",
    url = "http://releases.llvm.org/6.0.0/clang+llvm-6.0.0-x86_64-apple-darwin.tar.xz",
    sha256 = "0ef8e99e9c9b262a53ab8f2821e2391d041615dd3f3ff36fdf5370916b0f4268",
    strip_prefix = "clang+llvm-6.0.0-x86_64-apple-darwin",
    build_file_content = """
filegroup(
    name = "clang_check",
    srcs = ["bin/clang-check"],
    visibility = ["//visibility:public"],
)
    """
)

new_http_archive(
    name = "clang_linux",
    url = "http://releases.llvm.org/7.0.0/clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz",
    sha256 = "69b85c833cd28ea04ce34002464f10a6ad9656dd2bba0f7133536a9927c660d2",
    strip_prefix = "clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04",
    build_file_content = """
filegroup(
    name = "clang_check",
    srcs = ["bin/clang-check"],
    visibility = ["//visibility:public"],
)
    """
)

new_http_archive(
    name = "cmake_osx",
    url = "https://cmake.org/files/v3.12/cmake-3.12.3-Darwin-x86_64.tar.gz",
    sha256 = "b3bb06382a1ecdf8dbd26cf782e5fa3c2aaface0c1376f6d53d24be8aec324d2",
    strip_prefix = "cmake-3.12.3-Darwin-x86_64/CMake.app/Contents",
    build_file_content = """
filegroup(
    name = "cmake",
    srcs = ["bin/cmake"],
    visibility = ["//visibility:public"],
)
    """
)

new_http_archive(
    name = "cmake_linux",
    url = "https://cmake.org/files/v3.12/cmake-3.12.3-Linux-x86_64.tar.gz",
    sha256 = "0210f500c71af0ee7e8c42da76954298144d5f72f725ea381ae5db7b766b000e",
    strip_prefix = "cmake-3.12.3-Linux-x86_64",
    build_file_content ="""
filegroup(
    name = "cmake",
    srcs = ["bin/cmake"],
    visibility = ["//visibility:public"],
)
    """
)