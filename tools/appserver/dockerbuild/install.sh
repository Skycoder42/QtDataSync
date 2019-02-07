#!/bin/sh
set -ex

export MAKEFLAGS=-j$(nproc)
export QDEP_CACHE_DIR=/tmp/qdep-cache

DS_NAME=qdsappd
MAIN_DEP="qt5-qtbase qt5-qtbase-postgresql qt5-qtwebsockets icu-libs"
DEV_DEP="qt5-qtbase-dev qt5-qtwebsockets-dev qt5-qttools qt5-qttools-dev icu-dev build-base python3 git perl gawk"
PIP_DEV_DEPS="qdep appdirs lockfile argcomplete"

if [ ! -d "/tmp/src/.git" ]; then
	echo "Failed to find .git directory - not supported!"
	exit 1
fi

apk add --no-cache $MAIN_DEP $DEV_DEP
pip3 install $PIP_DEV_DEPS

# prepare qdep
qdep prfgen --qmake qmake-qt5

# build json serializer, qtservice
mkdir /tmp/sysbuild
cd /tmp/sysbuild
for repo in QtJsonSerializer QtService; do
	git clone https://github.com/Skycoder42/$repo.git ./$repo
	cd $repo
	git checkout $(git describe --tags --abbrev=0)

	if [ -f src/imports/imports.pro ]; then
		echo "SUBDIRS -= imports" >> src/src.pro
	fi

	qmake-qt5
	make
	make install
	cd ..
done

# build datasync
cd /tmp/src
echo "SUBDIRS = 3rdparty messages" >> src/src.pro
echo "SUBDIRS = " >> examples/examples.pro
echo "SUBDIRS = " >> tests/tests.pro

qmake-qt5
make qmake_all
make
make install

#create special symlinks, dirs and move the env script
mkdir -p /etc/$DS_NAME
ln -s $(qmake-qt5 -query QT_INSTALL_BINS)/$DS_NAME /usr/bin/$DS_NAME || true #allow to fail if already exists
mv /tmp/src/tools/appserver/dockerbuild/env_start.sh /usr/bin/

# test if working
/usr/bin/$DS_NAME --version

# remove unused stuff
pip3 uninstall -y $PIP_DEV_DEPS
apk del --no-cache --purge "*-dev" $DEV_DEP
rm -rf /tmp/*
rm -rf $HOME/.cache/qpmx
rm -rf /usr/local/bin/qpm

# test if still working
/usr/bin/$DS_NAME --version
