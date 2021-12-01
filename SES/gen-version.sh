#!/bin/bash
GIT_COMMIT=`git rev-parse --short HEAD || echo 'unknown'`-`git diff --quiet || echo 'dirty'`
GIT_BRANCH=`whoami`@`git rev-parse --abbrev-ref HEAD || echo 'unknown'`
GIT_BRANCH_NUM=`git rev-list --count HEAD || echo 'nan'`
VERSION=`git describe --tags --abbrev=0 --exact-match 2>/dev/null || echo 'unknown'`

echo -DGIT_COMMIT=\\\"${GIT_COMMIT}\\\" -DGIT_BRANCH=\\\"${GIT_BRANCH}\\\" -DGIT_BRANCH_NUM=\\\"${GIT_BRANCH_NUM}\\\" -DVERSION=\\\"${VERSION}\\\" > version.inc
#sleep 5