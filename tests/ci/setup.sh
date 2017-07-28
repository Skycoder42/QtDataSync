#!/bin/bash
set -e

# get postgres running for ws test
if [[ $PLATFORM == "gcc_64" ]]; then
	sudo curl -o /usr/local/bin/docker-compose -L "https://github.com/docker/compose/releases/download/1.12.0/docker-compose-$(uname -s)-$(uname -m)"
	sudo chmod +x /usr/local/bin/docker-compose

	sudo docker-compose -f ./tools/qdatasyncserver/docker-compose.yaml up -d
fi

# disable test on osx as workaround (build fail and no docker support)
if [[ $PLATFORM == "clang_64" ]]; then
	echo "SUBDIRS -= CachingDataStoreTest WsRemoteConnectorTest" >> ./tests/auto/datasync/datasync.pro
fi
