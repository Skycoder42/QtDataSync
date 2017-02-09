#ifndef DATAMERGER_H
#define DATAMERGER_H

#include "qtdatasync_global.h"
#include <QJsonObject>
#include <QObject>
#include "stateholder.h"

namespace QtDataSync {

class QTDATASYNCSHARED_EXPORT DataMerger : public QObject
{
	Q_OBJECT

	Q_PROPERTY(SyncPolicy syncPolicy READ syncPolicy WRITE setSyncPolicy)
	Q_PROPERTY(MergePolicy mergePolicy READ mergePolicy WRITE setMergePolicy)

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

	virtual void initialize();
	virtual void finalize();

	SyncPolicy syncPolicy() const;
	MergePolicy mergePolicy() const;

	virtual QJsonObject merge(QJsonObject local, QJsonObject remote);

public slots:
	void setSyncPolicy(SyncPolicy syncPolicy);
	void setMergePolicy(MergePolicy mergePolicy);

private:
	SyncPolicy _syncPolicy;
	MergePolicy _mergePolicy;
};

}

#endif // DATAMERGER_H
