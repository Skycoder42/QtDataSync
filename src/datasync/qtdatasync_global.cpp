#include "qtdatasync_global.h"
#include "objectkey.h"

#include <QtCore/QCoreApplication>

static void setupQtDataSync()
{
	qRegisterMetaType<QtDataSync::ObjectKey>();
	qRegisterMetaTypeStreamOperators<QtDataSync::ObjectKey>();
}
Q_COREAPP_STARTUP_FUNCTION(setupQtDataSync)
