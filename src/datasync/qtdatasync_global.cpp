#include "qtdatasync_global.h"
#include "objectkey.h"
#include "changecontroller_p.h"

#include "threadedserver_p.h"
#include "threadedclient_p.h"

#include <QtCore/QCoreApplication>
#include "message_p.h"

const QString QtDataSync::DefaultSetup(QStringLiteral("default"));

namespace {

void setupQtDataSync()
{
	qRegisterMetaType<QtDataSync::ObjectKey>();
	qRegisterMetaType<QtDataSync::ChangeController::ChangeInfo>();
	qRegisterMetaTypeStreamOperators<QtDataSync::ObjectKey>();

	qRegisterRemoteObjectsServer<QtDataSync::ThreadedServer>(QtDataSync::ThreadedServer::UrlScheme());
	qRegisterRemoteObjectsClient<QtDataSync::ThreadedClientIoDevice>(QtDataSync::ThreadedServer::UrlScheme());

	QtDataSync::Message::registerTypes();
}

}
Q_COREAPP_STARTUP_FUNCTION(setupQtDataSync)
