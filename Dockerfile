#LABEL maintainer="skycoder42.de@gmx.de"
FROM alpine:latest

COPY . /tmp/src
RUN /tmp/src/tools/appserver/dockerbuild/install.sh

ENV QDSAPP_CONFIG_FILE=/etc/qdsapp.conf \
	QDSAPP_DATABASE_HOST= \
	QDSAPP_DATABASE_PORT=5432 \
	QDSAPP_DATABASE_USER=postgres \
	QDSAPP_DATABASE_PASSWORD= \
	QDSAPP_DATABASE_NAME=postgres
CMD ["/usr/bin/env_start.sh"]
