config_setting(
    name = "darwin",
    constraint_values = ["@bazel_tools//platforms:osx"],
)

cc_library(
    name = "avcodec",
    deps = [":avutil", "@x264//:x264"],
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

cc_library(
    name = "swscale",
    deps = [":avutil"],
    hdrs = glob(["libswscale/*.h"]) + ["config.h"],
    srcs = ["libswscale.a"],
    includes = ["."],
    visibility = ["//visibility:public"],
)

genrule(
    name = "ffmpeg-build",
    srcs = glob(["**/*"], exclude=["bazel-*"]) + ["@nasm//:nasm", "@x264//:headers", "@x264//:libx264.a"],
    outs = [
        "config.h",
        "libavcodec.a",
        "libavformat.a",
        "libavutil.a",
        "libavutil/avconfig.h",
        "libswscale.a",
    ],
    cmd = """
        FFMPEG_ROOT=$$(dirname $(location configure))
        mkdir $$FFMPEG_ROOT/additional_includes
        cp $(locations @x264//:headers) $$FFMPEG_ROOT/additional_includes
        mkdir $$FFMPEG_ROOT/additional_libs
        cp $(location @x264//:libx264.a) $$FFMPEG_ROOT/additional_libs
        ASM=$$(pwd)/$(location @nasm//:nasm)
        pushd $$FFMPEG_ROOT 
            ./configure \
                --x86asmexe=$$ASM \
                --disable-everything \
                --disable-programs \
                --disable-doc \
                --disable-swresample \
                --enable-muxer=mpegts \
                --enable-decoder=h264 \
                --enable-encoder=aac \
                --enable-encoder=libx264 \
                --enable-gpl \
                --enable-libx264 \
                --extra-cflags=-I./additional_includes \
                --extra-ldflags=-L./additional_libs \
            && make
        popd
        cp $$FFMPEG_ROOT/config.h $(location config.h)
        cp $$FFMPEG_ROOT/libavcodec/libavcodec.a $(location libavcodec.a)
        cp $$FFMPEG_ROOT/libavformat/libavformat.a $(location libavformat.a)
        cp $$FFMPEG_ROOT/libavutil/libavutil.a $(location libavutil.a)
        cp $$FFMPEG_ROOT/libavutil/avconfig.h $(location libavutil/avconfig.h)
        cp $$FFMPEG_ROOT/libswscale/libswscale.a $(location libswscale.a)
    """,
)
