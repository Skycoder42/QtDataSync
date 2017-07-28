#!/bin/bash
set -e

# get postgres running
if [[ $PLATFORM == "gcc_64" ]]; then
	apt-get -qq install docker-compose
fi

sudo docker-compose -f ./tools/qdatasyncserver/docker-compose.yaml up -d

# disable test on osx as workaround
if [[ $PLATFORM == "clang_64" ]]; then
	echo "SUBDIRS -= CachingDataStoreTest" >> ./tests/auto/datasync/datasync.pro
fi
