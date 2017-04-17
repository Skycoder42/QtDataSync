FROM ubuntu:rolling

ENV QT_DEPS="libglib2.0-0 libstdc++6 libpq5 ca-certificates"
ENV QT_BUILD_DEPS="libgl1-mesa-dev libpulse-dev g++ make git curl xauth libx11-xcb1 libfontconfig1 libdbus-1-3"

# install deps
RUN apt-get update && apt-get -qq install --no-install-recommends $QT_DEPS $QT_BUILD_DEPS

# install qt
RUN mkdir /tmp/qt && curl -Lo /tmp/qt/installer.run https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
ADD qt-installer-script.qs /tmp/qt/qt-installer-script.qs
RUN chmod +x /tmp/qt/installer.run && QT_QPA_PLATFORM=minimal /tmp/qt/installer.run --script /tmp/qt/qt-installer-script.qs --addRepository https://install.skycoder42.de/qtmodules/linux_x64 

# copy datasync to /opt
ADD install.sh /tmp/qt/install.sh
RUN chmod +x /tmp/qt/install.sh && /tmp/qt/install.sh
ADD env_start.sh /opt/qdatasyncserver/bin/env_start.sh
ADD setup.conf /etc/qdatasyncserver/setup.conf

# remove unused stuff
RUN apt-get -qq purge --auto-remove $QT_BUILD_DEPS
RUN rm -rf /tmp/* && rm -rf /var/lib/apt/lists/*

# create env vars & ready for usage
ENV DATASYNC_DIR=/opt/qdatasyncserver/data
ENV DATASYNC_SECRET=
ENV DATABASE_HOST=
ENV DATABASE_PORT=5432
ENV DATABASE_USER=postgres
ENV DATABASE_PASSWORD=
ENV DATABASE_NAME=$DATABASE_USER

RUN mkdir -p $DATASYNC_DIR

CMD ["/opt/qdatasyncserver/bin/env_start.sh"]