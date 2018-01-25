#!/bin/sh
set -e

# install deps
apt-get -qq update
apt-get -qq install software-properties-common
add-apt-repository -y ppa:skycoder42/qt-modules
apt-get -qq update
apt-get -qq install libqt5datasync4-tools

#create data dir
mkdir -p /etc/qdsappd

# remove unused stuff
rm -rf /tmp/*
rm -rf /var/lib/apt/lists/*
