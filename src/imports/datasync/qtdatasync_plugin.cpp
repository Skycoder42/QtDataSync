#include "qtdatasync_plugin.h"
#include <QtQml>

#include <QtDataSync/datasetid.h>
#include <QtDataSync/authenticator.h>
#include <QtDataSync/googleauthenticator.h>
#include <QtDataSync/mailauthenticator.h>
#include <QtDataSync/cloudtransformer.h>
#include <QtDataSync/engine.h>
#include <QtDataSync/tablesynccontroller.h>

QtDataSyncDeclarativeModule::QtDataSyncDeclarativeModule(QObject *parent) :
	QQmlExtensionPlugin{parent}
{}

void QtDataSyncDeclarativeModule::registerTypes(const char *uri)
{
	Q_ASSERT(qstrcmp(uri, "de.skycoder42.QtDataSync") == 0);

	// Version 5.0
	qmlRegisterUncreatableType<QtDataSync::DatasetId>(uri, 5, 0, "DatasetId", QStringLiteral("Q_GADGETS cannot be created"));

	qmlRegisterUncreatableType<QtDataSync::IAuthenticator>(uri, 5, 0, "IAuthenticator", QStringLiteral("Authenticators cannot be created"));
	qmlRegisterUncreatableType<QtDataSync::OAuthAuthenticator>(uri, 5, 0, "OAuthAuthenticator", QStringLiteral("Authenticators cannot be created"));
	qmlRegisterUncreatableType<QtDataSync::AuthenticationSelectorBase>(uri, 5, 0, "AuthenticationSelector", QStringLiteral("Authenticators cannot be created"));
	qmlRegisterUncreatableType<QtDataSync::GoogleAuthenticator>(uri, 5, 0, "GoogleAuthenticator", QStringLiteral("Authenticators cannot be created"));
	qmlRegisterUncreatableType<QtDataSync::MailAuthenticator>(uri, 5, 0, "MailAuthenticator", QStringLiteral("Authenticators cannot be created"));

	qmlRegisterUncreatableType<QtDataSync::ICloudTransformer>(uri, 5, 0, "ICloudTransformer", QStringLiteral("Cloud transformers cannot be created"));
	qmlRegisterUncreatableType<QtDataSync::ISynchronousCloudTransformer>(uri, 5, 0, "ISynchronousCloudTransformer", QStringLiteral("Cloud transformers cannot be created"));
	qmlRegisterUncreatableType<QtDataSync::PlainCloudTransformer>(uri, 5, 0, "PlainCloudTransformer", QStringLiteral("Cloud transformers cannot be created"));

	qmlRegisterUncreatableType<QtDataSync::Engine>(uri, 5, 0, "Engine", QStringLiteral("Datasync engines must be created in C++ via a setup"));
	qmlRegisterUncreatableType<QtDataSync::TableSyncController>(uri, 5, 0, "TableSyncController", QStringLiteral("TableSyncController must be created via Engine::createController"));

	// Check to make shure no module update is forgotten
	static_assert(VERSION_MAJOR == 5 && VERSION_MINOR == 0, "QML module version needs to be updated");
}
