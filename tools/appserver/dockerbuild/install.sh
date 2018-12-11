#!/bin/bash
set -ex

export MAKEFLAGS=-j$(nproc)
export QPMX_CACHE_DIR=/tmp/qpmx-cache

DS_NAME=qdsappd
QT_VERSION_TAG=v5.12.0
MAIN_DEP="libssl1.1 zlib1g libdbus-1-3 libc6 libglib2.0-0 libudev1 libpcre2-16-0 libpq5 libdouble-conversion1 libicu57 libinput10 ca-certificates"
DEV_DEP="openssl libssl-dev zlib1g-dev dbus libdbus-1-dev libc6-dev libglib2.0-dev libudev-dev libdouble-conversion-dev libicu-dev libpcre2-dev libpq-dev libinput-dev make gcc g++ curl python3 git perl gawk"

if [ ! -d "/tmp/src/.git" ]; then
	echo "Failed to find .git directory - not supported!"
	exit 1
fi

apt-get -qq update
apt-get -qq install --no-install-recommends $MAIN_DEP $DEV_DEP

# get QPM
curl -O https://www.qpm.io/download/v0.11.0/linux_386/qpm
install -m 755 ./qpm /usr/local/bin/

mkdir /tmp/sysbuild
cd /tmp/sysbuild

# build qt
git clone --depth 1 https://code.qt.io/qt/qt5.git ./qt5 --branch $QT_VERSION_TAG
cd qt5
./init-repository --module-subset="qtbase,qtwebsockets,qttools"
_qt5_prefix=/usr/lib/qt5
_qt5_datadir=/usr/share/qt5
./configure -confirm-license -opensource \
	-prefix /usr \
	-archdatadir "$_qt5_prefix" \
	-datadir "$_qt5_prefix" \
	-importdir "$_qt5_prefix"/imports \
	-libexecdir "$_qt5_prefix"/libexec \
	-plugindir "$_qt5_prefix"/plugins \
	-translationdir "$_qt5_datadir"/translations \
	-sysconfdir /etc/xdg \
	-release -reduce-exports \
	-make libs -make tools \
	-glib \
	-no-rpath \
	-no-separate-debug-info \
	-openssl-linked \
	-plugin-sql-psql \
	-dbus-linked \
	-system-pcre \
	-system-zlib \
	-no-reduce-relocations \
	-no-gui -no-widgets -no-feature-accessibility -no-feature-dom
make > /dev/null
make install
cd ..

# build json serializer, qpmx, qtservice
for repo in QtJsonSerializer qpmx QtService; do
	git clone https://github.com/Skycoder42/$repo.git ./$repo
	cd $repo
	latesttag=$(git describe --tags --abbrev=0)
	echo checking out ${latesttag}
	git checkout ${latesttag}

	if [[ "$repo" == "qpmx" ]]; then
		git submodule init
		git submodule update
	fi

	if [[ -f src/imports/imports.pro ]]; then
		echo "SUBDIRS -= imports" >> src/src.pro
	fi

	qmake
	make > /dev/null || (cat /tmp/qpmx-*/*; exit 1)
	make install
	cd ..
done

# build datasync
cd /tmp/src
echo "SUBDIRS = 3rdparty messages" >> src/src.pro
echo "SUBDIRS = " >> examples/examples.pro
echo "SUBDIRS = " >> tests/tests.pro

qmake
make qmake_all
make
make install

#create special symlinks, dirs and move the env script
mkdir -p /etc/$DS_NAME
ln -s $(qmake -query QT_INSTALL_BINS)/$DS_NAME /usr/bin/$DS_NAME || true #allow to fail if already exists
mv /tmp/src/tools/appserver/dockerbuild/env_start.sh /usr/bin/

# test if working
/usr/bin/$DS_NAME --version

# remove unused stuff
apt-get -qq autoremove --purge $DEV_DEP
rm -rf /tmp/*
rm -rf $HOME/.cache/qpmx
rm -rf /usr/local/bin/qpm

# test if still working
/usr/bin/$DS_NAME --version
