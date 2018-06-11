#!/bin/sh
set -ex

export MAKEFLAGS=-j$(nproc)

apk update
apk add qt5-qtbase-dev qt5-qtbase-postgresql qt5-qtwebsockets-dev crypto++-dev

# get QPM
curl -O https://www.qpm.io/download/v0.11.0/linux_386/qpm
install -m 755 ./qpm /usr/local/bin/

# build json serializer, qpmx, qtservice
for repo in QtJsonSerializer qpmx QtService; do
	# build QtService
	tdir=$(mktemp -d)
	cd $tdir
	
	git clone https://github.com/Skycoder42/$repo.git ./$repo
	cd $repo
	latesttag=$(git describe --tags)
	echo checking out ${latesttag}
	git checkout ${latesttag}

	qmake
	make qmake_all > /dev/null
	make > /dev/null
	make install
	cd ..
done

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
ln -s $(qmake -qt5 -query QT_INSTALL_BINS)/$DS_NAME /usr/bin/$DS_NAME
mv dockerbuild/env_start.sh /usr/bin/
