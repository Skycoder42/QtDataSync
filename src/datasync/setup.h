#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/engine.h"
#include "QtDataSync/exception.h"

#include <chrono>

#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qsettings.h>

class QLockFile;

namespace QtDataSync {

class IAuthenticator;
class ICloudTransformer;

class SetupPrivate;
class Q_DATASYNC_EXPORT Setup
{
	Q_DISABLE_COPY(Setup)

public:
	enum class ConfigType {
		WebConfig,
		GoogleServicesJson,
		GoogleServicesPlist
	};

	Setup();
	Setup(Setup &&other) noexcept;
	Setup &operator=(Setup &&other) noexcept;
	~Setup();

	void swap(Setup &other);

	static Setup fromConfig(const QString &configPath, ConfigType configType);
	static Setup fromConfig(QIODevice *configDevice, ConfigType configType);

	Setup &setSettings(QSettings *settings);

	Setup &setFirebaseProjectId(QString projectId);
	Setup &setFirebaseWebApiKey(QString webApiKey);

	Setup &setOAuthClientCallbackPort(quint16 port);
	Setup &setOAuthClientId(QString clientId);
	Setup &setOAuthClientSecret(QString secret);

	Setup &setAuthenticator(IAuthenticator *authenticator);

	Setup &setRemoteReadTimeout(std::chrono::milliseconds ms);
	Setup &setRemotePageLimit(int limit);
	Setup &setCloudTransformer(ICloudTransformer *transformer);

#ifndef QTDATASYNC_NO_NTP
	Setup &enableNtpSync(QString hostName = QStringLiteral("pool.ntp.org"), quint16 port = 123);
#endif

	Engine *createEngine(QObject *parent = nullptr);

private:
	QScopedPointer<SetupPrivate> d;
};

class Q_DATASYNC_EXPORT SetupException : public Exception
{
public:
	SetupException(QString error);

	QString qWhat() const override;

	void raise() const override;
	ExceptionBase *clone() const override;

protected:
	QString _error;
};

inline Q_DATASYNC_EXPORT void swap(Setup& lhs, Setup& rhs) {
	lhs.swap(rhs);
}

}

#endif // QTDATASYNC_SETUP_H
