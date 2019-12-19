#include "setup.h"
#include "setup_p.h"
#include "exception.h"
#include "engine_p.h"
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logSetup, "qt.datasync.Setup")

Setup::Setup() :
	d{new SetupPrivate{}}
{}

Setup::Setup(Setup &&other) noexcept
{
	d.swap(other.d);
}

Setup &Setup::operator=(Setup &&other) noexcept
{
	d.swap(other.d);
	return *this;
}

Setup::~Setup() = default;

void Setup::swap(Setup &other)
{
	d.swap(other.d);
}

Setup Setup::fromConfig(const QString &configPath)
{
	QFile configFile{configPath};
	if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
		throw FileException{&configFile};
	return fromConfig(&configFile);
}

Setup Setup::fromConfig(QIODevice *configDevice)
{
	Q_ASSERT(configDevice->isReadable());
	QJsonParseError error;
	const auto root = QJsonDocument::fromJson(configDevice->readAll(), &error).object();
	if (error.error != QJsonParseError::NoError) {
		if (auto fDev = qobject_cast<QFileDevice*>(configDevice); fDev)
			throw JsonException{error, fDev};
		else
			throw JsonException{error, configDevice};
	}

	Setup setup;
	const auto &d = setup.d;

	// project id
	const auto project_info = root[QStringLiteral("project_info")].toObject();
	d->firebase.projectId = project_info[QStringLiteral("project_id")].toString();
	if (d->firebase.projectId.isEmpty())
		qCWarning(logSetup) << "Unable to find the firebase project id in the google services configuration file";

	const auto clients = root[QStringLiteral("client")].toArray();
	for (const auto &clientV : clients) {
		const auto client = clientV.toObject();

		// api key
		const auto api_keys = client[QStringLiteral("api_key")].toArray();
		for (const auto &api_keyV : api_keys) {
			const auto api_key = api_keyV.toObject();
			d->firebase.webApiKey = api_key[QStringLiteral("current_key")].toString();
			if (!d->firebase.webApiKey.isEmpty())
				break;
		}

		// oAuth
		const auto oauth_clients = client[QStringLiteral("oauth_client")].toArray();
		for (const auto &oauth_clientV : oauth_clients) {
			const auto oauth_client = oauth_clientV.toObject();
			d->oAuth.clientId = oauth_client[QStringLiteral("client_id")].toString();
			d->oAuth.secret = oauth_client[QStringLiteral("client_secret")].toString();
			d->oAuth.port = static_cast<quint16>(oauth_client[QStringLiteral("callback_port")].toInt());
			if (!d->oAuth.clientId.isEmpty())
				break;
		}

		if (!d->firebase.webApiKey.isEmpty() &&
			!d->oAuth.clientId.isEmpty())
			break;
	}
	if (d->firebase.webApiKey.isEmpty())
		qCWarning(logSetup) << "Unable to find the firebase web api key in the google services configuration file";
	if (d->oAuth.clientId.isEmpty())
		qCWarning(logSetup) << "Unable to find the google OAuth 'client_id' in the google services configuration file";
	else {
		if (d->oAuth.secret.isEmpty())
			qCWarning(logSetup) << "Found google OAuth client id, but no 'client_secret' in the google services configuration file";
		if (d->oAuth.port == 0)
			qCWarning(logSetup) << "Found google OAuth client id, but no 'callback_port' in the google services configuration file";
	}

	return setup;
}

Setup &Setup::setSettings(QSettings *settings)
{
	d->settings = settings;
	return *this;
}

Setup &Setup::setFirebaseProjectId(QString projectId)
{
	d->firebase.projectId = std::move(projectId);
	return *this;
}

Setup &Setup::setFirebaseWebApiKey(QString webApiKey)
{
	d->firebase.webApiKey = std::move(webApiKey);
	return *this;
}

Setup &Setup::setOAuthClientCallbackPort(quint16 port)
{
	d->oAuth.port = port;
	return *this;
}

Setup &Setup::setOAuthClientId(QString clientId)
{
	d->oAuth.clientId = std::move(clientId);
	return *this;
}

Setup &Setup::setOAuthClientSecret(QString secret)
{
	d->oAuth.secret = std::move(secret);
	return *this;

}

Setup &Setup::setAuthenticator(IAuthenticator *authenticator)
{
	if (d->authenticator)
		d->authenticator->deleteLater();
	d->authenticator = authenticator;
	return *this;
}

Setup &Setup::setRemoteReadTimeout(std::chrono::milliseconds ms)
{
	d->firebase.readTimeOut = std::move(ms);
	return *this;
}

Setup &Setup::setRemotePageLimit(int limit)
{
	d->firebase.readLimit = limit;
	return *this;
}

#ifndef QTDATASYNC_NO_NTP
Setup &Setup::enableNtpSync(QString hostName, quint16 port)
{
	d->ntpAddress = std::move(hostName);
	d->ntpPort = port;
	return *this;
}
#endif

Engine *Setup::createEngine(QObject *parent)
{
	return new Engine{std::move(d), parent};
}



void SetupPrivate::finializeForEngine(Engine *engine)
{
	if (settings)
		settings->setParent(engine);
	else {
		settings = new QSettings{engine};
		settings->beginGroup(QStringLiteral("qtdatasync"));
	}
	qCDebug(logSetup).noquote().nospace() << "Using settings: " << settings->fileName()
										  << " (" << settings->group() << ")";

	if (authenticator)
		authenticator->setParent(engine);
	else
		authenticator = new OAuthAuthenticator{engine};
}



SetupException::SetupException(QString error) :
	Exception{},
	_error{std::move(error)}
{}

QString SetupException::qWhat() const
{
	return _error;
}

void SetupException::raise() const
{
	throw *this;
}

ExceptionBase *SetupException::clone() const
{
	return new SetupException{*this};
}



SetupLockedException::SetupLockedException(QLockFile *lock) :
	SetupException{{}}
{
	if (lock->getLockInfo(&_pid, &_hostname, &_appname)) {
		_error = QStringLiteral("Storage directory is already beeing used by application \"%1\" with PID %2 (host: %3)")
					 .arg(_appname)
					 .arg(_pid)
					 .arg(_hostname);
	} else
		_error = QStringLiteral("Storage directory is already beeing used by another application");
}

qint64 SetupLockedException::pid() const
{
	return _pid;
}

QString SetupLockedException::hostname() const
{
	return _hostname;
}

QString SetupLockedException::appname() const
{
	return _appname;
}

void SetupLockedException::raise() const
{
	throw *this;
}

ExceptionBase *SetupLockedException::clone() const
{
	return new SetupLockedException{*this};
}
