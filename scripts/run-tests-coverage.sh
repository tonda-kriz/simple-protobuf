#!/bin/bash

set -e

rm -rf coverage
cmake -B coverage -G Ninja . -DCMAKE_BUILD_TYPE=Debug -DSDS_PROTO_BUILD_TEST=ON -DSDS_PROTO_USE_COVERAGE=ON
cmake --build coverage --target unit_tests
cd coverage
ctest --output-on-failure
lcov --capture -o coverage.info  --directory . --exclude '/usr/*' --exclude '*/test/*' -q --gcov-tool gcov
genhtml coverage.info --output-directory report
