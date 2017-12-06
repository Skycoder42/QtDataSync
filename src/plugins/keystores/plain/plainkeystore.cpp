#include "plainkeystore.h"
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>

PlainKeyStore::PlainKeyStore(QObject *parent) :
	KeyStore(parent),
	_settings(new QSettings(QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
							.absoluteFilePath(QStringLiteral("plain.keystore")),
							QSettings::IniFormat,
							this))
{}

bool PlainKeyStore::canCheckContains() const
{
	return true;
}

bool PlainKeyStore::contains(const QByteArray &key) const
{
	return _settings->contains(QString::fromUtf8(key));
}

void PlainKeyStore::storeSecret(const QByteArray &key, const QByteArray &secret)
{
	_settings->setValue(QString::fromUtf8(key), secret);
	emit secretStored();
}

void PlainKeyStore::loadSecret(const QByteArray &key)
{
	auto data = _settings->value(QString::fromUtf8(key));
	if(data.isValid())
		emit secretLoaded(data.toByteArray());
	else
		emit secretNotExistant();
}
