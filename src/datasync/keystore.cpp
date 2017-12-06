#include "keystore.h"
#include "cryptocontroller_p.h"
using namespace QtDataSync;

KeyStore::KeyStore(QObject *parent) :
	QObject(parent)
{}

QStringList KeyStore::listProviders()
{
	return CryptoController::allKeys();
}

QString KeyStore::defaultProvider()
{
	return QStringLiteral("plain");
}

bool KeyStore::contains(const QByteArray &key) const
{
	Q_UNUSED(key)
	return false;
}
