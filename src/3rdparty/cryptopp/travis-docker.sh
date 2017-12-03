#!/bin/bash
set -e

scriptdir=$(dirname $0)

if [[ "$PLATFORM" == "gcc_64" ]]; then
	$scriptdir/linux.sh
fi

if [[ "$PLATFORM" == "android"* ]]; then
	$scriptdir/android.sh
fi
