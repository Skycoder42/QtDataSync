#include "plainkeystore.h"
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>

#include <QtDataSync/defaults.h>

PlainKeyStore::PlainKeyStore(const QtDataSync::Defaults &defaults, QObject *parent) :
	KeyStore(defaults, parent),
	_settings(nullptr)
{}

QString PlainKeyStore::providerName() const
{
	return QStringLiteral("plain");
}

void PlainKeyStore::loadStore()
{
	if(!_settings) {
		_settings = new QSettings(defaults().storageDir().absoluteFilePath(QStringLiteral("plain.keystore")),
								  QSettings::IniFormat,
								  this);
		if(_settings->status() != QSettings::NoError) {
			_settings->deleteLater();
			throw QtDataSync::KeyStoreException(this, QStringLiteral("Keystore file is not accessible"));
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
	_settings->sync();
	if(_settings->status() != QSettings::NoError)
		throw QtDataSync::KeyStoreException(this, QStringLiteral("Failed to write to keystore file"));
}

QByteArray PlainKeyStore::loadPrivateKey(const QString &key)
{
	return _settings->value(key).toByteArray();
}

void PlainKeyStore::remove(const QString &key)
{
	_settings->remove(key);
	_settings->sync();
	if(_settings->status() != QSettings::NoError)
		throw QtDataSync::KeyStoreException(this, QStringLiteral("Failed to delete from keystore file"));
}
