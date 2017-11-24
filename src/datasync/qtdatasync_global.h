#ifndef QTDATASYNC_GLOBAL_H
#define QTDATASYNC_GLOBAL_H

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

#if defined(QT_BUILD_DATASYNC_LIB)
#	define Q_DATASYNC_EXPORT Q_DECL_EXPORT
#else
#	define Q_DATASYNC_EXPORT Q_DECL_IMPORT
#endif

namespace QtDataSync {
//! The default setup name
extern Q_DATASYNC_EXPORT const QString DefaultSetup;
}

#endif // QTDATASYNC_GLOBAL_H
