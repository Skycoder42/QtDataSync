#include "qtdatasync_global.h"
#include "objectkey.h"
#include "changecontroller_p.h"

#include "exchangerotransport_p.h"

#include <QtCore/QCoreApplication>
#include "message_p.h"

const QString QtDataSync::DefaultSetup(QStringLiteral("default"));

namespace {

void setupQtDataSync()
{
	qRegisterMetaType<QtDataSync::ObjectKey>();
	qRegisterMetaType<QtDataSync::ChangeController::ChangeInfo>();
	qRegisterMetaTypeStreamOperators<QtDataSync::ObjectKey>();

	QtDataSync::QtRoTransportRegistry::registerTransport(QtDataSync::ExchangeBufferServer::UrlScheme(),
														 new QtDataSync::ThreadedQtRoServer{},
														 new QtDataSync::ThreadedQtRoClient{});

	QtDataSync::Message::registerTypes();
}

}
Q_COREAPP_STARTUP_FUNCTION(setupQtDataSync)
