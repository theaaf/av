filegroup(
    name = "headers",
    srcs = [
        "source/x265.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "x265",
    hdrs = [":headers"],
    srcs = ["lib/libx265.a"],
    includes = ["."],
    visibility = ["//visibility:public"],
)

config_setting(
    name = "cmake_osx",
    constraint_values = [
        "@bazel_tools//platforms:osx",
    ],
)

config_setting(
    name = "cmake_linux",
    constraint_values = [
        "@bazel_tools//platforms:linux",
    ],
)

filegroup(
    name = "cmake",
    srcs = select({
        ":cmake_osx": ["@cmake_osx//:cmake"],
        ":cmake_linux": ["@cmake_linux//:cmake"],
    }),
)

genrule(
    name = "x265-build",
    srcs = glob(["**/*",], exclude = ["bazel-*"]) + ["@nasm//:nasm",] + [":cmake"],
    outs = [
        "include/x265_config.h",
        "include/x265.h",
        "lib/libx265.a",
        "x265.pc"
    ],
    cmd = """
        X265_ROOT=$$(dirname $(location source/CMakeLists.txt))
        CMAKE=$$(pwd)/$(location cmake)
        AS=$$(pwd)/$(location @nasm//:nasm)
        pushd $$X265_ROOT
            AS=$$AS $$CMAKE -DCMAKE_INSTALL_PREFIX=$$(dirname $(location x265.pc)) -DENABLE_SHARED:bool=off .
            make -j8
        popd
        cp $$X265_ROOT/x265.h $(location include/x265.h)
        cp $$X265_ROOT/x265_config.h $(location include/x265_config.h)
        cp $$X265_ROOT/libx265.a $(location lib/libx265.a)
        cp $$X265_ROOT/x265.pc $(location x265.pc)
    """,
    visibility = ["//visibility:public"],
)