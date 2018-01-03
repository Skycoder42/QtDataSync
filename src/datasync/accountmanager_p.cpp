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
		emit accountExportError(id, tr("Failed to generate export data. Make shure you are connected and synchronized with the server"));
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
		emit accountExportError(id, tr("Failed to generate export data. Make shure you are connected and synchronized with the server"));
	}
}

void AccountManagerPrivate::importAccount(quint32 id, const QJsonObject &importData, bool keepData)
{

}

void AccountManagerPrivate::replyToLogin(const QUuid &deviceId, bool accept)
{

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
