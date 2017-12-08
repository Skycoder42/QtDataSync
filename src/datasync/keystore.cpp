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
	auto kwallet = QStringLiteral("kwallet");
	if(CryptoController::allKeys().contains(kwallet))
		return kwallet;
#endif
	return QStringLiteral("plain");
}
