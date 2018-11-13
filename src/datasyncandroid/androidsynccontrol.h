#ifndef QTDATASYNC_ANDROIDSYNCCONTROL_H
#define QTDATASYNC_ANDROIDSYNCCONTROL_H

#include <chrono>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

#include "QtDataSyncAndroid/qtdatasyncandroid_global.h"

namespace QtDataSync {

class AndroidSyncControlPrivate;
//! A class to manage background synchronization for android
class Q_DATASYNCANDROID_EXPORT AndroidSyncControl : public QObject
{
	Q_OBJECT

	//! Holds whether the sync control is able to configure background sync
	Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)

	//! The JAVA class name of the service to be used to do the background sync
	Q_PROPERTY(QString serviceId READ serviceId WRITE setServiceId NOTIFY serviceIdChanged)
	//! The interval in minutes between synchronizations
	Q_PROPERTY(qint64 interval READ interval WRITE setInterval NOTIFY intervalChanged)

	//! Specify if background synchronization is enabled or not
	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
	//! Default constructor
	explicit AndroidSyncControl(QObject *parent = nullptr);
	//! Constructor with a service class
	AndroidSyncControl(QString serviceId, QObject *parent = nullptr);
	~AndroidSyncControl() override;

	//! @readAcFn{AndroidSyncControl::valid}
	bool isValid() const;

	//! @readAcFn{AndroidSyncControl::serviceId}
	QString serviceId() const;
	//! @readAcFn{AndroidSyncControl::interval}
	qint64 interval() const;
	//! @readAcFn{AndroidSyncControl::interval}
	std::chrono::minutes intervalMinutes() const;
	//! @readAcFn{AndroidSyncControl::enabled}
	bool isEnabled() const;

	//! @writeAcFn{AndroidSyncControl::interval}
	template <typename TRep, typename TPeriod>
	void setInterval(const std::chrono::duration<TRep, TPeriod> &interval);
	//! @writeAcFn{AndroidSyncControl::interval}
	void setInterval(std::chrono::minutes interval);

public Q_SLOTS:
	//! @writeAcFn{AndroidSyncControl::serviceId}
	void setServiceId(QString serviceId);
	//! @writeAcFn{AndroidSyncControl::interval}
	void setInterval(qint64 interval);
	//! @writeAcFn{AndroidSyncControl::enabled}
	void setEnabled(bool enabled);

	//! Immediatly triggers a background synchronization
	bool triggerSyncNow();

Q_SIGNALS:
	//! @notifyAcFn{AndroidSyncControl::valid}
	void validChanged(QPrivateSignal);
	//! @notifyAcFn{AndroidSyncControl::serviceId}
	void serviceIdChanged(const QString &serviceId, QPrivateSignal);
	//! @notifyAcFn{AndroidSyncControl::interval}
	void intervalChanged(qint64 interval, QPrivateSignal);

private:
	QScopedPointer<AndroidSyncControlPrivate> d;
};

template<typename TRep, typename TPeriod>
void AndroidSyncControl::setInterval(const std::chrono::duration<TRep, TPeriod> &interval)
{
	setInterval(std::chrono::duration_cast<std::chrono::minutes>(interval));
}

}

#endif // QTDATASYNC_ANDROIDSYNCCONTROL_H
