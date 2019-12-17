#ifndef QTDATASYNC_SETUP_H
#define QTDATASYNC_SETUP_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/engine.h"
#include "QtDataSync/exception.h"

#include <QtCore/qobject.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qsettings.h>

class QLockFile;

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

	Setup &setLocalDir(const QString &localDir);
	Setup &setSettings(QSettings *settings);

	Setup &setFirebaseProjectId(QString projectId);
	Setup &setFirebaseWebApiKey(QString webApiKey);

	Setup &setOAuthClientCallbackPort(quint16 port);
	Setup &setOAuthClientId(QString clientId);
	Setup &setOAuthClientSecret(QString secret);

	Setup &setAuthenticator(IAuthenticator *authenticator);

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

class Q_DATASYNC_EXPORT SetupLockedException : public SetupException
{
public:
	SetupLockedException(QLockFile *lock);

	qint64 pid() const;
	QString hostname() const;
	QString appname() const;

	void raise() const override;
	ExceptionBase *clone() const override;

protected:
	qint64 _pid = -1;
	QString _hostname;
	QString _appname;
};

inline Q_DATASYNC_EXPORT void swap(Setup& lhs, Setup& rhs) {
	lhs.swap(rhs);
}

}

#endif // QTDATASYNC_SETUP_H
