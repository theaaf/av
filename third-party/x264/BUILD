filegroup(
    name = "headers",
    srcs = [
        "x264.h",
        "x264_config.h"
        ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "x264",
    hdrs = [":headers"],
    srcs = ["libx264.a"],
    includes = ["."],
    visibility = ["//visibility:public"],
)

genrule(
    name = "x264-build",
    srcs = glob(["**/*"], exclude=["bazel-*"]) + ["@nasm//:nasm"],
    outs = [
        "x264_config.h",
        "libx264.a",
    ],
    cmd = """
        X264_ROOT=$$(dirname $(location configure))
        AS=$$(pwd)/$(location @nasm//:nasm)
        pushd $$X264_ROOT 
            AS=$$AS ./configure \
                --disable-cli \
                --enable-static \
                --enable-pic \
            && make -j8
        popd
        cp $$X264_ROOT/x264_config.h $(location x264_config.h)
        cp $$X264_ROOT/libx264.a $(location libx264.a)
    """,
    visibility = ["//visibility:public"],
)
