#ifndef STATEHOLDER_H
#define STATEHOLDER_H

#include "qtdatasync_global.h"
#include <QHash>
#include <QObject>

namespace QtDataSync {

class QTDATASYNCSHARED_EXPORT StateHolder : public QObject
{
	Q_OBJECT

public:
	enum ChangeState {
		Unchanged = 0,
		Changed = 1,
		Deleted = 2
	};
	Q_ENUM(ChangeState)

	typedef QPair<QByteArray, QString> ChangeKey;
	typedef QHash<ChangeKey, ChangeState> ChangeHash;

	explicit StateHolder(QObject *parent = nullptr);

	virtual void initialize();
	virtual void finalize();

	virtual ChangeHash listLocalChanges() = 0;
	virtual void markLocalChanged(const ChangeKey &key, ChangeState changed) = 0;
	virtual void markAllLocalChanged(const QByteArray &typeName, ChangeState changed) = 0;
};

}

Q_DECLARE_METATYPE(QtDataSync::StateHolder::ChangeKey)
Q_DECLARE_METATYPE(QtDataSync::StateHolder::ChangeHash)

#endif // STATEHOLDER_H
