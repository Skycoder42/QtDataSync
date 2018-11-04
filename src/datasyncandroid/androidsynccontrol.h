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

	Q_PROPERTY(bool valid READ isValid CONSTANT)

	Q_PROPERTY(QString serviceId READ serviceId WRITE setServiceId NOTIFY serviceIdChanged)
	Q_PROPERTY(qint64 interval READ interval WRITE setInterval NOTIFY intervalChanged)

	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
	explicit AndroidSyncControl(QObject *parent = nullptr);
	AndroidSyncControl(QString serviceId, QObject *parent = nullptr);
	~AndroidSyncControl() override;

	bool isValid() const;

	QString serviceId() const;
	qint64 interval() const;
	std::chrono::minutes intervalMinutes() const;
	bool isEnabled() const;

	template <typename TRep, typename TPeriod>
	void setInterval(const std::chrono::duration<TRep, TPeriod> &interval);
	void setInterval(std::chrono::minutes interval);

public Q_SLOTS:
	void setServiceId(QString serviceId);
	void setInterval(qint64 interval);
	void setEnabled(bool enabled);

	bool triggerSyncNow();

Q_SIGNALS:
	void serviceIdChanged(const QString &serviceId, QPrivateSignal);
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
