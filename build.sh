#!/bin/bash

pushd ~/os161/kern/conf
./config ASST1
cd ../compile/ASST1
bmake depend
bmake
bmake install
popd
