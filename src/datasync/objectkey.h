#ifndef QTDATASYNC_OBJECTKEY_H
#define QTDATASYNC_OBJECTKEY_H

#include <QtCore/qmetatype.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

//! Defines a unique key to identify a dataset globally
struct Q_DATASYNC_EXPORT ObjectKey
{
	//! The name of the type the dataset is of
	QByteArray typeName;
	//! The id of the dataset (it's USER property)
	QString id;

	//! Default constructor, with optional parameters
	ObjectKey(const QByteArray &typeName = {}, const QString &id = {});

	//! Creates a hash of the object key to anonymize ize
	QByteArray hashed() const;

	//! Equality operator
	bool operator==(const ObjectKey &other) const;
	//! Inequality operator
	bool operator!=(const ObjectKey &other) const;
};

//! Overload of qHash to use ObjectKey with QHash
uint Q_DATASYNC_EXPORT qHash(const ObjectKey &key, uint seed = 0);
//! Stream operator to stream into a QDataStream
QDataStream Q_DATASYNC_EXPORT &operator<<(QDataStream &stream, const ObjectKey &key);
//! Stream operator to stream out of a QDataStream
QDataStream Q_DATASYNC_EXPORT &operator>>(QDataStream &stream, ObjectKey &key);
//! Stream operator to stream into a QDebug
QDebug Q_DATASYNC_EXPORT operator<<(QDebug debug, const ObjectKey &key);

}

Q_DECLARE_METATYPE(QtDataSync::ObjectKey)
Q_DECLARE_TYPEINFO(QtDataSync::ObjectKey, Q_MOVABLE_TYPE);

#endif // QTDATASYNC_OBJECTKEY_H
