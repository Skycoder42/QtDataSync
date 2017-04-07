#ifndef QTDATASYNC_QDATASYNC_H
#define QTDATASYNC_QDATASYNC_H

#include <QtCore/qglobal.h>
#include <QtCore/qpair.h>
#include <QtCore/qobject.h>

#if defined(QT_BUILD_DATASYNC_LIB)
#	define Q_DATASYNC_EXPORT Q_DECL_EXPORT
#else
#	define Q_DATASYNC_EXPORT Q_DECL_IMPORT
#endif

//! The datasync main namespace
namespace QtDataSync {

//! A typedef for an objects key, consisting of type name and key
typedef QPair<QByteArray, QString> ObjectKey;

}

Q_DECLARE_METATYPE(QtDataSync::ObjectKey)

#endif // QTDATASYNC_QDATASYNC_H
