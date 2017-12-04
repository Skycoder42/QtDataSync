#!/bin/bash
set -e
# $1 android arch (armv7a, x86)

export MAKEFLAGS="-j$(nproc)"

export ANDROID_HOME=$HOME/android/sdk
export ANDROID_NDK=$HOME/android/sdk/ndk-bundle
export ANDROID_SDK_ROOT=$ANDROID_HOME
export ANDROID_NDK_ROOT=$ANDROID_NDK
export ABI_NR=16
export AOSP_API="android-$ABI_NR" #required to build against the same sysroot as Qt

VERSION=5_6_5
NAME=CRYPTOPP_${VERSION}
SHA512SUM=82e9a51080ace2734bfe4ba932c31e6a963cb20b570f0fea2fbe9ceccb887c8afecb36cde91c46ac6fea1357fdff6320ab2535b3f0aa48424acdd2cd9dd2e880
ABI=$1

if [[ "$ABI" == "armv7a" ]]; then
	ABI_INC=arm-linux-androideabi
fi
if [[ "$ABI" == "x86" ]]; then
	ABI_INC=i686-linux-android
fi

sDir=$(dirname $(readlink -f $0))
tDir=$(mktemp -d)
pushd $tDir

curl -Lo "./$NAME.tar.gz" "https://github.com/weidai11/cryptopp/archive/$NAME.tar.gz"
echo "$SHA512SUM $NAME.tar.gz" | sha512sum --check -

tar -xf "$NAME.tar.gz"

pushd cryptopp-$NAME
set +e
source setenv-android.sh $ABI gnu
set -e
export AOSP_STL_INC="$ANDROID_NDK_ROOT/sysroot/usr/include/$ABI_INC -I$AOSP_STL_INC -I$ANDROID_NDK_ROOT/sysroot/usr/include/"
export CXXFLAGS="-D__ANDROID_API__=$ABI_NR"
make -f GNUmakefile-cross static
make -f GNUmakefile-cross install PREFIX=$sDir

popd
rm -rf $tDir
