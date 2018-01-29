LABEL maintainer="skycoder42.de@gmx.de"
FROM ubuntu:rolling #rolling until bionic was released. Qt 5.5 is just to old

COPY install.sh /tmp/install.sh
RUN /tmp/install.sh

COPY env_start.sh /usr/bin/env_start.sh
# create env vars & ready for usage
ENV QDSAPP_CONFIG_FILE=/etc/qdsapp.conf
# must be set: ENV QDSAPP_DATABASE_HOST=
ENV QDSAPP_DATABASE_PORT=5432
ENV QDSAPP_DATABASE_USER=postgres
# must be set: ENV QDSAPP_DATABASE_PASSWORD=
ENV QDSAPP_DATABASE_NAME=$QDSAPP_DATABASE_USER

CMD ["/usr/bin/env_start.sh"]
