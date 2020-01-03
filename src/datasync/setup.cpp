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

Setup Setup::fromConfig(const QString &configPath, ConfigType configType)
{
	QFile configFile{configPath};
	if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
		throw FileException{&configFile};
	return fromConfig(&configFile, configType);
}

Setup Setup::fromConfig(QIODevice *configDevice, ConfigType configType)
{
	Q_ASSERT(configDevice->isReadable());
	Setup setup;
	QString configName;
	switch (configType) {
	case ConfigType::WebConfig:
		configName = QStringLiteral("web configuration json");
		setup.d->readWebConfig(configDevice);
		break;
	case ConfigType::GoogleServicesJson:
		configName = QStringLiteral("google services configuration json");
		setup.d->readGSJsonConfig(configDevice);
		break;
	case ConfigType::GoogleServicesPlist:
		configName = QStringLiteral("google services configuration plist");
		setup.d->readGSPlistConfig(configDevice);
		break;
	}

	if (setup.d->firebase.projectId.isEmpty())
		qCWarning(logSetup).noquote() << "Unable to find the firebase project id in the" << configName << "file";
	if (setup.d->firebase.webApiKey.isEmpty())
		qCWarning(logSetup).noquote() << "Unable to find the firebase web api key in the" << configName << "file";
	if (setup.d->oAuth.clientId.isEmpty())
		qCWarning(logSetup).noquote() << "Unable to find the google OAuth client ID in the" << configName << "file";
	else {
		if (setup.d->oAuth.secret.isEmpty())
			qCWarning(logSetup).noquote() << "Found google OAuth client id, but no client secret in the" << configName << "file";
		if (setup.d->oAuth.port == 0)
			qCWarning(logSetup).noquote() << "Found google OAuth client id, but no callback port in the" << configName << "file";
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

Setup &Setup::setCloudTransformer(ICloudTransformer *transformer)
{
	if (d->transformer)
		d->transformer->deleteLater();
	d->transformer = transformer;
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
	authenticator->init(engine);

	if (transformer)
		transformer->setParent(engine);
	else
		transformer = new PlainCloudTransformer{engine};
}

void SetupPrivate::readWebConfig(QIODevice *device)
{
	QJsonParseError error;
	const auto root = QJsonDocument::fromJson(device->readAll(), &error).object();
	if (error.error != QJsonParseError::NoError) {
		if (auto fDev = qobject_cast<QFileDevice*>(device); fDev)
			throw JsonException{error, fDev};
		else
			throw JsonException{error, device};
	}

	firebase.projectId = root[QStringLiteral("projectId")].toString();
	firebase.webApiKey = root[QStringLiteral("apiKey")].toString();
	auto jAuth = root[QStringLiteral("oAuth")].toObject();
	oAuth.clientId = jAuth[QStringLiteral("clientID")].toString();
	oAuth.secret = jAuth[QStringLiteral("clientSecret")].toString();
	oAuth.port = static_cast<quint16>(jAuth[QStringLiteral("callbackPort")].toInt());
}

void SetupPrivate::readGSJsonConfig(QIODevice *device)
{
	QJsonParseError error;
	const auto root = QJsonDocument::fromJson(device->readAll(), &error).object();
	if (error.error != QJsonParseError::NoError) {
		if (auto fDev = qobject_cast<QFileDevice*>(device); fDev)
			throw JsonException{error, fDev};
		else
			throw JsonException{error, device};
	}

	// project id
	const auto project_info = root[QStringLiteral("project_info")].toObject();
	firebase.projectId = project_info[QStringLiteral("project_id")].toString();

	const auto clients = root[QStringLiteral("client")].toArray();
	for (const auto &clientV : clients) {
		const auto client = clientV.toObject();

		// api key
		const auto api_keys = client[QStringLiteral("api_key")].toArray();
		for (const auto &api_keyV : api_keys) {
			const auto api_key = api_keyV.toObject();
			firebase.webApiKey = api_key[QStringLiteral("current_key")].toString();
			if (!firebase.webApiKey.isEmpty())
				break;
		}

		// oAuth
		const auto oauth_clients = client[QStringLiteral("oauth_client")].toArray();
		for (const auto &oauth_clientV : oauth_clients) {
			const auto oauth_client = oauth_clientV.toObject();
			oAuth.clientId = oauth_client[QStringLiteral("client_id")].toString();
			oAuth.secret = oauth_client[QStringLiteral("client_secret")].toString();
			oAuth.port = static_cast<quint16>(oauth_client[QStringLiteral("callback_port")].toInt());
			if (!oAuth.clientId.isEmpty())
				break;
		}

		if (!firebase.webApiKey.isEmpty() &&
			!oAuth.clientId.isEmpty())
			break;
	}
}

void SetupPrivate::readGSPlistConfig(QIODevice *device)
{
#ifdef Q_OS_DARWIN
	if (auto fDevice = qobject_cast<QFileDevice*>(device); fDevice) {
		QSettings settings{fDevice->fileName(), QSettings::NativeFormat};
		firebase.projectId = settings.value(QStringLiteral("PROJECT_ID")).toString();
		firebase.webApiKey = settings.value(QStringLiteral("API_KEY")).toString();
		oAuth.clientId = settings.value(QStringLiteral("CLIENT_ID")).toString();
		oAuth.secret = settings.value(QStringLiteral("CLIENT_SECRET")).toString();
		oAuth.port = static_cast<quint16>(settings.value(QStringLiteral("CALLBACK_PORT")).toInt());
	} else
		throw PListException{device};
#else
	throw PListException{device};
#endif
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
