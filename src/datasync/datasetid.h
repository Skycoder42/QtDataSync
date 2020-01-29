#ifndef QTDATASYNC_DATASETID_H
#define QTDATASYNC_DATASETID_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

namespace QtDataSync {

//! Defines a unique key to identify a dataset globally
struct Q_DATASYNC_EXPORT DatasetId
{
	//! The name of the type the dataset is of
	QString tableName;
	//! The id of the dataset (it's USER property)
	QString key;

	bool isValid() const;
	QString toString() const;

	//! Equality operator
	bool operator==(const DatasetId &other) const;
	//! Inequality operator
	bool operator!=(const DatasetId &other) const;

	inline operator QVariant() const {
		return QVariant::fromValue(*this);
	}
};

//! Overload of qHash to use DatasetId with QHash
uint Q_DATASYNC_EXPORT qHash(const DatasetId &key, uint seed = 0);
//! Stream operator to stream into a QDebug
QDebug Q_DATASYNC_EXPORT operator<<(QDebug debug, const DatasetId &key);

}

Q_DECLARE_METATYPE(QtDataSync::DatasetId)
Q_DECLARE_TYPEINFO(QtDataSync::DatasetId, Q_MOVABLE_TYPE);

#endif // QTDATASYNC_DATASETID_H
