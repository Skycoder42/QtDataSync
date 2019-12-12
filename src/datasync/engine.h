#ifndef QTDATASYNC_ENGINE_H
#define QTDATASYNC_ENGINE_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qobject.h>

namespace QtDataSync {

class IAuthenticator;

class EnginePrivate;
class Q_DATASYNC_EXPORT Engine : public QObject
{
	Q_OBJECT

	Q_PROPERTY(IAuthenticator* authenticator READ authenticator CONSTANT)

public:
	explicit Engine(QObject *parent = nullptr);

	IAuthenticator *authenticator() const;

public Q_SLOTS:
	void start();

private:
	friend class Setup;
	Q_DECLARE_PRIVATE(Engine)
};

}

#endif // QTDATASYNC_ENGINE_H
