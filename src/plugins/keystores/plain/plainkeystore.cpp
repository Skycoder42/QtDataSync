#include "plainkeystore.h"
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>

#include <QtDataSync/defaults.h>

PlainKeyStore::PlainKeyStore(const QtDataSync::Defaults &defaults, QObject *parent) :
	KeyStore(defaults, parent),
	_settings(nullptr)
{}

void PlainKeyStore::loadStore()
{
	if(!_settings) {
		_settings = new QSettings(defaults().storageDir().absoluteFilePath(QStringLiteral("plain.keystore")),
								  QSettings::IniFormat,
								  this);
		if(!_settings->isWritable()) {
			_settings->deleteLater();
			throw QtDataSync::KeyStoreException(defaults(),
												QStringLiteral("plain"),
												QStringLiteral("Keystore file is not writable"));
		}
	}
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
	return _settings->contains(key);
}

void PlainKeyStore::storePrivateKey(const QString &key, const QByteArray &pKey)
{
	_settings->setValue(key, pKey);
}

QByteArray PlainKeyStore::loadPrivateKey(const QString &key)
{
	return _settings->value(key).toByteArray();
}

void PlainKeyStore::remove(const QString &key)
{
	_settings->remove(key);
}
