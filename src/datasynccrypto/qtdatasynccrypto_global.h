#ifndef QTDATASYNCCRYPTO_GLOBAL_H
#define QTDATASYNCCRYPTO_GLOBAL_H

#include <QtCore/qglobal.h>

#ifndef QT_STATIC
#  if defined(QT_BUILD_DATASYNCCRYPTO_LIB)
#    define Q_DATASYNCCRYPTO_EXPORT Q_DECL_EXPORT
#  else
#    define Q_DATASYNCCRYPTO_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_DATASYNCCRYPTO_EXPORT
#endif

namespace QtDataSync::Crypto {

}

#endif // QTDATASYNCCRYPTO_GLOBAL_H
