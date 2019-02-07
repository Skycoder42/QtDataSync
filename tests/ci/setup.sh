#!/bin/bash
set -e

scriptdir=$(dirname $0)

# get postgres running for ws test
if [[ $PLATFORM == "gcc_64" ]]; then
	sudo docker run -d --rm --name datasync_postgres -e "POSTGRES_PASSWORD=baum42" postgres:latest

	postgresIp=$(sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' datasync_postgres)
	echo host=$postgresIp >> ./tests/ci/qdsapp.conf
	echo 'SETUP_FILE=$$PWD/../../../ci/qdsapp.conf' > ./tests/auto/datasync/TestAppServer/setup.pri
	echo 'SETUP_FILE=$$PWD/../../../ci/qdsapp.conf' > ./tests/auto/datasync/IntegrationTest/setup.pri
fi
