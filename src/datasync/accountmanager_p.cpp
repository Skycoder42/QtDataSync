#include "accountmanager_p.h"

#include <QJsonDocument>

using namespace QtDataSync;
using std::get;
using std::tie;

#define QTDATASYNC_LOG _logger

AccountManagerPrivate::AccountManagerPrivate(ExchangeEngine *engineParent) :
	AccountManagerPrivateSource(engineParent),
	_engine(engineParent),
	_logger(_engine->defaults().createLogger("account", this))
{
	connect(_engine->remoteConnector(), &RemoteConnector::deviceNameChanged,
			this, &AccountManagerPrivate::deviceNameChanged);
	connect(_engine->cryptoController(), &CryptoController::fingerprintChanged,
			this, &AccountManagerPrivate::deviceFingerprintChanged);
	connect(_engine, &ExchangeEngine::lastErrorChanged,
			this, &AccountManagerPrivate::lastErrorChanged);
	connect(_engine->remoteConnector(), &RemoteConnector::devicesListed,
			this, &AccountManagerPrivate::accountDevices);
	connect(_engine->remoteConnector(), &RemoteConnector::loginRequested,
			this, &AccountManagerPrivate::requestLogin);
	connect(_engine->remoteConnector(), &RemoteConnector::importCompleted,
			this, &AccountManagerPrivate::importCompleted);
	connect(_engine->remoteConnector(), &RemoteConnector::accountAccessGranted,
			this, &AccountManagerPrivate::accountAccessGranted);
}

QString AccountManagerPrivate::deviceName() const
{
	return _engine->remoteConnector()->deviceName();
}

QByteArray AccountManagerPrivate::deviceFingerprint() const
{
	return _engine->cryptoController()->fingerprint();
}

QString AccountManagerPrivate::lastError() const
{
	return _engine->lastError();
}

void AccountManagerPrivate::setDeviceName(QString deviceName)
{
	if(deviceName.isNull())
		_engine->remoteConnector()->resetDeviceName();
	else
		_engine->remoteConnector()->setDeviceName(deviceName);
}

void AccountManagerPrivate::listDevices()
{
	_engine->remoteConnector()->listDevices();
}

void AccountManagerPrivate::removeDevice(const QUuid &deviceId)
{
	_engine->remoteConnector()->removeDevice(deviceId);
}

void AccountManagerPrivate::updateExchangeKey()
{
	_engine->remoteConnector()->initKeyUpdate();
}

void AccountManagerPrivate::resetAccount(bool keepData)
{
	_engine->resetAccount(keepData);
}

void AccountManagerPrivate::changeRemote(const RemoteConfig &config, bool keepData)
{
	_engine->remoteConnector()->changeRemote(config);
	_engine->resetAccount(keepData, false);
}

void AccountManagerPrivate::exportAccount(const QUuid &id, bool includeServer)
{
	try {
		auto data = _engine->remoteConnector()->exportAccount(includeServer, QString());
		auto json = serializeExportData(get<0>(data)); //ignore the salt and key, not needed
		emit accountExportReady(id, json);
	} catch(QException &e) {
		logWarning() << "Failed to generate export data with error:" << e.what();
		emit accountExportError(id, tr("Failed to generate export data. You must be registered to a server to export data."));
	}
}

void AccountManagerPrivate::exportAccountTrusted(const QUuid &id, bool includeServer, const QString &password)
{
	if(password.isEmpty()) {
		emit accountExportError(id, tr("Password must not be empty."));
		return;
	}

	try {
		ExportData data;
		QByteArray salt;
		CryptoPP::SecByteBlock key;
		tie(data, salt, key) = _engine->remoteConnector()->exportAccount(includeServer, password);
		auto json = serializeExportData(data);
		auto enc = _engine->cryptoController()->exportEncrypt(data.scheme, salt, key, QJsonDocument(json).toJson(QJsonDocument::Compact));
		QJsonObject res;
		res[QStringLiteral("trusted")] = true; //trusted exports are always encrypted, needed for deser
		res[QStringLiteral("scheme")] = QString::fromUtf8(data.scheme);
		res[QStringLiteral("salt")] = QString::fromUtf8(salt.toBase64());
		res[QStringLiteral("data")] = QString::fromUtf8(enc.toBase64());
		emit accountExportReady(id, res);
	} catch(QException &e) {
		logWarning() << "Failed to generate export data with error:" << e.what();
		emit accountExportError(id, tr("Failed to generate export data. You must be registered to a server to export data."));
	}
}

void AccountManagerPrivate::importAccount(const JsonObject &importData, bool keepData)
{
	try {
		auto data = deserializeExportData(importData);
		_engine->remoteConnector()->prepareImport(data, CryptoPP::SecByteBlock());
		_engine->resetAccount(keepData, false);
		emit accountImportResult(true, {});
	} catch(QException &e) {
		logWarning() << "Failed to import data with error:" << e.what();
		emit accountImportResult(false, tr("Failed import data. Data invalid."));
	}
}

