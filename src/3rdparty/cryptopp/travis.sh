#!/bin/bash
set -e

scriptdir=$(dirname $0)

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
	docker run --rm --name docker-qt-build -e QMAKE_FLAGS -e BUILD_DOC -e TEST_DIR -e NO_TESTS -v "$(pwd):/root/project" skycoder42/qt-build /root/project/src/3rdparty/cryptopp/travis-docker.sh
fi

if [[ "$PLATFORM" == "clang_64" ]]; then
	$scriptdir/macos.sh
fi

if [[ "$PLATFORM" == "ios" ]]; then
	$scriptdir/ios.sh
fi
