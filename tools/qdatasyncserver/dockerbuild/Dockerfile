##
## Docker image for compiling QT desktop applications
##

FROM debian:jessie-slim

ENV QT_PATH="/opt/qt"
ENV QT_TMP_PATH="/tmp/qt"
ENV QT_SCRIPT_PATH="/usr/local/bin"
ENV PATH="${PATH}:${QT_PATH}/5.8/gcc_64/bin"
ENV QT_INSTALLATION_DEPS='ca-certificates curl xvfb xauth libx11-xcb1 libfontconfig1 libdbus-1-3 libgl1-mesa-dev libglib2.0-0'

ADD qt-installer-script.qs ${QT_TMP_PATH}/qt-installer-script.qs
ADD qt-make-test.sh ${QT_SCRIPT_PATH}/qt-make-test.sh

##
## Use the following commands to list all dependencies required
## by the QT installer:
##
# RUN apk add --no-cache binutils
# RUN objdump -p ${QT_TMP_PATH}/installer.run | grep NEEDED
##
## Dependencies at the time of writing:
##
## libutil.so.1
## libX11.so.6
## libX11-xcb.so.1
## libxcb.so.1
## libfontconfig.so.1
## libfreetype.so.6
## libdbus-1.so.3
## libdl.so.2
## librt.so.1
## libpthread.so.0
## libstdc++.so.6
## libm.so.6
## libgcc_s.so.1
## libc.so.6
## ld-linux-x86-64.so.2
##

##
## 1) Install the dependencies required for installing QT (removed later on).
##
## 2) Download and run the QT installer.
##    Set QT_QPA_PLATFORM=minimal to make the QT installer not use any GUI.
##    Use the installer script to automate the installation process.
##
## 3) Uninstall dependencies that were required for installation only.
##    Install the dependencies required for compiling QT applications.
##
## 4) Clean up:
##    Remove QT files not required for compiling QT applications:
##      - example code
##      - documentation
##      - tools (QtCreator)
##    Remove temporary files.
##    Remove unused apt files.
##
RUN apt-get update
RUN apt-get -qq install --no-install-recommends $QT_INSTALLATION_DEPS
RUN curl -Lo ${QT_TMP_PATH}/installer.run https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
RUN chmod +x ${QT_TMP_PATH}/installer.run && QT_QPA_PLATFORM=minimal xvfb-run ${QT_TMP_PATH}/installer.run --script ${QT_TMP_PATH}/qt-installer-script.qs --addRepository https://install.skycoder42.de/qtmodules/linux_x64
RUN apt-get -qq purge --auto-remove $QT_INSTALLATION_DEPS

RUN apt-get -qq install --no-install-recommends libgl1-mesa-dev libglib2.0-0 libpulse-dev g++ make

RUN rm -rf ${QT_PATH}/Examples
RUN rm -rf ${QT_PATH}/Docs
RUN rm -rf /tmp/*
RUN rm -rf /var/lib/apt/lists/*

RUN chmod +x ${QT_SCRIPT_PATH}/qt-make-test.sh

