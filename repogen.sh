#!/bin/bash
# IMPORTANT: Adjust path to script of https://github.com/Skycoder42/QtModules (repogen.py)
# $1 path to module binaries
# $2 Version


myDir=$(dirname "$0")
qtDir=${1?First parameter must be set to the dir to install}
version=${2?Set the version as second parameter}
qtvid=${qtdir//./}

"$myDir/../QtModules/deploy/repogen.py"  "$qtDir" DataSync "qt.qt5.$qtvid.skycoder42.jsonserializer,qt.qt5.$qtvid.skycoder42.backgroundprocess" "" "A simple offline-first synchronisation framework, to synchronize data of Qt applications between devices." "$version" "$myDir/LICENSE" BSD-3-Clause
