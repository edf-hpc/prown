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
echo run isolate
sudo $TMPDIR/tests/isolate
