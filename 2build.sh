#!/bin/bash

git pull origin $(git rev-parse --abbrev-ref HEAD)
pushd ~/os161/root

sys161 kernel-ASST3-OPT

popd
