#!/bin/sh

set -eu

install -d -o node -g node /data
exec gosu node "$@"
