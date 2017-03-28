#!/bin/bash
# $1 $$PWD
# $2 $$QT_INSTALL_LIBS
# $2 $$QT_INSTALL_PLUGINS
# $pwd: $$OUT_PWD

rm -R dockerbuild
mkdir dockerbuild
cd dockerbuild
cp "$1/Dockerfile" ./

# deployment
# copy libs
mkdir lib
cp -P $2/libicudata.so* ./lib/
cp -P $2/libicui18n.so* ./lib/
cp -P $2/libicuuc.so* ./lib/
cp -P $2/libQt5Core.so* ./lib/
cp -P $2/libQt5Network.so* ./lib/
cp -P $2/libQt5Concurrent.so* ./lib/
cp -P $2/libQt5Sql.so* ./lib/
cp -P $2/libQt5WebSockets.so* ./lib/
cp -P $2/libQt5BackgroundProcess.so* ./lib/

# copy plugins
mkdir plugins
mkdir plugins/sqldrivers
cp $3/sqldrivers/libqsqlpsql.so ./plugins/sqldrivers/

# copy binary
mkdir bin
cp ../../../bin/qdatasyncserver ./bin/

# run dockerbuild
sudo docker build -t skycoder42/qdatasyncserver . 
sudo docker run --name qdatasyncserver skycoder42/qdatasyncserver
sudo docker rm qdatasyncserver