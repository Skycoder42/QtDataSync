#ifndef STATEHOLDER_H
#define STATEHOLDER_H

#include "qtdatasync_global.h"
#include <QObject>

namespace QtDataSync {

class QTDATASYNCSHARED_EXPORT StateHolder : public QObject
{
	Q_OBJECT

public:
	struct ChangedInfo {
		QByteArray typeName;
		QList<QString> keys;
	};

	explicit StateHolder(QObject *parent = nullptr);

	virtual void initialize();
	virtual void finalize();

	virtual QList<ChangedInfo> listLocalChanges() = 0;
	virtual void markLocalChanged(QByteArray typeName, const QString &key, bool changed) = 0;
};

}

#endif // STATEHOLDER_H
