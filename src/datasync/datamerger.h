#ifndef DATAMERGER_H
#define DATAMERGER_H

#include "QtDataSync/qdatasync_global.h"
#include "QtDataSync/defaults.h"
#include "QtDataSync/stateholder.h"

#include <QtCore/qjsonobject.h>
#include <QtCore/qobject.h>

namespace QtDataSync {

class DataMergerPrivate;
class Q_DATASYNC_EXPORT DataMerger : public QObject
{
	Q_OBJECT

	Q_PROPERTY(SyncPolicy syncPolicy READ syncPolicy WRITE setSyncPolicy)
	Q_PROPERTY(MergePolicy mergePolicy READ mergePolicy WRITE setMergePolicy)
	Q_PROPERTY(bool repeatFailed READ repeatFailed WRITE setRepeatFailed)

public:
	enum SyncPolicy {
		PreferUpdated,
		PreferDeleted,
		PreferLocal,
		PreferRemote
	};
	Q_ENUM(SyncPolicy)

	enum MergePolicy {
		KeepLocal,
		KeepRemote,
		Merge
	};
	Q_ENUM(MergePolicy)

	explicit DataMerger(QObject *parent = nullptr);
	~DataMerger();

	virtual void initialize(Defaults *defaults);
	virtual void finalize();

	SyncPolicy syncPolicy() const;
	MergePolicy mergePolicy() const;
	bool repeatFailed() const;

	virtual QJsonObject merge(QJsonObject local, QJsonObject remote);

public Q_SLOTS:
	void setSyncPolicy(SyncPolicy syncPolicy);
	void setMergePolicy(MergePolicy mergePolicy);
	void setRepeatFailed(bool repeatFailed);

private:
	QScopedPointer<DataMergerPrivate> d;
};

}

#endif // DATAMERGER_H
