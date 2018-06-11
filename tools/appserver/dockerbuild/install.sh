#!/bin/sh
set -ex

export MAKEFLAGS=-j$(nproc)

DS_NAME=qdsappd
QT_VERSION=5.11.0
MAIN_DEP="libressl zlib dbus-libs glib libgcc libpcre2-16 libpq ca-certificates"
DEV_DEP="libressl-dev zlib-dev dbus-dev glib-dev perl eudev-dev gawk pcre2-dev postgresql-dev linux-headers make gcc g++ curl python3 git"

apk add --no-cache $MAIN_DEP $DEV_DEP

# get QPM
curl -O https://www.qpm.io/download/v0.11.0/linux_386/qpm
install -m 755 ./qpm /usr/local/bin/

mkdir /tmp/sysbuild
cd /tmp/sysbuild

# build qtbase
git clone --depth 1 https://code.qt.io/qt/qtbase.git ./qtbase --branch $QT_VERSION
cd qtbase
git apply -v /tmp/src/tools/appserver/dockerbuild/libressl-compat.patch
for file in src/network/ssl/*; do
	sed -i -e 's/OPENSSL_VERSION_NUMBER >= 0x10002000L/!defined(LIBRESSL_VERSION_NUMBER)/g' "$file"
done
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

# build qt modules
for repo in qtwebsockets; do
	git clone --depth 1 https://code.qt.io/qt/$repo.git ./$repo --branch $QT_VERSION
	cd $repo
	qmake
	make > /dev/null
	make install
	cd ..
done

# build json serializer, qpmx, qtservice
for repo in QtJsonSerializer qpmx QtService; do
	git clone https://github.com/Skycoder42/$repo.git ./$repo
	cd $repo
	latesttag=$(git describe --tags --abbrev=0)
	echo checking out ${latesttag}
	git checkout ${latesttag}
	
	if [ "$repo" == "qpmx" ]; then
		git submodule init
		git submodule update
		git apply -v /tmp/src/tools/appserver/dockerbuild/qpmx.patch
	fi

	qmake
	make > /dev/null || (cat /tmp/qpmx-*/*; exit 1)
	make install
	cd ..
done

# build cryptopp
git clone --depth 1 https://github.com/weidai11/cryptopp.git ./cryptopp
cd $repo
latesttag=$(git describe --tags --abbrev=0)
echo checking out ${latesttag}
git checkout ${latesttag}

sed -i -e 's/^CXXFLAGS/#CXXFLAGS/' GNUmakefile
export CXXFLAGS="${CXXFLAGS} -DNDEBUG -fPIC"
make -f GNUmakefile
make libcryptopp.so
make install
cd ..

# build messages
cd /tmp/src/src/messages
ln -s ../../tools/appserver/.qmake.conf

echo "CONFIG+=system_cryptopp" >> .qmake.conf
qmake
make qmake_all
make

# build the server and install it
cd /tmp/src/tools/appserver
ln -s ../../src/messages/lib lib

# already done for messages: echo "CONFIG+=system_cryptopp" >> .qmake.conf
qmake
make qmake_all
make
make install

#create special symlinks, dirs and move the env script
mkdir -p /etc/$DS_NAME
ln -s $(qmake -qt5 -query QT_INSTALL_BINS)/$DS_NAME /usr/bin/$DS_NAME || true #allow to fail if already exists
mv dockerbuild/env_start.sh /usr/bin/

# remove unused stuff
apk del --no-cache --purge "*-dev" $DEV_DEP
rm -rf /tmp/*
rm -rf $HOME/.cache/qpmx
rm -rf /usr/local/bin/qpm

# test if working
/usr/bin/$DS_NAME --version
