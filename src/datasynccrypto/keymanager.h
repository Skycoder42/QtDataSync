#ifndef QTDATASYNC_CRYPTO_KEYMANAGER_H
#define QTDATASYNC_CRYPTO_KEYMANAGER_H

#include "QtDataSyncCrypto/qtdatasynccrypto_global.h"
#include "QtDataSyncCrypto/securebytearray.h"

#include <QtCore/qobject.h>

namespace QtDataSync::Crypto {

class Q_DATASYNC_CRYPTO_EXPORT KeyProvider
{
public:
	void loadKey(std::function<void(const SecureByteArray &)> loadedCallback,
				 std::function<void()> failureCallback);
};

class KeyManagerPrivate;
class Q_DATASYNC_CRYPTO_EXPORT KeyManager : public QObject
{
	Q_OBJECT

public:
	explicit KeyManager(QObject *parent = nullptr);

private:
	Q_DECLARE_PRIVATE(KeyManager)
};

}

#endif // QTDATASYNC_CRYPTO_KEYMANAGER_H
