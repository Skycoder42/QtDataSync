FROM ubuntu:rolling

ADD qt-installer-script.qs /tmp/qt/qt-installer-script.qs
ADD install.sh /tmp/qt/install.sh

RUN /tmp/qt/install.sh

ADD env_start.sh /opt/qdatasyncserver/bin/env_start.sh
ADD setup.conf /etc/qdatasyncserver/setup.conf

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