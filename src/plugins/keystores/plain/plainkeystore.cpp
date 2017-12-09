#include "plainkeystore.h"
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>

PlainKeyStore::PlainKeyStore(QObject *parent) :
	KeyStore(parent),
	_settings(nullptr)
{}

bool PlainKeyStore::loadStore()
{
	if(!_settings) {
		_settings = new QSettings(QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
								  .absoluteFilePath(QStringLiteral("plain.keystore")),
								  QSettings::IniFormat,
								  this);
		if(!_settings->isWritable()) {
			_settings->deleteLater();
			return false;
		}
	}

	return _settings;
}

void PlainKeyStore::closeStore()
{
	if(_settings) {
		_settings->sync();
		_settings->deleteLater();
	}
}

bool PlainKeyStore::contains(const QString &key) const
{
	if(_settings)
		return _settings->contains(key);
	else
		return false;
}

bool PlainKeyStore::storePrivateKey(const QString &key, const QSslKey &pKey)
{
	if(!_settings)
		return false;
	else {
		_settings->setValue(key, pKey.toDer());
		return true;
	}
}

QSslKey PlainKeyStore::loadPrivateKey(const QString &key)
{
	if(!_settings)
		return {};
	else
		return QSslKey(_settings->value(key).toByteArray(), QSsl::Rsa, QSsl::Der);
}

bool PlainKeyStore::remove(const QString &key)
{
	if(!_settings)
		return false;
	else {
		_settings->remove(key);
		return true;
	}
}
