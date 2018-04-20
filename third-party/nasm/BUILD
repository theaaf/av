genrule(
    name = "nasm-build",
    srcs = glob(["**/*"], exclude=["bazel-*"]),
    outs = ["nasm"],
    executable = True,
    output_to_bindir = True,
    cmd = """
        NASM_ROOT=$$(dirname $(location configure))
        pushd $$NASM_ROOT 
            ./configure
            make -j8
        popd
        cp $$NASM_ROOT/nasm $(location nasm)
    """,
    visibility = ["//visibility:public"],
)
