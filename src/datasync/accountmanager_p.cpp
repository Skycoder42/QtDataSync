#include "accountmanager_p.h"

#include <QJsonDocument>

using namespace QtDataSync;

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
	connect(_engine->remoteConnector(), &RemoteConnector::devicesListed,
			this, &AccountManagerPrivate::accountDevices);
	connect(_engine->remoteConnector(), &RemoteConnector::loginRequested,
			this, &AccountManagerPrivate::requestLogin);
}

QString AccountManagerPrivate::deviceName() const
{
	return _engine->remoteConnector()->deviceName();
}

QByteArray AccountManagerPrivate::deviceFingerprint() const
{
	return _engine->cryptoController()->fingerprint();
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

void AccountManagerPrivate::updateDeviceKey()
{

}

void AccountManagerPrivate::updateExchangeKey()
{

}

void AccountManagerPrivate::resetAccount(bool keepData)
{
	_engine->resetAccount(keepData);
}

void AccountManagerPrivate::exportAccount(quint32 id, bool includeServer)
{
	try {
		emit accountExportReady(id, createExportData(false, includeServer));
	} catch(QException &e) {
		logWarning() << "Failed to generate export data with error:" << e.what();
		emit accountExportError(id, tr("Failed to generate export data. You must be registered to a server to export data."));
	}
}

void AccountManagerPrivate::exportAccountTrusted(quint32 id, bool includeServer, const QString &password)
{
	try {
		auto json = createExportData(true, includeServer);
		auto enc = _engine->cryptoController()->pwEncrypt(QJsonDocument(json).toJson(QJsonDocument::Compact), password);
		QJsonObject res;
		res[QStringLiteral("trusted")] = true; //trusted exports are always encrypted, needed for deser
		res[QStringLiteral("scheme")] = QString::fromUtf8(std::get<0>(enc));
		res[QStringLiteral("salt")] = QString::fromUtf8(std::get<1>(enc).toBase64());
		res[QStringLiteral("data")] = QString::fromUtf8(std::get<2>(enc).toBase64());
		emit accountExportReady(id, res);
	} catch(QException &e) {
		logWarning() << "Failed to generate export data with error:" << e.what();
		emit accountExportError(id, tr("Failed to generate export data. You must be registered to a server to export data."));
	}
}

void AccountManagerPrivate::importAccount(const JsonObject &importData, bool keepData)
{
	try {
		ExportData data;

		//cant use json serializer because of HeaderHash and config ptr
		data.pNonce = QByteArray::fromBase64(importData[QStringLiteral("nonce")].toString().toUtf8());
		data.partnerId = QUuid::fromString(importData[QStringLiteral("partner")].toString());
		data.trusted = importData[QStringLiteral("trusted")].toBool();
		data.signature = QByteArray::fromBase64(importData[QStringLiteral("signature")].toString().toUtf8());

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
		   data.signature.isEmpty() ||
		   (data.config && !data.config->url().isValid())) {
			throw Exception(_engine->defaults(), QStringLiteral("One or more fields contain incomplete or invalid data"));
		}

		//start the import sequence (prepare remcon, then reset)
		_engine->remoteConnector()->prepareImport(data);
		_engine->resetAccount(keepData, false);
		emit accountImportResult(true, {});
	} catch(QException &e) {
		logWarning() << "Failed to import data with error:" << e.what();
		emit accountImportResult(false, tr("Failed import data. Data invalid."));
	}
}

void AccountManagerPrivate::importAccountTrusted(const JsonObject &importData, const QString &password, bool keepData)
{
	try {
		auto raw = _engine->cryptoController()->pwDecrypt(importData[QStringLiteral("scheme")].toString().toUtf8(),
														  QByteArray::fromBase64(importData[QStringLiteral("salt")].toString().toUtf8()),
														  QByteArray::fromBase64(importData[QStringLiteral("data")].toString().toUtf8()),
														  password);
		QJsonParseError error;
		auto obj = QJsonDocument::fromJson(raw, &error).object();
		if(error.error != QJsonParseError::NoError)
			throw Exception(_engine->defaults(), error.errorString());

		importAccount(obj, keepData);
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

QJsonObject AccountManagerPrivate::createExportData(bool trusted, bool includeServer)
{
	auto data = _engine->remoteConnector()->exportAccount(trusted, includeServer);

	//cant use json serializer because of HeaderHash and config ptr
	QJsonObject dJson;
	dJson[QStringLiteral("nonce")] = QString::fromUtf8(data.pNonce.toBase64());
	dJson[QStringLiteral("partner")] = data.partnerId.toString();
	dJson[QStringLiteral("trusted")] = data.trusted;
	dJson[QStringLiteral("signature")] = QString::fromUtf8(data.signature.toBase64());

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
