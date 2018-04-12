genrule(
    name = "yasm-build",
    srcs = glob(["**/*"], exclude=["bazel-*"]),
    outs = ["yasm"],
    executable = True,
    output_to_bindir = True,
    cmd = """
        YASM_ROOT=$$(dirname $(location configure))
        pushd $$YASM_ROOT 
            ./configure
            make -j8
        popd
        cp $$YASM_ROOT/yasm $(location yasm)
    """,
    visibility = ["//visibility:public"],
)
