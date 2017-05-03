#!/usr/bin/env bash

set -e

taskset 0x3 bin/test_app 'string argument' 0 1 0x12313
