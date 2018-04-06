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
    build_file_content = """
load("@//:build.bzl", "template_rule")

cc_library(
    name = "aws",
    srcs = glob([
        "aws-cpp-sdk-core/include/**/*.h",
        "aws-cpp-sdk-core/source/*.cpp",
        "aws-cpp-sdk-core/source/auth/**/*.cpp",
        "aws-cpp-sdk-core/source/config/**/*.cpp",
        "aws-cpp-sdk-core/source/client/**/*.cpp",
        "aws-cpp-sdk-core/source/external/**/*.cpp",
        "aws-cpp-sdk-core/source/internal/**/*.cpp",
        "aws-cpp-sdk-core/source/http/*.cpp",
        "aws-cpp-sdk-core/source/http/curl/**/*.cpp",
        "aws-cpp-sdk-core/source/http/standard/**/*.cpp",
        "aws-cpp-sdk-core/source/utils/*.cpp",
        "aws-cpp-sdk-core/source/utils/base64/**/*.cpp",
        "aws-cpp-sdk-core/source/utils/json/**/*.cpp",
        "aws-cpp-sdk-core/source/utils/logging/**/*.cpp",
        "aws-cpp-sdk-core/source/utils/memory/**/*.cpp",
        "aws-cpp-sdk-core/source/utils/stream/**/*.cpp",
        "aws-cpp-sdk-core/source/utils/threading/**/*.cpp",
        "aws-cpp-sdk-core/source/utils/xml/**/*.cpp",
        "aws-cpp-sdk-core/source/utils/crypto/*.cpp",
        "aws-cpp-sdk-core/source/utils/crypto/commoncrypto/**/*.cpp",
        "aws-cpp-sdk-core/source/utils/crypto/factory/**/*.cpp",
        "aws-cpp-sdk-core/source/platform/linux-shared/*.cpp",
        "aws-cpp-sdk-s3/include/**/*.h",
        "aws-cpp-sdk-s3/source/**/*.cpp",
    ]),
    hdrs = [
        "aws-cpp-sdk-core/include/aws/core/SDKConfig.h",
    ],
    defines = [
        "ENABLE_CURL_CLIENT",
        "ENABLE_COMMONCRYPTO_ENCRYPTION",
        "AWS_SDK_VERSION_MAJOR=1",
        "AWS_SDK_VERSION_MINOR=4",
        "AWS_SDK_VERSION_PATCH=30",
    ],
    includes = [
        "aws-cpp-sdk-core/include",
        "aws-cpp-sdk-s3/include/",
    ],
    linkopts = ["-lcurl"],
    visibility = ["//visibility:public"],
)

template_rule(
    name = "SDKConfig_h",
    src = "aws-cpp-sdk-core/include/aws/core/SDKConfig.h.in",
    out = "aws-cpp-sdk-core/include/aws/core/SDKConfig.h",
    substitutions = {
        "cmakedefine": "define",
    },
)
""",
)

