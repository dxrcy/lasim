#!/bin/sh

tests="$(dirname $0)"
out="$tests/out"
project="$tests/.."
examples="$project/examples"

if [ ! -d "$tests" ] || 
    [ ! -d "$project" ] || 
    [ ! -d "$examples" ]; then
    echo 'missing directory'
    exit 1
fi
[ -d "$out" ] || mkdir "$out"

lasim() {
    "$tests/../lasim" $@ || exit $?
}

lc3as() {
    /usr/bin/lc3as $@
}

diff() {
    /usr/bin/delta $@
}

report_status() {
    if [ "$1" -eq 0 ];
        then printf '\x1b[32mpass\x1b[0m\n'
        else printf '\x1b[31mFAIL\x1b[0m\n'
    fi
}

