#!/bin/bash

mkdir -p benchmark/img

find build/benchmark -type f -executable -exec strip --strip-all {} +
python scripts/file-size-plotter.py build/benchmark
mv file-size-benchmark.png benchmark/img/file-size-benchmark.png

python scripts/benchmark-plotter.py build/benchmark/gpb-benchmark build/benchmark/spb-benchmark build/benchmark/gpb-lite-benchmark
mv speed-benchmark.png benchmark/img/speed-benchmark.png
