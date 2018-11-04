#ifndef QTDATASYNC_IOSSYNCDELEGATE_H
#define QTDATASYNC_IOSSYNCDELEGATE_H

#include <chrono>
#include <functional>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qsettings.h>

#include <QtDataSync/setup.h>
#include <QtDataSync/syncmanager.h>

#include "QtDataSyncIos/qtdatasyncios_global.h"

namespace QtDataSync {

class IosSyncDelegatePrivate;
class Q_DATASYNCIOS_EXPORT IosSyncDelegate : public QObject
{
	Q_OBJECT

	Q_PROPERTY(qint64 interval READ interval WRITE setInterval NOTIFY intervalChanged)
	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

	Q_PROPERTY(bool waitFullSync READ waitFullSync WRITE setWaitFullSync NOTIFY waitFullSyncChanged)
	Q_PROPERTY(bool persistState READ persistState CONSTANT)

public:
	enum class SyncResult {
		NewData,
		NoData,
		Error
	};
	Q_ENUM(SyncResult)

	static void init(IosSyncDelegate *delegate = nullptr);
	static IosSyncDelegate *currentDelegate();

	explicit IosSyncDelegate(QObject *parent = nullptr);
	explicit IosSyncDelegate(QSettings *settings, QObject *parent = nullptr);
	~IosSyncDelegate() override;

	qint64 interval() const;
	std::chrono::minutes intervalMinutes() const;
	bool isEnabled() const;
	bool waitFullSync() const;
	virtual bool persistState() const;

	template <typename TRep, typename TPeriod>
	void setInterval(const std::chrono::duration<TRep, TPeriod> &interval);
	void setInterval(std::chrono::minutes interval);

public Q_SLOTS:
	void setInterval(qint64 interval);
	void setEnabled(bool enabled);
	void setWaitFullSync(bool waitFullSync);

Q_SIGNALS:
	void intervalChanged(qint64 interval, QPrivateSignal);
	void enabledChanged(bool enabled, QPrivateSignal);
	void waitFullSyncChanged(bool waitFullSync, QPrivateSignal);

protected:
	virtual QString setupName() const;
	virtual void prepareSetup(Setup &setup);
	virtual SyncResult onSyncCompleted(SyncManager::SyncState state);

	virtual void performSync(std::function<void(QtDataSync::IosSyncDelegate::SyncResult)> callback);

private Q_SLOTS:
	void backendReady();

private:
	friend class QtDataSync::IosSyncDelegatePrivate;
	QScopedPointer<IosSyncDelegatePrivate> d;
};

template<typename TRep, typename TPeriod>
void IosSyncDelegate::setInterval(const std::chrono::duration<TRep, TPeriod> &interval)
{
	setInterval(std::chrono::duration_cast<std::chrono::minutes>(interval));
}

}

#endif // QTDATASYNC_IOSSYNCDELEGATE_H
