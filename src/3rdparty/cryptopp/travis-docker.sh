#!/bin/bash
set -e

scriptdir=$(dirname $0)

if [[ "$PLATFORM" == "gcc_64" ]]; then
	$scriptdir/linux.sh
fi

if [[ "$PLATFORM" == "android_armv7" ]]; then
	$scriptdir/android.sh armv7a
fi

if [[ "$PLATFORM" == "android_x86" ]]; then
	$scriptdir/android.sh x86
fi
