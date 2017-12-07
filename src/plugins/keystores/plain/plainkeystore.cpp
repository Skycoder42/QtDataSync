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

bool PlainKeyStore::containsSecret(const QString &key) const
{
	if(_settings)
		return _settings->contains(key);
	else
		return false;
}

bool PlainKeyStore::storeSecret(const QString &key, const QByteArray &secret)
{
	if(!_settings)
		return false;
	else {
		_settings->setValue(key, secret);
		return true;
	}
}

QByteArray PlainKeyStore::loadSecret(const QString &key)
{
	if(!_settings)
		return {};
	else
		return _settings->value(key).toByteArray();
}

bool PlainKeyStore::removeSecret(const QString &key)
{
	if(!_settings)
		return false;
	else {
		_settings->remove(key);
		return true;
	}
}
