#ifndef QTDATASYNC_ANDROIDSYNCCONTROL_H
#define QTDATASYNC_ANDROIDSYNCCONTROL_H

#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>

#include "QtDataSyncAndroid/qtdatasyncandroid_global.h"

namespace QtDataSync {

class AndroidSyncControlData;
class Q_DATASYNCANDROID_EXPORT AndroidSyncControl
{
	Q_GADGET

	Q_PROPERTY(QString serviceId READ serviceId WRITE setServiceId)
	Q_PROPERTY(qint64 delay READ delay WRITE setDelay)

	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
	AndroidSyncControl();
	AndroidSyncControl(const AndroidSyncControl &other);
	AndroidSyncControl(AndroidSyncControl &&other) noexcept;
	AndroidSyncControl& operator=(const AndroidSyncControl &other);
	AndroidSyncControl& operator=(AndroidSyncControl &&other) noexcept;
	~AndroidSyncControl();

	bool isValid() const;
	explicit operator bool() const;
	bool operator!() const;

	bool operator==(const AndroidSyncControl &other) const;
	bool operator!=(const AndroidSyncControl &other) const;

	QString serviceId() const;
	qint64 delay() const;
	bool isEnabled() const;

	void setServiceId(QString serviceId);
	void setDelay(qint64 delay);
	void setEnabled(bool enabled);

private:
	QSharedDataPointer<AndroidSyncControlData> d;
};

}

Q_DECLARE_METATYPE(QtDataSync::AndroidSyncControl)

#endif // QTDATASYNC_ANDROIDSYNCCONTROL_H
