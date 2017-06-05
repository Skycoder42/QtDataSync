#!/bin/sh
set -e

QT_DEPS="libglib2.0-0 libstdc++6 libpq5 ca-certificates"
QT_BUILD_DEPS="libgl1-mesa-dev libpulse-dev g++ make git curl xauth libx11-xcb1 libfontconfig1 libdbus-1-3"
INSTALL_DIR=/tmp/qt
INSTALLER=$INSTALL_DIR/installer.run
QT_DIR=$INSTALL_DIR/install/5.9/gcc_64
DATASYNC_DIR=/opt/qdatasyncserver

# install deps
apt-get update
apt-get -qq install --no-install-recommends $QT_DEPS $QT_BUILD_DEPS

# install qt
curl -Lo $INSTALLER https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
chmod +x $INSTALLER
QT_QPA_PLATFORM=minimal $INSTALLER --script $INSTALL_DIR/qt-installer-script.qs --addRepository https://install.skycoder42.de/qtmodules/linux_x64

# move libs
QT_LIB_DIR=$QT_DIR/lib
DATASYNC_LIB_DIR=$DATASYNC_DIR/lib/
mkdir -p $DATASYNC_LIB_DIR
mv $QT_LIB_DIR/libicudata.so* $DATASYNC_LIB_DIR
mv $QT_LIB_DIR/libicui18n.so* $DATASYNC_LIB_DIR
mv $QT_LIB_DIR/libicuuc.so* $DATASYNC_LIB_DIR
mv $QT_LIB_DIR/libQt5Core.so* $DATASYNC_LIB_DIR
mv $QT_LIB_DIR/libQt5Network.so* $DATASYNC_LIB_DIR
mv $QT_LIB_DIR/libQt5Concurrent.so* $DATASYNC_LIB_DIR
mv $QT_LIB_DIR/libQt5Sql.so* $DATASYNC_LIB_DIR
mv $QT_LIB_DIR/libQt5WebSockets.so* $DATASYNC_LIB_DIR
mv $QT_LIB_DIR/libQt5BackgroundProcess.so* $DATASYNC_LIB_DIR

# move plugins
QT_PLUGIN_DIR=$QT_DIR/plugins
DATASYNC_PLUGIN_DIR=$DATASYNC_DIR/plugins
mkdir -p $DATASYNC_PLUGIN_DIR/sqldrivers
mv $QT_PLUGIN_DIR/sqldrivers/libqsqlpsql.so $DATASYNC_PLUGIN_DIR/sqldrivers/

# move binary & conf
QT_BIN_DIR=$QT_DIR/bin
DATASYNC_BIN_DIR=$DATASYNC_DIR/bin/
mkdir -p $DATASYNC_BIN_DIR
mv $QT_BIN_DIR/qdatasyncserver $DATASYNC_BIN_DIR
mv $QT_BIN_DIR/qt.conf $DATASYNC_BIN_DIR

#create data dir
mkdir -p $DATASYNC_DIR/data

# remove unused stuff
apt-get -qq purge --auto-remove $QT_BUILD_DEPS
rm -rf /tmp/*
rm -rf /var/lib/apt/lists/*
