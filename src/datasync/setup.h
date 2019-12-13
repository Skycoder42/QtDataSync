#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/engine.h"

#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>

namespace QtDataSync {

class IAuthenticator;

class SetupPrivate;
class Q_DATASYNC_EXPORT Setup
{
	Q_DISABLE_COPY(Setup)

public:
	Setup();
	Setup(Setup &&other) noexcept;
	Setup &operator=(Setup &&other) noexcept;
	~Setup();

	void swap(Setup &other);

	static Setup fromConfig(const QString &configPath);
	static Setup fromConfig(QIODevice *configDevice);

	Setup &setFirebaseProjectId(QString projectId);
	Setup &setFirebaseWebApiKey(QString webApiKey);

	Setup &setOAuthClientCallbackPort(quint16 port);
	Setup &setOAuthClientId(QString clientId);
	Setup &setOAuthClientSecret(QString secret);

	Setup &setAuthenticator(IAuthenticator *authenticator);

	Engine *createEngine(QObject *parent = nullptr);

private:
	QScopedPointer<SetupPrivate> d;
};

inline Q_DATASYNC_EXPORT void swap(Setup& lhs, Setup& rhs) {
	lhs.swap(rhs);
}

}

#endif // QTDATASYNC_SETUP_H
