#include "setup.h"
#include "setup_p.h"
#include "exception.h"
#include "engine_p.h"
#include <QtCore/QFile>
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

	const auto project_info = root[QStringLiteral("project_info")].toObject();
	const auto projectId = project_info[QStringLiteral("project_id")].toString();
	if (!projectId.isEmpty())
		setup.setFirebaseProjectId(projectId);
	else
		qCWarning(logSetup) << "Unable to find the firebase project id in the google services configuration file";

	QString webApiKey;
	const auto clients = root[QStringLiteral("client")].toArray();
	for (const auto &clientV : clients) {
		const auto client = clientV.toObject();
		const auto api_keys = client[QStringLiteral("api_key")].toArray();
		for (const auto &api_keyV : api_keys) {
			const auto api_key = api_keyV.toObject();
			webApiKey = api_key[QStringLiteral("current_key")].toString();
			if (!webApiKey.isEmpty())
				break;
		}
		if (!webApiKey.isEmpty())
			break;
	}
	if (!webApiKey.isEmpty())
		setup.setFirebaseWebApiKey(webApiKey);
	else
		qCWarning(logSetup) << "Unable to find the firebase web api key in the google services configuration file";

	return setup;
}

Setup &Setup::setFirebaseProjectId(QString projectId)
{
	d->config.projectId = std::move(projectId);
	return *this;
}

Setup &Setup::setFirebaseWebApiKey(QString webApiKey)
{
	d->config.webApiKey = std::move(webApiKey);
	return *this;
}

Setup &Setup::setOAuthClientId(QString clientId)
{
	d->config.clientId = std::move(clientId);
	return *this;
}

Setup &Setup::setOAuthClientSecret(QString secret)
{
	d->config.secret = std::move(secret);
	return *this;

}

Setup &Setup::setOAuthClientCallbackPort(quint16 port)
{
	d->config.port = port;
	return *this;
}

Setup &Setup::setAuthenticator(IAuthenticator *authenticator)
{
	if (d->authenticator)
		d->authenticator->deleteLater();
	d->authenticator = authenticator;
	return *this;
}

Engine *Setup::createEngine(QObject *parent)
{
	if (!d->authenticator)
		d->authenticator = new OAuthAuthenticator{QString::fromUtf8(d->firebaseWebApiKey)};

	auto engine = new Engine{parent};
	d->authenticator->setParent(engine);
	std::swap(engine->d_func()->authenticator, d->authenticator);
	return engine;
}
