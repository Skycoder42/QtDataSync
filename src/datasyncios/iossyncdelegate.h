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
//! A helper class to perform background synchronization on Ios
class Q_DATASYNCIOS_EXPORT IosSyncDelegate : public QObject
{
	Q_OBJECT

	//! The interval in minutes between synchronizations
	Q_PROPERTY(qint64 interval READ interval WRITE setInterval NOTIFY intervalChanged)
	//! Specify if background synchronization is enabled or not
	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

	//! Specify whether the service should wait for a full synchronization or only the download
	Q_PROPERTY(bool waitFullSync READ waitFullSync WRITE setWaitFullSync NOTIFY waitFullSyncChanged)
	//! Persist the state of properties so that on a restart of the application they can be restored
	Q_PROPERTY(bool persistState READ persistState CONSTANT)

public:
	//! The possible results of a synchronization attempt
	enum class SyncResult {
		NewData, //!< Synchronization was successful and new data was downloaded
		NoData, //!< Synchronization was successful but no new data was downloaded
		Error //!< Synchronization failed
	};
	Q_ENUM(SyncResult)

	//! Registers a sync delegate to let it perform synchronization if needed
	static void init(IosSyncDelegate *delegate = nullptr);
	//! Returns the currently registered sync delegate
	static IosSyncDelegate *currentDelegate();

	//! Default constructor
	explicit IosSyncDelegate(QObject *parent = nullptr);
	//! Default constructor with settings to use for state restoration
	explicit IosSyncDelegate(QSettings *settings, QObject *parent = nullptr);
	~IosSyncDelegate() override;

	//! @readAcFn{IosSyncDelegate::interval}
	qint64 interval() const;
	//! @readAcFn{IosSyncDelegate::interval}
	std::chrono::minutes intervalMinutes() const;
	//! @readAcFn{IosSyncDelegate::enabled}
	bool isEnabled() const;
	//! @readAcFn{IosSyncDelegate::waitFullSync}
	bool waitFullSync() const;
	//! @readAcFn{IosSyncDelegate::persistState}
	virtual bool persistState() const;

	//! @writeAcFn{IosSyncDelegate::interval}
	template <typename TRep, typename TPeriod>
	void setInterval(const std::chrono::duration<TRep, TPeriod> &interval);
	//! @writeAcFn{IosSyncDelegate::interval}
	void setInterval(std::chrono::minutes interval);

public Q_SLOTS:
	//! @writeAcFn{IosSyncDelegate::interval}
	void setInterval(qint64 interval);
	//! @writeAcFn{IosSyncDelegate::enabled}
	void setEnabled(bool enabled);
	//! @writeAcFn{IosSyncDelegate::waitFullSync}
	void setWaitFullSync(bool waitFullSync);

Q_SIGNALS:
	//! @notifyAcFn{IosSyncDelegate::interval}
	void intervalChanged(qint64 interval, QPrivateSignal);
	//! @notifyAcFn{IosSyncDelegate::enabled}
	void enabledChanged(bool enabled, QPrivateSignal);
	//! @notifyAcFn{IosSyncDelegate::waitFullSync}
	void waitFullSyncChanged(bool waitFullSync, QPrivateSignal);

protected:
	//! Returns the name of the setup to be synchronized
	virtual QString setupName() const;
	//! Prepares the setup to be synchronized
	virtual void prepareSetup(Setup &setup);
	//! Is called by the service as soon the the synchronization has been completed
	virtual SyncResult onSyncCompleted(SyncManager::SyncState state);

	//! Is called by Ios to start the background synchronization
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
