#ifndef QTDATASYNC_CRYPTO_KEYMANAGER_P_H
#define QTDATASYNC_CRYPTO_KEYMANAGER_P_H

#include "keymanager.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync::Crypto {

class Q_DATASYNC_CRYPTO_EXPORT KeyManagerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(KeyManager)

public:
};

}

#endif // QTDATASYNC_CRYPTO_KEYMANAGER_P_H
