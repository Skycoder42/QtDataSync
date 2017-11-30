#include "qtdatasync_global.h"
#include "objectkey.h"
#include "changecontroller_p.h"

#include <QtCore/QCoreApplication>

const QString QtDataSync::DefaultSetup(QStringLiteral("default"));

static void setupQtDataSync()
{
	qRegisterMetaType<QtDataSync::ObjectKey>();
	qRegisterMetaType<QtDataSync::ChangeController::ChangeInfo>();
	qRegisterMetaTypeStreamOperators<QtDataSync::ObjectKey>();
}
Q_COREAPP_STARTUP_FUNCTION(setupQtDataSync)
