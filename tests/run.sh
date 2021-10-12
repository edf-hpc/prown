#!/bin/sh
set -e
TMPDIR=$(mktemp -d)
echo compiling
gcc -o isolate isolate.c
echo copy into $TMPDIR
cp -r $(dirname $0)/.. $TMPDIR/
echo run isolate
sudo $TMPDIR/tests/isolate
echo removing $TMPDIR
rm -rf $TMPDIR
