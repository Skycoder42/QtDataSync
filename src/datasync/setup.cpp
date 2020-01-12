#include "setup.h"
#include "setup_impl.h"
#include "exception.h"
#include "engine_p.h"
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logSetup, "qt.datasync.Setup")

Engine *SetupPrivate::createEngine(FirebaseAuthenticator *authenticator, ICloudTransformer *transformer, QObject *parent)
{
	return nullptr; // TODO new Engine{std::move(d), parent};
}

void SetupPrivate::finializeForEngine(Engine *engine)
{
	// TODO settings created AFTER auth creation!!!
	if (settings)
		settings->setParent(engine);
	else {
		settings = new QSettings{engine};
		settings->beginGroup(QStringLiteral("qtdatasync"));
	}
	qCDebug(logSetup).noquote().nospace() << "Using settings: " << settings->fileName()
										  << " (" << settings->group() << ")";
}

void SetupPrivate::readWebConfig(QIODevice *device, const std::function<void(QJsonObject)> &extender)
{
	QJsonParseError error;
	const auto root = QJsonDocument::fromJson(device->readAll(), &error).object();
	if (error.error != QJsonParseError::NoError) {
		if (auto fDev = qobject_cast<QFileDevice*>(device); fDev)
			throw JsonException{error, fDev};
		else
			throw JsonException{error, device};
	}

//	firebase.projectId = root[QStringLiteral("projectId")].toString();
//	auth.firebaseApiKey = root[QStringLiteral("apiKey")].toString();
//	auto jAuth = root[QStringLiteral("oAuth")].toObject();
//	auth.oAuth.clientId = jAuth[QStringLiteral("clientID")].toString();
//	auth.oAuth.secret = jAuth[QStringLiteral("clientSecret")].toString();
//	auth.oAuth.port = static_cast<quint16>(jAuth[QStringLiteral("callbackPort")].toInt());
}

void SetupPrivate::readGSJsonConfig(QIODevice *device, const std::function<void(QJsonObject)> &extender)
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
//	const auto project_info = root[QStringLiteral("project_info")].toObject();
//	firebase.projectId = project_info[QStringLiteral("project_id")].toString();

//	const auto clients = root[QStringLiteral("client")].toArray();
//	for (const auto &clientV : clients) {
//		const auto client = clientV.toObject();

//		// api key
//		const auto api_keys = client[QStringLiteral("api_key")].toArray();
//		for (const auto &api_keyV : api_keys) {
//			const auto api_key = api_keyV.toObject();
//			auth.firebaseApiKey = api_key[QStringLiteral("current_key")].toString();
//			if (!auth.firebaseApiKey.isEmpty())
//				break;
//		}

//		// oAuth
//		const auto oauth_clients = client[QStringLiteral("oauth_client")].toArray();
//		for (const auto &oauth_clientV : oauth_clients) {
//			const auto oauth_client = oauth_clientV.toObject();
//			auth.oAuth.clientId = oauth_client[QStringLiteral("client_id")].toString();
//			auth.oAuth.secret = oauth_client[QStringLiteral("client_secret")].toString();
//			auth.oAuth.port = static_cast<quint16>(oauth_client[QStringLiteral("callback_port")].toInt());
//			if (!auth.oAuth.clientId.isEmpty())
//				break;
//		}

//		if (!auth.firebaseApiKey.isEmpty() &&
//			!auth.oAuth.clientId.isEmpty())
//			break;
//	}
}

void SetupPrivate::readGSPlistConfig(QIODevice *device, const std::function<void(QSettings*)> &extender)
{
#ifdef Q_OS_DARWIN
	if (auto fDevice = qobject_cast<QFileDevice*>(device); fDevice) {
		QSettings settings{fDevice->fileName(), QSettings::NativeFormat};
//		firebase.projectId = settings.value(QStringLiteral("PROJECT_ID")).toString();
//		auth.firebaseApiKey = settings.value(QStringLiteral("API_KEY")).toString();
//		auth.oAuth.clientId = settings.value(QStringLiteral("CLIENT_ID")).toString();
//		auth.oAuth.secret = settings.value(QStringLiteral("CLIENT_SECRET")).toString();
//		auth.oAuth.port = static_cast<quint16>(settings.value(QStringLiteral("CALLBACK_PORT")).toInt());
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
