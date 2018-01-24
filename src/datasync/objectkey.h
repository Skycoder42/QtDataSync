#ifndef QTDATASYNC_OBJECTKEY_H
#define QTDATASYNC_OBJECTKEY_H

#include <QtCore/qmetatype.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

struct Q_DATASYNC_EXPORT ObjectKey
{
	QByteArray typeName;
	QString id;

	ObjectKey(const QByteArray &typeName = {}, const QString &id = {});

	QByteArray hashed() const;

	bool operator==(const ObjectKey &other) const;
	bool operator!=(const ObjectKey &other) const;
};

uint Q_DATASYNC_EXPORT qHash(const ObjectKey &key, uint seed = 0);
QDataStream Q_DATASYNC_EXPORT &operator<<(QDataStream &stream, const ObjectKey &key);
QDataStream Q_DATASYNC_EXPORT &operator>>(QDataStream &stream, ObjectKey &key);
QDebug Q_DATASYNC_EXPORT operator<<(QDebug debug, const ObjectKey &key);

}

Q_DECLARE_METATYPE(QtDataSync::ObjectKey)

#endif // QTDATASYNC_OBJECTKEY_H
