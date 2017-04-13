#!/bin/bash
# IMPORTANT: Adjust path to script of https://github.com/Skycoder42/QtModules (repogen.py)
# $1 path to module binaries
# $2 Version

myDir=$(dirname "$0")
"$myDir/../QtModules/repogen.py" "$1" DataSync "qt.58.skycoder42.jsonserializer>=2.0.0, qt.58.skycoder42.backgroundprocess>=1.5.0" "qdatasyncserver" "A simple offline-first synchronisation framework, to synchronize data of Qt applications between devices." "$2" "$myDir/LICENSE" BSD-3-Clause