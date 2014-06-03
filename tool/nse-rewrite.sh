#!/bin/bash

FILENAME=$1
TMP=.${FILENAME}.tmp
CLANG_NSE=clang-nse

${CLANG_NSE} ${FILENAME} --
echo -e "#include <nse_sequential.h>\n#include <nse_report.h>" > ${TMP} && cat ${FILENAME} >> ${TMP} && mv ${TMP} ${FILENAME}
sed 's/assert\((.*)\)/crv::sequential_dfs_checker().add_error(!\1)/g' ${FILENAME} > ${TMP}
mv ${TMP} ${FILENAME}
