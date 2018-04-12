config_setting(
    name = "darwin",
    constraint_values = ["@bazel_tools//platforms:osx"],
)

cc_library(
    name = "avcodec",
    deps = [":avutil"],
    hdrs = glob(["libavcodec/*.h"]) + ["config.h"],
    srcs = ["libavcodec.a"],
    linkopts = select({
        ":darwin": ["-liconv"],
        "//conditions:default": [],
    }),
    includes = ["."],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "avformat",
    deps = [":avcodec"],
    hdrs = glob(["libavformat/*.h"]) + ["config.h"],
    srcs = ["libavformat.a"],
    linkopts = ["-lz"],
    includes = ["."],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "avutil",
    hdrs = glob(["libavutil/*.h"]) + ["config.h", "libavutil/avconfig.h"],
    srcs = ["libavutil.a"],
    linkopts = select({
        ":darwin": ["-framework CoreVideo"],
        "//conditions:default": [],
    }),
    includes = ["."],
    visibility = ["//visibility:public"],
)

genrule(
    name = "ffmpeg-build",
    srcs = glob(["**/*"], exclude=["bazel-*"]) + ["@yasm//:yasm"],
    outs = [
        "config.h",
        "libavcodec.a",
        "libavformat.a",
        "libavutil.a",
        "libavutil/avconfig.h",
    ],
    cmd = """
        FFMPEG_ROOT=$$(dirname $(location configure))
        YASM=$$(pwd)/$(location @yasm//:yasm)
        pushd $$FFMPEG_ROOT 
            ./configure \
                --x86asmexe=$$YASM \
                --disable-everything \
                --disable-programs \
                --disable-doc \
                --disable-swscale \
                --disable-swresample \
                --enable-muxer=mpegts \
                --enable-encoder=aac \
                --enable-bsf=h264_mp4toannexb \
            && make
        popd
        cp $$FFMPEG_ROOT/config.h $(location config.h)
        cp $$FFMPEG_ROOT/libavcodec/libavcodec.a $(location libavcodec.a)
        cp $$FFMPEG_ROOT/libavformat/libavformat.a $(location libavformat.a)
        cp $$FFMPEG_ROOT/libavutil/libavutil.a $(location libavutil.a)
        cp $$FFMPEG_ROOT/libavutil/avconfig.h $(location libavutil/avconfig.h)
    """,
)
