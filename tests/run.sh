#!/bin/sh
set -e

TMPDIR=$(mktemp -d)

cleanup() {
  echo removing $TMPDIR
  sudo rm -rf $TMPDIR
}
trap cleanup EXIT

echo compiling
gcc -o isolate isolate.c
echo copy into $TMPDIR
cp -a $(dirname $0)/.. $TMPDIR/
# set required capability on prown binary
sudo setcap cap_chown+ep $TMPDIR/src/prown
echo run isolate
sudo $TMPDIR/tests/isolate
