#!/bin/bash
set -e

# get postgres running for ws test
if [[ $PLATFORM == "gcc_64" ]]; then
	echo "CONFIG += include_server_tests" >> .qmake.conf

	sudo curl -o /usr/local/bin/docker-compose -L "https://github.com/docker/compose/releases/download/1.18.0/docker-compose-$(uname -s)-$(uname -m)"
	sudo chmod +x /usr/local/bin/docker-compose

	sudo docker run -d --rm --name datasync_postgres -e "POSTGRES_PASSWORD=baum42" postgres:latest

	postgresIp=$(sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' datasync_postgres)
	echo host=$postgresIp >> ./tests/ci/qdsapp.conf
	echo 'SETUP_FILE=$$PWD/../../../ci/qdsapp.conf' > ./tests/auto/datasync/TestAppServer/setup.pri
	echo 'SETUP_FILE=$$PWD/../../../ci/qdsapp.conf' > ./tests/auto/datasync/IntegrationTest/setup.pri
fi
