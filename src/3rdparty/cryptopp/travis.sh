#!/bin/bash
set -e

version=6_0_0

case "$PLATFORM" in
	gcc_64)
		pkgName=linux
		;;
	clang_64)
		pkgName=macos
		;;
	*)
		pkgName=$PLATFORM
		;;
esac

curl -Lo /tmp/cryptopp.tar.xz https://github.com/Skycoder42/ci-builds/releases/download/cryptopp_${version}/cryptopp_${version}_${pkgName}.tar.xz
tar -xf /tmp/cryptopp.tar.xz -C ./src/3rdparty/
