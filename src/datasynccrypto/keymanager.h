#ifndef QTDATASYNC_CRYPTO_KEYMANAGER_H
#define QTDATASYNC_CRYPTO_KEYMANAGER_H

#include "QtDataSyncCrypto/qtdatasynccrypto_global.h"

#include <QtCore/qobject.h>

namespace QtDataSync::Crypto {

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
