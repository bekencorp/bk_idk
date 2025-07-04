#!/bin/bash

set -e

pwd=$(pwd)

rm -rf build
mkdir -p build
cp -r config/* build

cd build
${pwd}/main.py gen all --debug
${pwd}/main.py pack all --debug

cp all-app.bin /win30/ming.liu/bk7236/tmp1/
cp ota.bin /win30/ming.liu/bk7236/tmp1/
