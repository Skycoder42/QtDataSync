#!/bin/bash
set -e

# get postgres running
if [[ $PLATFORM == "gcc_64" ]]; then
	sudo curl -o /usr/local/bin/docker-compose -L "https://github.com/docker/compose/releases/download/1.11.2/docker-compose-$(uname -s)-$(uname -m)"
	sudo chmod +x /usr/local/bin/docker-compose
	sudo docker-compose -v
fi

sudo docker-compose -f ./tools/qdatasyncserver/docker-compose.yaml up -d

# disable test on osx as workaround
if [[ $PLATFORM == "clang_64" ]]; then
	echo "SUBDIRS -= CachingDataStoreTest" >> ./tests/auto/datasync/datasync.pro
fi
