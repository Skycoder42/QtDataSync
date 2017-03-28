FROM ubuntu:latest

RUN apt-get update
RUN apt-get -qq install --no-install-recommends libglib2.0-0 libstdc++6 libpq5

ENV DATASYNC_DIR=/opt/qdatasyncserver/data
ENV DATASYNC_SECRET=
ENV DATABASE_HOST=
ENV DATABASE_PORT=5432
ENV DATABASE_USER=postgres
ENV DATABASE_PASSWORD=
ENV DATABASE_NAME=$DATABASE_USER

ADD lib /opt/qdatasyncserver/lib/
ADD plugins /opt/qdatasyncserver/plugins/
ADD bin /opt/qdatasyncserver/bin/
ADD setup.conf /etc/qdatasyncserver/setup.conf
RUN mkdir -p $DATASYNC_DIR

CMD ["/opt/qdatasyncserver/bin/env_start.sh"]