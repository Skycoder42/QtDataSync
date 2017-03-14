#ifndef QDATASYNC_H
#define QDATASYNC_H

#include <QtCore/qglobal.h>
#include <QtCore/qpair.h>
#include <QtCore/qobject.h>

#if defined(QT_BUILD_DATASYNC_LIB)
#	define Q_DATASYNC_EXPORT Q_DECL_EXPORT
#else
#	define Q_DATASYNC_EXPORT Q_DECL_IMPORT
#endif

namespace QtDataSync {

typedef QPair<QByteArray, QString> ObjectKey;

}

Q_DECLARE_METATYPE(QtDataSync::ObjectKey)

#endif // QDATASYNC_H
