#!/bin/sh
set -ex

cd /tmp/src/tools/appserver

export MAKEFLAGS=-j$(nproc)

DS_NAME=qdsappd
TOOL_DEPS="libqt5sql5 libqt5sql5-psql libqt5concurrent5 libqt5websockets5 libcrypto++6 libicu57"
BUILD_DEPS="build-essential qtbase5-dev qtdeclarative5-dev libqt5scxml-dev libqt5remoteobjects-dev libqt5websockets5-dev libqt5jsonserializer-dev libcrypto++-dev libicu-dev qt5-qmake qpmx pkg-config curl ca-certificates"

echo "CONFIG+=system_cryptopp" >> .qmake.conf

# install deps
apt-get -qq update
apt-get -qq install software-properties-common
add-apt-repository -y ppa:skycoder42/qt-modules
apt-get -qq update
apt-get -qq install --no-install-recommends $TOOL_DEPS $BUILD_DEPS
curl -O https://www.qpm.io/download/v0.10.0/linux_386/qpm
install -m 755 ./qpm /usr/local/bin/

# build the server and install it
qmake -qt5
make qmake_all
make
make install

#create special symlinks, dirs and move the env script
mkdir -p /etc/$DS_NAME
ln -s $(qmake -qt5 -query QT_INSTALL_BINS)/$DS_NAME /usr/bin/$DS_NAME
mv dockerbuild/env_start.sh /usr/bin/

# remove unused stuff
apt-get -qq purge $BUILD_DEPS
apt-get -qq autoremove
rm -rf /tmp/*
rm -rf /var/lib/apt/lists/*
rm -rf $HOME/.cache/qpmx
rm -rf /usr/local/bin/qpm
