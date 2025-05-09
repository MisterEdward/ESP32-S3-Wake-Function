#!/bin/bash
set -e

scriptdir=${0%`basename "$0"`}
cd $scriptdir
scriptdir=`pwd`

. .common.inc.sh

reset_build_ts
get_idf

# Remove or comment out the line that tries to add libssh2
# idf.py add-dependency "libssh2/libssh2=1.10.0"

$idf build

echo "build finished successfully"
