#!/bin/sh

source "$(dirname $0)/shared.sh"

"$out/test.bin"
report_status $?

