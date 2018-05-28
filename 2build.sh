#!/bin/bash

git pull origin $(git rev-parse --abbrev-ref HEAD)
pushd ~/os161/kern/conf
./config ASST2
cd ../compile/ASST3
bmake depend
bmake
bmake install
popd
