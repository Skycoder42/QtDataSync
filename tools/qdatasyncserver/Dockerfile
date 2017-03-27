FROM debian:jessie-slim

ADD lib /opt/qdatasyncserver/lib/
ADD plugins /opt/qdatasyncserver/plugins/
ADD bin /opt/qdatasyncserver/bin/

RUN apt-get update
RUN apt-get -qq install --no-install-recommends libglib2.0-0 libstdc++6

CMD ["--help"]
ENTRYPOINT /opt/qdatasyncserver/bin/qdatasyncserver