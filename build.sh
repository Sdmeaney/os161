#!/bin/bash

pushd ~/os161/kern/conf
./config ASST2
cd ../compile/ASST2
bmake depend
bmake
bmake install
popd
