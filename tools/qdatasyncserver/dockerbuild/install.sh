#!/bin/sh
set -e

QT_DIR=/tmp/qt/install/5.8/gcc_64
DATASYNC_DIR=/opt/qdatasyncserver

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