#!/bin/sh

DATASYNC_CONFIG=$DATASYNC_DIR/setup.conf
echo "[server]" > $DATASYNC_CONFIG
echo "port=80" >> $DATASYNC_CONFIG
echo "secret=$DATASYNC_SECRET" >> $DATASYNC_CONFIG
echo  >> $DATASYNC_CONFIG
echo "[database]" >> $DATASYNC_CONFIG
echo "name=$DATABASE_NAME" >> $DATASYNC_CONFIG
echo "host=$DATABASE_HOST" >> $DATASYNC_CONFIG
echo "port=$DATABASE_PORT" >> $DATASYNC_CONFIG
echo "username=$DATABASE_USER" >> $DATASYNC_CONFIG
echo "password=$DATABASE_PASSWORD" >> $DATASYNC_CONFIG

exec /opt/qdatasyncserver/bin/qdatasyncserver '__qbckgrndprcss$start#master~' -c "$DATASYNC_DIR/setup.conf" --no-daemon -L "$DATASYNC_LOG_FILE"
