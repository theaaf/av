#!/usr/bin/env bash

ENABLE_CHECKS=\
cppcoreguidelines-*,\
hicpp-*,\
clang-analyzer-*,\
modernize-*,\
bugprone-*,\
cert-*,\
performance-*,\
readability-*,\
misc-*,\
portability-*,\

DISABLE_CORE_CHECKS=\
-cppcoreguidelines-pro-bounds-pointer-arithmetic,\
-cppcoreguidelines-pro-type-reinterpret-cast,\
-cppcoreguidelines-pro-type-union-access,\
-cppcoreguidelines-pro-bounds-constant-array-index,\
-cppcoreguidelines-pro-bounds-array-to-pointer-decay,\
-cppcoreguidelines-owning-memory,\

DISABLE_HI_CHECKS=\
-hicpp-signed-bitwise,\
-hicpp-no-array-decay,\

DISABLE_MISC_CHECKS=\
-readability-implicit-bool-conversion,\
-modernize-redundant-void-arg,\

OS="`uname`"
case $OS in
    'Linux')
    sed -i s+DUMMYROOT+$(pwd)+g analysis/compile_commands.json
    sed -i s+-fno-canonical-system-headers++g analysis/compile_commands.json
    ;;
    'Darwin')
    sed -i '' s+DUMMYROOT+$(pwd)+g analysis/compile_commands.json
    ;;
esac
echo 'Running clang-tidy...'
find . -name '*.cpp' | xargs ./external/clang_*/bin/clang-tidy -p analysis/compile_commands.json \
                            -checks=${ENABLE_CHECKS}${DISABLE_CORE_CHECKS}${DISABLE_HI_CHECKS}${DISABLE_MISC_CHECKS}
echo 'Finished clang-tidy'