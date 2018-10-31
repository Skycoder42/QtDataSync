#ifndef QTDATASYNC_ANDROIDSYNCCONTROL_H
#define QTDATASYNC_ANDROIDSYNCCONTROL_H

#include <chrono>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

#include "QtDataSyncAndroid/qtdatasyncandroid_global.h"

namespace QtDataSync {

class AndroidSyncControlPrivate;
class Q_DATASYNCANDROID_EXPORT AndroidSyncControl : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString serviceId READ serviceId WRITE setServiceId NOTIFY serviceIdChanged)
	Q_PROPERTY(qint64 delay READ delay WRITE setDelay NOTIFY delayChanged)

	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
	AndroidSyncControl(QObject *parent = nullptr);
	AndroidSyncControl(QString serviceId, QObject *parent = nullptr);
	~AndroidSyncControl() override;

	bool isValid() const;

	QString serviceId() const;
	qint64 delay() const;
	std::chrono::minutes delayMinutes() const;
	bool isEnabled() const;

	template <typename TRep, typename TPeriod>
	void setDelay(const std::chrono::duration<TRep, TPeriod> &delay);
	void setDelay(std::chrono::minutes delay);

public Q_SLOTS:
	void setServiceId(QString serviceId);
	void setDelay(qint64 delay);
	void setEnabled(bool enabled);

	bool triggerSyncNow();

Q_SIGNALS:
	void serviceIdChanged(const QString &serviceId, QPrivateSignal);
	void delayChanged(qint64 delay, QPrivateSignal);

private:
	QScopedPointer<AndroidSyncControlPrivate> d;
};

template<typename TRep, typename TPeriod>
void AndroidSyncControl::setDelay(const std::chrono::duration<TRep, TPeriod> &delay)
{
	setDelay(std::chrono::duration_cast<std::chrono::minutes>(delay));
}

}

#endif // QTDATASYNC_ANDROIDSYNCCONTROL_H
