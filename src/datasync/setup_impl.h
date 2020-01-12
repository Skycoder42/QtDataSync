#ifndef QTDATASYNC_SETUP_IMPL_H
#define QTDATASYNC_SETUP_IMPL_H

#include <QtCore/qdir.h>
#include <QtCore/qsettings.h>
#include <QtCore/qloggingcategory.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class FirebaseAuthenticator;
class ICloudTransformer;
class Engine;

class Q_DATASYNC_EXPORT SetupPrivate
{
public:
	enum class ConfigType {
		WebConfig,
		GoogleServicesJson,
		GoogleServicesPlist
	};

	QString projectId;
	QString apiKey;
	std::chrono::milliseconds readTimeOut {std::chrono::minutes{15}};
	int readLimit = 100;

#ifndef QTDATASYNC_NO_NTP
	QString ntpAddress;
	quint16 ntpPort = 123;
#endif

	QSettings *settings = nullptr;

	Engine *createEngine(FirebaseAuthenticator *authenticator,
						 ICloudTransformer *transformer,
						 QObject *parent);
	void finializeForEngine(Engine *engine);

	void readWebConfig(QIODevice *device, const std::function<void(QJsonObject)> &extender);
	void readGSJsonConfig(QIODevice *device, const std::function<void(QJsonObject)> &extender);
	void readGSPlistConfig(QIODevice *device, const std::function<void(QSettings*)> &extender);
};

struct OAuthExtenderPrivate
{
	quint16 port = 0;
	QString clientId;
	QString secret;
};

Q_DECLARE_LOGGING_CATEGORY(logSetup)

}

#endif // QTDATASYNC_SETUP_IMPL_H
