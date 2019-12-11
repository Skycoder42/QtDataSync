#ifndef QTDATASYNC_ENGINE_H
#define QTDATASYNC_ENGINE_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qobject.h>

namespace QtDataSync {

class EnginePrivate;
class Q_DATASYNC_EXPORT Engine : public QObject
{
	Q_OBJECT

public:
	explicit Engine(QObject *parent = nullptr);

public Q_SLOTS:
	void setupFirebase(const QByteArray &projectId,
					   const QByteArray &webApiKey);

private:
	Q_DECLARE_PRIVATE(Engine)
};

}

#endif // QTDATASYNC_ENGINE_H
