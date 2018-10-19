#!/bin/bash

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
echo 'Running clang-check...'
find . -name '*.cpp' | xargs ./external/clang_*/bin/clang-check -p analysis/compile_commands.json -analyze
echo 'Finished clang-check'