#!/bin/bash

set -o errexit -o pipefail -o nounset

cd "$(readlink -f "$0" | xargs dirname | xargs dirname)" &&
find . -path ./build -prune -name '*.[ch]' -o -name '*.[ch]pp' | xargs clang-format -i