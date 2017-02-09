#ifndef QTDATASYNC_GLOBAL_H
#define QTDATASYNC_GLOBAL_H

#include <QtCore/qglobal.h>

#include <QPair>
#include <QObject>

#if defined(QTDATASYNC_LIBRARY)
#  define QTDATASYNCSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QTDATASYNCSHARED_EXPORT Q_DECL_IMPORT
#endif

namespace QtDataSync {

typedef QPair<QByteArray, QString> ObjectKey;

}

Q_DECLARE_METATYPE(QtDataSync::ObjectKey)

#endif // QTDATASYNC_GLOBAL_H
