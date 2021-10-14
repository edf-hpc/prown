#!/bin/sh
# Prown is a simple tool developed to give users the possibility to
# own projects (files and repositories).
# Copyright (C) 2021 EDF SA.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

set -e

TMPDIR=$(mktemp -d)

cleanup() {
  echo removing $TMPDIR
  sudo rm -rf $TMPDIR
}
trap cleanup EXIT

echo copy into $TMPDIR
cp -a $(dirname $0)/.. $TMPDIR/
# set required capability on prown binary
sudo setcap cap_chown+ep $TMPDIR/src/prown
echo run isolate
sudo $TMPDIR/tests/isolate
