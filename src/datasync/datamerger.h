#ifndef QTDATASYNC_DATAMERGER_H
#define QTDATASYNC_DATAMERGER_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/defaults.h"
#include "QtDataSync/stateholder.h"

#include <QtCore/qjsonobject.h>
#include <QtCore/qobject.h>

namespace QtDataSync {

class DataMergerPrivate;
//! The class responsible for deciding what happens on sync conflicts
class QT_DEPRECATED Q_DATASYNC_EXPORT DataMerger : public QObject
{
	Q_OBJECT

	//! Specifies the policy regarding the handling of changed/deleted conflicts
	Q_PROPERTY(SyncPolicy syncPolicy READ syncPolicy WRITE setSyncPolicy)
	//! Specifies the policy regarding the handling of changed/changed conflicts
	Q_PROPERTY(MergePolicy mergePolicy READ mergePolicy WRITE setMergePolicy)

public:
	//! The policy for changed/deleted conflicts
	enum SyncPolicy {
		PreferUpdated,//!< Always keep the changed data
		PreferDeleted,//!< Always delete the data
		PreferLocal,//!< Prefer whatever was done locally
		PreferRemote//!< Prefer whatever state is on the remote server
	};
	Q_ENUM(SyncPolicy)

	//! The policy for changed/changed conflicts
	enum MergePolicy {
		KeepLocal,//!< Prefer the local change
		KeepRemote,//!< Prefer the change state on the remote server
		Merge//!< Call merge() to merge the data
	};
	Q_ENUM(MergePolicy)

	//! Constructor
	explicit DataMerger(QObject *parent = nullptr);
	//! Destructor
	~DataMerger();

	//! Called from the engine to initialize the merger
	virtual void initialize(Defaults *defaults);
	//! Called from the engine to finalize the merger
	virtual void finalize();

	//! @readAcFn{DataMerger::syncPolicy}
	SyncPolicy syncPolicy() const;
	//! @readAcFn{DataMerger::mergePolicy}
	MergePolicy mergePolicy() const;

	//! Called, if the merge policy is Merge and data needs to be merged
	virtual QJsonObject merge(QJsonObject local, QJsonObject remote);

public Q_SLOTS:
	//! @writeAcFn{DataMerger::syncPolicy}
	void setSyncPolicy(SyncPolicy syncPolicy);
	//! @writeAcFn{DataMerger::mergePolicy}
	void setMergePolicy(MergePolicy mergePolicy);

private:
	QScopedPointer<DataMergerPrivate> d;
};

//! Extended DataMerger with an additional parameter for merge
class Q_DATASYNC_EXPORT DataMerger2 : public DataMerger
{
	Q_OBJECT

public:
	//! Constructor
	DataMerger2(QObject *parent = nullptr);

	//! Called, if the merge policy is Merge and data needs to be merged
	virtual QJsonObject merge(QJsonObject local, QJsonObject remote, const QByteArray &typeName);

private:
	QJsonObject merge(QJsonObject local, QJsonObject remote) final;
};

}

#endif // QTDATASYNC_DATAMERGER_H
