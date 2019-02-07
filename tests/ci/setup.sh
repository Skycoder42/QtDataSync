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

# debug test
if [[ $TRAVIS_OS_NAME == "osx" ]]; then
	echo "QDEP_PATH = $(which qdep)" > /tmp/qdep.prf
	cat $scriptdir/qdep.prf >> /tmp/qdep.prf
	$SUDO mv /tmp/qdep.prf /opt/qt/$QT_VER/$PLATFORM/mkspecs/features/qdep.prf
fi
