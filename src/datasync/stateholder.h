#ifndef STATEHOLDER_H
#define STATEHOLDER_H

#include "QtDataSync/qdatasync_global.h"
#include "QtDataSync/defaults.h"

#include <QtCore/qdir.h>
#include <QtCore/qhash.h>
#include <QtCore/qobject.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT StateHolder : public QObject
{
	Q_OBJECT

public:
	enum ChangeState {
		Unchanged = 0,
		Changed = 1,
		Deleted = 2
	};
	Q_ENUM(ChangeState)

	typedef QHash<ObjectKey, ChangeState> ChangeHash;

	explicit StateHolder(QObject *parent = nullptr);

	virtual void initialize(Defaults *defaults);
	virtual void finalize();

	virtual ChangeHash listLocalChanges() = 0;
	virtual void markLocalChanged(const ObjectKey &key, ChangeState changed) = 0;

	virtual void resetAllChanges(const QList<ObjectKey> &changeKeys) = 0;
	virtual void clearAllChanges() = 0;
};

}

Q_DECLARE_METATYPE(QtDataSync::StateHolder::ChangeHash)

#endif // STATEHOLDER_H
