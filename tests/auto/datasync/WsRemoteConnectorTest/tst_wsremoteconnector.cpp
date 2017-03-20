#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "tst.h"
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
}

void WsRemoteConnectorTest::testServerConnecting()
{
	QSignalSpy syncSpy(controller, &SyncController::syncStateChanged);
	QSignalSpy conSpy(auth, &WsAuthenticator::connectedChanged);

	QVERIFY(syncSpy.wait());
	QCOMPARE(controller->syncState(), SyncController::Disconnected);
	QVERIFY(!auth->isConnected());

	//empty store -> will create a new id
	auth->setRemoteUrl(QStringLiteral("ws://localhost:4242"));
	auth->setServerSecret("baum42");
	auth->reconnect();

	QVERIFY(conSpy.wait());
	QVERIFY(auth->isConnected());
	for(auto i = 0; i < 10 && syncSpy.count() < 4; i++)
		syncSpy.wait(500);
	QCOMPARE(syncSpy.count(), 4);
	QCOMPARE(syncSpy[0][0], QVariant::fromValue(SyncController::Disconnected));
	QCOMPARE(syncSpy[1][0], QVariant::fromValue(SyncController::Loading));
	QCOMPARE(syncSpy[2][0], QVariant::fromValue(SyncController::Syncing));
	QCOMPARE(syncSpy[3][0], QVariant::fromValue(SyncController::Synced));
	QCOMPARE(controller->syncState(), SyncController::Synced);

	//now reconnect, to test if "login" works as well
	conSpy.clear();
	syncSpy.clear();
	auth->reconnect();

	QVERIFY(conSpy.wait());
	QVERIFY(!auth->isConnected());
	QVERIFY(conSpy.wait());
	QVERIFY(auth->isConnected());
	for(auto i = 0; i < 10 && syncSpy.count() < 4; i++)
		syncSpy.wait(500);
	QCOMPARE(syncSpy.count(), 4);
	QCOMPARE(syncSpy[0][0], QVariant::fromValue(SyncController::Disconnected));
	QCOMPARE(syncSpy[1][0], QVariant::fromValue(SyncController::Loading));
	QCOMPARE(syncSpy[2][0], QVariant::fromValue(SyncController::Syncing));
	QCOMPARE(syncSpy[3][0], QVariant::fromValue(SyncController::Synced));
	QCOMPARE(controller->syncState(), SyncController::Synced);
}

QTEST_MAIN(WsRemoteConnectorTest)

#include "tst_wsremoteconnector.moc"
