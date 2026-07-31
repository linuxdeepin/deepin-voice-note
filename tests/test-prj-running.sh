#!/bin/bash

# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

builddir=build
reportdir=build-ut

# Tiptap migration UTs invoke web-editor/scripts/validate-envelope.mjs.
# Keep node_modules out of git, but make the UT entry script prepare the
# JavaScript dependencies declared by package-lock.json before running C++ UTs.
repo_root=$(cd "$(dirname "$0")/.."; pwd)
if [ -f "$repo_root/web-editor/package-lock.json" ]; then
    if [ ! -d "$repo_root/web-editor/node_modules/@tiptap/core" ]; then
        (cd "$repo_root/web-editor" && npm ci)
    fi
fi

rm -rf $builddir
rm -rf ../$builddir
rm -rf $reportdir
rm -rf ../$reportdir
mkdir -p ../$builddir
mkdir -p ../$reportdir
cd ../$builddir
#编译
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_SAFETYTEST_ARG="CMAKE_SAFETYTEST_ARG_ON" ..
make -j8
#生成asan日志和ut测试xml结果
./tests/deepin-voice-note-test --gtest_output=xml:./report/report_deepin-voice-note.xml

workdir=$(cd ../$(dirname $0)/$builddir; pwd)

mkdir -p report
#统计代码覆盖率并生成html报告
lcov -d $workdir -c -o ./coverage.info

lcov --extract ./coverage.info '*/src/*' -o ./coverage.info

lcov --remove ./coverage.info '*/tests/*' -o ./coverage.info

genhtml -o ./html ./coverage.info

mv ./html/index.html ./html/cov_deepin-voice-note.html
#对asan、ut、代码覆盖率结果收集至指定文件夹
cp -r html ../$reportdir/
cp -r report ../$reportdir/
cp -r asan*.log* ../$reportdir/asan_deepin-voice-note.log

exit 0
