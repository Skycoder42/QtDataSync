#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "tst.h"
#include "mockremoteserver.h"
#include "QtDataSync/private/wsremoteconnector_p.h"

using namespace QtDataSync;

class WsRemoteConnectorTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testServerConnecting();

private:
	MockRemoteServer *server;
	WsRemoteConnector *remote;
	SyncController *controller;
	WsAuthenticator *auth;
};

void WsRemoteConnectorTest::initTestCase()
{
#ifdef Q_OS_UNIX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	server = new MockRemoteServer(this);
	remote = new WsRemoteConnector();

	//create setup to "init" both of them, but datasync itself is not used here
	Setup setup;
	mockSetup(setup);
	setup.setRemoteConnector(remote)
			.create();

	controller = new SyncController(this);
	auth = Setup::authenticatorForSetup<WsAuthenticator>(this);
}

void WsRemoteConnectorTest::cleanupTestCase()
{
	Setup::removeSetup(Setup::DefaultSetup);
	server->close();
}

void WsRemoteConnectorTest::testServerConnecting()
{
	QSignalSpy syncSpy(controller, &SyncController::syncStateChanged);
	QSignalSpy conSpy(auth, &WsAuthenticator::connectedChanged);


	QVERIFY(syncSpy.wait());
	QCOMPARE(controller->syncState(), SyncController::Disconnected);
	QVERIFY(!auth->isConnected());

	auto port = server->setup();
	QVERIFY(port != 0);

	//prepare server
	auto userId = QUuid::createUuid();
	server->expected.append({
								{"command", "createIdentity"}
							});
	server->reply.enqueue({
							 {"command" , "identified"},
							 {"data" , QJsonObject({
								  {"userId", userId.toString()},
								  {"resync", true}
							  })}
						 });

	QUrl url(QStringLiteral("ws://localhost"));
	url.setPort(port);
	auth->setRemoteUrl(url);
	auth->setServerSecret(server->secret);
	auth->reconnect();

	QVERIFY(conSpy.wait());
	QVERIFY(auth->isConnected());
	for(auto i = 0; i < 10 && syncSpy.count() < 3; i++)
		syncSpy.wait(500);
	QCOMPARE(syncSpy.count(), 3);
	QCOMPARE(syncSpy[0][0], QVariant::fromValue(SyncController::Loading));
	QCOMPARE(syncSpy[1][0], QVariant::fromValue(SyncController::Syncing));
	QCOMPARE(syncSpy[2][0], QVariant::fromValue(SyncController::Synced));
	QCOMPARE(controller->syncState(), SyncController::Synced);
}

QTEST_MAIN(WsRemoteConnectorTest)

#include "tst_wsremoteconnector.moc"
