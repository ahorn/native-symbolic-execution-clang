#!/bin/bash

FILENAME=$1
TMP=.${FILENAME}.tmp
CLANG_NSE=../../../../../../../build/target/bin/clang-nse
CLANG_CPP=/usr/bin/clang++

${CLANG_CPP} -cc1 -rewrite-macros ${FILENAME} > ${TMP} && mv ${TMP} ${FILENAME}
${CLANG_NSE} ${FILENAME} --
echo -e "#include <nse_sequential.h>\n#include <nse_report.h>" > ${TMP} && cat ${FILENAME} >> ${TMP} && mv ${TMP} ${FILENAME}
