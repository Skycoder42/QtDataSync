#include "accountmanager_p.h"
using namespace QtDataSync;

AccountManagerPrivate::AccountManagerPrivate(ExchangeEngine *engineParent) :
	AccountManagerPrivateSource(engineParent),
	_engine(engineParent)
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

}

void AccountManagerPrivate::exportAccountTrusted(quint32 id, bool includeServer, const QString &password)
{

}

void AccountManagerPrivate::importAccount(quint32 id, const QByteArray &importData, bool keepData)
{

}

void AccountManagerPrivate::replyToLogin(const QUuid &deviceId, bool accept)
{

}

QByteArray AccountManagerPrivate::createExportData(bool includeServer, bool trusted)
{
	//TODO export data is: x{pnonce, deviceid, trusted}, signature(x), [server]
}



QDataStream &QtDataSync::operator<<(QDataStream &stream, const ExportData &data)
{
	stream << data.pNonce
		   << data.partnerId
		   << data.trusted
		   << data.signature;
	if(data.config) {
		stream << true
			   << *(data.config);
	} else
		stream << false;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, ExportData &data)
{
	stream.startTransaction();
	bool hasConfig;
	stream >> data.pNonce
		   >> data.partnerId
		   >> data.trusted
		   >> data.signature
		   >> hasConfig;
	if(hasConfig) {
		data.config = QSharedPointer<RemoteConfig>::create();
		stream >> *(data.config);
	} else
		data.config.reset();
	stream.commitTransaction();
	return stream;
}
