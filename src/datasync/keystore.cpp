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
#ifdef Q_OS_LINUX
	return QStringLiteral("kwallet");
#else
	return QStringLiteral("plain");
#endif
}
