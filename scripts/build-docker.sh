#!/bin/bash

set -o errexit -o pipefail -o nounset

cd "$(readlink -f "$0" | xargs dirname)" &&
docker build -t rsafefs-dev ..

#docker run -it --rm rsafefs-dev /bin/bash