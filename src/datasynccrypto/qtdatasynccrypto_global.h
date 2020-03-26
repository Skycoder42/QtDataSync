#ifndef QTDATASYNC_CRYPTO_GLOBAL_H
#define QTDATASYNC_CRYPTO_GLOBAL_H

#include <QtCore/qglobal.h>

#ifndef QT_STATIC
#  if defined(QT_BUILD_DATASYNCCRYPTO_LIB)
#    define Q_DATASYNC_CRYPTO_EXPORT Q_DECL_EXPORT
#  else
#    define Q_DATASYNC_CRYPTO_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_DATASYNC_CRYPTO_EXPORT
#endif

namespace QtDataSync::Crypto {

}

#endif // QTDATASYNC_CRYPTO_GLOBAL_H
