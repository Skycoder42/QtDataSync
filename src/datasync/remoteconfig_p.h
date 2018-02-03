#ifndef QTDATASYNC_REMOTECONFIG_P_H
#define QTDATASYNC_REMOTECONFIG_P_H

#include "qtdatasync_global.h"
#include "remoteconfig.h"

namespace QtDataSync {

//no export needed
class RemoteConfigPrivate : public QSharedData
{
public:
	RemoteConfigPrivate(const QUrl &url,
						const QString &accessKey,
						const RemoteConfig::HeaderHash &headers,
						int keepaliveTimeout);
	RemoteConfigPrivate(const RemoteConfigPrivate &other);

	QUrl url;
	QString accessKey;
	RemoteConfig::HeaderHash headers;
	int keepaliveTimeout;
};

}

#endif // QTDATASYNC_REMOTECONFIG_P_H
