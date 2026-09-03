#!/bin/sh
# Build and run {{project_name}}.
#
# This file is YOURS (ownership class: seeded).
#
# sh rather than bash: Termux ships both, but sh is the one that is always
# there, and nothing here needs more than POSIX.
set -eu

cd "$(dirname "$0")"
make "$@"
exec ./build/{{project_name}}
