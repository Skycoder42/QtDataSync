#ifndef QTDATASYNC_SYNCCONTROLLER_P_H
#define QTDATASYNC_SYNCCONTROLLER_P_H

#include "qtdatasync_global.h"
#include "controller_p.h"
#include "localstore_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SyncController : public Controller
{
	Q_OBJECT

public:
	explicit SyncController(const Defaults &defaults, QObject *parent = nullptr);

	void initialize(const QVariantHash &params) override;

public Q_SLOTS:
	void setSyncEnabled(bool enabled);
	void syncChange(quint64 key, const QByteArray &changeData);

Q_SIGNALS:
	void syncDone(quint64 key);

private:
	LocalStore *_store;
	bool _enabled;
};

}

#endif // QTDATASYNC_SYNCCONTROLLER_P_H
