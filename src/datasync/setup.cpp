#include "setup.h"
#include "engine_p.h"
#include "enginethread.h"
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtRemoteObjects/QRemoteObjectHost>
using namespace QtDataSync;
using namespace QtDataSync::__private;

Q_LOGGING_CATEGORY(QtDataSync::logSetup, "qt.datasync.Setup")

void SetupPrivate::init(SetupExtensionPrivate *authExt, SetupExtensionPrivate *transExt)
{
	_authExt = authExt;
	_transExt = transExt;
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
	firebase.apiKey = root[QStringLiteral("apiKey")].toString();
	_authExt->extendFromWebConfig(root);
	_transExt->extendFromWebConfig(root);
	testConfigRes();
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

	// api key
	const auto clients = root[QStringLiteral("client")].toArray();
	for (const auto &clientV : clients) {
		const auto client = clientV.toObject();
		const auto api_keys = client[QStringLiteral("api_key")].toArray();
		for (const auto &api_keyV : api_keys) {
			const auto api_key = api_keyV.toObject();
			firebase.apiKey = api_key[QStringLiteral("current_key")].toString();
			if (!firebase.apiKey.isEmpty())
				break;
		}

		if (!firebase.apiKey.isEmpty())
			break;
	}

	_authExt->extendFromGSJsonConfig(root);
	_transExt->extendFromGSJsonConfig(root);
	testConfigRes();
}

void SetupPrivate::readGSPlistConfig(QIODevice *device)
{
#ifdef Q_OS_DARWIN
	if (auto fDevice = qobject_cast<QFileDevice*>(device); fDevice) {
		QSettings settings{fDevice->fileName(), QSettings::NativeFormat};
		firebase.projectId = settings.value(QStringLiteral("PROJECT_ID")).toString();
		firebase.apiKey = settings.value(QStringLiteral("API_KEY")).toString();
		_authExt->extendFromGSPlistConfig(&settings);
		_transExt->extendFromGSPlistConfig(&settings);
		testConfigRes();
	} else
		throw PListException{device};
#else
	throw PListException{device};
#endif
}

void SetupPrivate::testConfigRes()
{
	if (firebase.projectId.isEmpty())
		qCWarning(logSetup).noquote() << "Unable to find the firebase project id in the configuration file";
	if (firebase.apiKey.isEmpty())
		qCWarning(logSetup).noquote() << "Unable to find the firebase Web-API-Key id in the configuration file";
	_authExt->testConfigSatisfied(logSetup());
	_transExt->testConfigSatisfied(logSetup());
}

Engine *SetupPrivate::createEngine(QScopedPointer<QtDataSync::__private::SetupPrivate> &&self, QObject *parent)
{
	return new Engine{std::move(self), parent};
}

EngineThread *SetupPrivate::createThreadedEngine(QScopedPointer<SetupPrivate> &&self, QtDataSync::__private::SetupPrivate::ThreadInitFunc &&initFn, QObject *parent)
{
	return new EngineThread{std::move(self), std::move(initFn), parent};
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

	if (nam)
		nam->setParent(engine);
	else {
		nam = new QNetworkAccessManager{engine};
		nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
		nam->setStrictTransportSecurityEnabled(true);
	}

	if (roNode)
		roNode->setParent(engine);
	else if (roUrl.isValid())
		roNode = new QRemoteObjectHost{roUrl, engine};
}

IAuthenticator *SetupPrivate::createAuthenticator(QObject *parent)
{
	const auto authenticator = qobject_cast<IAuthenticator*>(_authExt->createInstance(parent));
	if (authenticator)
		return authenticator;
	else
		throw SetupException{QStringLiteral("Setup authentication extender did not create a valid FirebaseAuthenticator instance")};
}

ICloudTransformer *SetupPrivate::createTransformer(QObject *parent)
{
	const auto transformer = qobject_cast<ICloudTransformer*>(_transExt->createInstance(parent));
	if (transformer)
		return transformer;
	else
		throw SetupException{QStringLiteral("Setup transformation extender did not create a valid FirebaseAuthenticator instance")};
}





SetupExtensionPrivate::SetupExtensionPrivate() = default;

SetupExtensionPrivate::SetupExtensionPrivate(SetupExtensionPrivate &&other) noexcept = default;

SetupExtensionPrivate &SetupExtensionPrivate::operator=(SetupExtensionPrivate &&other) noexcept = default;

SetupExtensionPrivate::~SetupExtensionPrivate() = default;

void SetupExtensionPrivate::extendFromWebConfig(const QJsonObject &) {}

void SetupExtensionPrivate::extendFromGSJsonConfig(const QJsonObject &) {}

void SetupExtensionPrivate::extendFromGSPlistConfig(QSettings *) {}

void SetupExtensionPrivate::testConfigSatisfied(const QLoggingCategory &) {}



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
