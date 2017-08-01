#!/bin/bash
set -e

# get postgres running for ws test
if [[ $PLATFORM == "gcc_64" ]]; then
	sudo curl -o /usr/local/bin/docker-compose -L "https://github.com/docker/compose/releases/download/1.14.0/docker-compose-$(uname -s)-$(uname -m)"
	sudo chmod +x /usr/local/bin/docker-compose

	sudo docker-compose -f ./tests/ci/docker-compose.yaml up -d
	#docker run -d --name datasync_example_postgres -e POSTGRES_USER=qtdatasync -e POSTGRES_PASSWORD=baum42 -e POSTGRES_DB=QtDataSync postgres:latest

	
	postgresIp=$(sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' datasync_example_postgres)
	echo host=$postgresIp >> ./tests/ci/server-setup.conf
	echo 'SETUP_FILE=$$PWD/../../../ci/server-setup.conf' > ./tests/auto/datasync/WsRemoteConnectorTest/setup.pri
fi

# disable test on osx as workaround (build fail and no docker support)
if [[ $PLATFORM == "clang_64" ]]; then
	echo "SUBDIRS -= CachingDataStoreTest WsRemoteConnectorTest" >> ./tests/auto/datasync/datasync.pro
fi
