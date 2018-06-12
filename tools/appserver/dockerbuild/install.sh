#!/bin/sh
set -ex

export MAKEFLAGS=-j$(nproc)
export QPMX_CACHE_DIR=/tmp/qpmx-cache

DS_NAME=qdsappd
QT_VERSION=5.11.0
CRYPTOPP_VERSION_MAJOR=7
CRYPTOPP_VERSION_MINOR=0
CRYPTOPP_VERSION_PATCH=0
CRYPTOPP_VERSION=${CRYPTOPP_VERSION_MAJOR}_${CRYPTOPP_VERSION_MINOR}_${CRYPTOPP_VERSION_PATCH}
MAIN_DEP="libressl zlib dbus-libs glib libgcc libstdc++ libpcre2-16 libpq ca-certificates eudev-libs eudev libpcre2-16"
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
cd ..

# build qt modules
for repo in qtwebsockets qttools; do
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

	if [ -f src/imports/imports.pro ]; then
		echo "SUBDIRS -= imports" >> src/src.pro
	fi

	qmake
	make > /dev/null || (cat /tmp/qpmx-*/*; exit 1)
	make install
	cd ..
done

# build cryptopp
git clone --depth 1 https://github.com/weidai11/cryptopp.git ./cryptopp --branch CRYPTOPP_$CRYPTOPP_VERSION
cd cryptopp
CXXFLAGS="$CXXFLAGS -DNDEBUG -fPIC" make dynamic
make install PREFIX="/usr"
install -m644 "/tmp/src/tools/appserver/dockerbuild/libcrypto++.pc" "/usr/lib/pkgconfig/libcrypto++.pc"
cd ..
CPP_PATCHV=${CRYPTOPP_VERSION_MAJOR}.${CRYPTOPP_VERSION_MINOR}.${CRYPTOPP_VERSION_PATCH}
ln -s /usr/lib/libcryptopp.so.${CRYPTOPP_VERSION_MAJOR} /usr/lib/libcryptopp.so.${CPP_PATCHV}
ln -s /usr/lib/libcryptopp.so.${CRYPTOPP_VERSION_MAJOR}.${CRYPTOPP_VERSION_MINOR} /usr/lib/libcryptopp.so.${CPP_PATCHV}
/sbin/ldconfig -n /usr/lib

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
ln -s $(qmake -query QT_INSTALL_BINS)/$DS_NAME /usr/bin/$DS_NAME || true #allow to fail if already exists
mv dockerbuild/env_start.sh /usr/bin/

# test if working
/usr/bin/$DS_NAME --version

# remove unused stuff
apk del --no-cache --purge "*-dev" $DEV_DEP
rm -rf /tmp/*
rm -rf $HOME/.cache/qpmx
rm -rf /usr/local/bin/qpm

# test if still working
/usr/bin/$DS_NAME --version
