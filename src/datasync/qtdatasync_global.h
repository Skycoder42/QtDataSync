#ifndef QDATASYNC_GLOBAL_H
#define QDATASYNC_GLOBAL_H

#include <QtCore/qglobal.h>

#ifndef QT_STATIC
#  if defined(QT_BUILD_DATASYNC_LIB)
#    define Q_DATASYNC_EXPORT Q_DECL_EXPORT
#  else
#    define Q_DATASYNC_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_DATASYNC_EXPORT
#endif

namespace QtDataSync {

}

#endif // QDATASYNC_GLOBAL_H
