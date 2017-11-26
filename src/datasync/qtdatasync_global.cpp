#include "qtdatasync_global.h"
#include "objectkey.h"

#include <QtCore/QCoreApplication>

const QString QtDataSync::DefaultSetup(QStringLiteral("DefaultSetup"));

static void setupQtDataSync()
{
	qRegisterMetaType<QtDataSync::ObjectKey>();
	qRegisterMetaTypeStreamOperators<QtDataSync::ObjectKey>();
}
Q_COREAPP_STARTUP_FUNCTION(setupQtDataSync)
