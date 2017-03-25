#ifndef STATEHOLDER_H
#define STATEHOLDER_H

#include "QtDataSync/qdatasync_global.h"
#include "QtDataSync/defaults.h"

#include <QtCore/qdir.h>
#include <QtCore/qhash.h>
#include <QtCore/qobject.h>

namespace QtDataSync {

//! The class responsible for storing the local change state
class Q_DATASYNC_EXPORT StateHolder : public QObject
{
	Q_OBJECT

public:
	//! Defines the possible change state of a dataset
	enum ChangeState {
		Unchanged = 0,//!< The dataset has not been changed since the last sync
		Changed = 1,//!< The dataset has been modified/created
		Deleted = 2//!< The dataset has been deleted
	};
	Q_ENUM(ChangeState)

	//! A typedef for a hash of object keys and their change state
	typedef QHash<ObjectKey, ChangeState> ChangeHash;

	//! Constructor
	explicit StateHolder(QObject *parent = nullptr);

	//! Called from the engine to initialize the state holder
	virtual void initialize(Defaults *defaults);
	//! Called from the engine to finalize the state holder
	virtual void finalize();

	//! Returns the complete local change state
	virtual ChangeHash listLocalChanges() = 0;
	//! Updates the change state of the dataset with the given key
	virtual void markLocalChanged(const ObjectKey &key, ChangeState changed) = 0;

	//! Clear the complete change state and replace it by the given keys as changed
	virtual ChangeHash resetAllChanges(const QList<ObjectKey> &changeKeys) = 0;
	//! Clear the complete change state by deleting all entries
	virtual void clearAllChanges() = 0;
};

}

Q_DECLARE_METATYPE(QtDataSync::StateHolder::ChangeHash)

#endif // STATEHOLDER_H
