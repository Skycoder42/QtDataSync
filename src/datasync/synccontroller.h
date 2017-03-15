#ifndef SYNCCONTROLLER_H
#define SYNCCONTROLLER_H

#include "QtDataSync/qdatasync_global.h"

#include <QtCore/qobject.h>

#include <functional>

namespace QtDataSync {

class SyncControllerPrivate;
class Q_DATASYNC_EXPORT SyncController : public QObject
{
	Q_OBJECT

	Q_PROPERTY(SyncState syncState READ syncState NOTIFY syncStateChanged)
	Q_PROPERTY(QString authenticationError READ authenticationError NOTIFY authenticationErrorChanged)

public:
	enum SyncState {
		Loading,
		Disconnected,
		Syncing,
		Synced,
		SyncedWithErrors
	};
	Q_ENUM(SyncState)

	explicit SyncController(QObject *parent = nullptr);
	explicit SyncController(const QString &setupName, QObject *parent = nullptr);
	~SyncController();

	SyncState syncState() const;
	QString authenticationError() const;

public Q_SLOTS:
	void triggerSync();
	void triggerSyncWithResult(std::function<void(SyncState)> resultFn);
	void triggerResync();
	void triggerResyncWithResult(std::function<void(SyncState)> resultFn);

Q_SIGNALS:
	void syncStateChanged(SyncState syncState);
	void authenticationErrorChanged(const QString &authenticationError);

private:
	QScopedPointer<SyncControllerPrivate> d;

	void setupTriggerResult(std::function<void(SyncState)> resultFn);
};

}

#endif // SYNCCONTROLLER_H