void AccountManagerPrivate::importAccountTrusted(const JsonObject &importData, const QString &password, bool keepData)
{
	if(password.isEmpty()) {
		emit accountImportResult(false, tr("Password must not be empty."));
		return;
	}

	try {
		auto scheme = importData[QStringLiteral("scheme")].toString().toUtf8();
		auto salt = QByteArray::fromBase64(importData[QStringLiteral("salt")].toString().toUtf8());

		auto key = _engine->cryptoController()->recoverExportKey(scheme, salt, password);
		auto raw = _engine->cryptoController()->importDecrypt(scheme, salt, key,
															  QByteArray::fromBase64(importData[QStringLiteral("data")].toString().toUtf8()));
		QJsonParseError error;
		auto obj = QJsonDocument::fromJson(raw, &error).object();
		if(error.error != QJsonParseError::NoError)
			throw Exception(_engine->defaults(), error.errorString());
		auto data = deserializeExportData(obj);
		//verify cmac not needed here, as integrity and authenticity are already part of the encryption scheme
		_engine->remoteConnector()->prepareImport(data, key);
		_engine->resetAccount(keepData, false);
		emit accountImportResult(true, {});
	} catch(QException &e) {
		logWarning() << "Failed to decrypt import data with error:" << e.what();
		emit accountImportResult(false, tr("Failed to decrypt data. Data invalid or password wrong."));
	}
}

void AccountManagerPrivate::replyToLogin(const QUuid &deviceId, bool accept)
{
	if(_loginRequests.remove(deviceId))
		_engine->remoteConnector()->loginReply(deviceId, accept);
}

void AccountManagerPrivate::requestLogin(const DeviceInfo &deviceInfo)
{
	_loginRequests.insert(deviceInfo.deviceId());
	emit loginRequested(deviceInfo);
}

QJsonObject AccountManagerPrivate::serializeExportData(const ExportData &data) const
{
	//cant use json serializer because of HeaderHash and config ptr
	QJsonObject dJson;
	dJson[QStringLiteral("trusted")] = data.trusted;
	dJson[QStringLiteral("nonce")] = QString::fromUtf8(data.pNonce.toBase64());
	dJson[QStringLiteral("partner")] = data.partnerId.toString();
	dJson[QStringLiteral("scheme")] = QString::fromUtf8(data.scheme);
	dJson[QStringLiteral("cmac")] = QString::fromUtf8(data.cmac.toBase64());

	if(data.config) {
		QJsonObject cJson;
		cJson[QStringLiteral("url")] = data.config->url().toString();
		cJson[QStringLiteral("accessKey")] = data.config->accessKey();
		cJson[QStringLiteral("keepaliveTimeout")] = data.config->keepaliveTimeout();

		QJsonObject hJson;
		auto headers = data.config->headers();
		for(auto it = headers.constBegin(); it != headers.constEnd(); it++) {
			hJson.insert(QString::fromUtf8(it.key()),
						 QString::fromUtf8(it.value().toBase64()));
		}
		cJson[QStringLiteral("headers")] = hJson;

		dJson[QStringLiteral("config")] = cJson;
	} else
		dJson[QStringLiteral("config")] = QJsonValue::Null;

	return dJson;
}

ExportData AccountManagerPrivate::deserializeExportData(const QJsonObject &importData) const
{
	ExportData data;

	//cant use json serializer because of HeaderHash and config ptr
	data.trusted = importData[QStringLiteral("trusted")].toBool();
	data.pNonce = QByteArray::fromBase64(importData[QStringLiteral("nonce")].toString().toUtf8());
	data.partnerId = QUuid(importData[QStringLiteral("partner")].toString());
	data.scheme = importData[QStringLiteral("scheme")].toString().toUtf8();
	data.cmac = QByteArray::fromBase64(importData[QStringLiteral("cmac")].toString().toUtf8());

	if(importData[QStringLiteral("config")].isObject()) {
		auto cJson = importData[QStringLiteral("config")].toObject();

		data.config = QSharedPointer<RemoteConfig>::create();
		data.config->setUrl(QUrl(cJson[QStringLiteral("url")].toString()));
		data.config->setAccessKey(cJson[QStringLiteral("accessKey")].toString());
		data.config->setKeepaliveTimeout(cJson[QStringLiteral("keepaliveTimeout")].toInt());

		auto hJson = cJson[QStringLiteral("headers")].toObject();
		RemoteConfig::HeaderHash headers;
		for(auto it = hJson.constBegin(); it != hJson.constEnd(); it++) {
			headers.insert(it.key().toUtf8(),
						   QByteArray::fromBase64(it.value().toString().toUtf8()));
		}
		data.config->setHeaders(headers);
	}

	//quick sanity check of data
	if(data.pNonce.isEmpty() ||
	   data.partnerId.isNull() ||
	   data.scheme.isEmpty() ||
	   data.cmac.isEmpty() ||
	   (data.config && !data.config->url().isValid())) {
		throw Exception(_engine->defaults(), QStringLiteral("One or more fields contain incomplete or invalid data"));
	}

	return data;
}
