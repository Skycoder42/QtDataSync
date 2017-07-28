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
	void testUpload_data();
	void testUpload();
	void testReloadAndResync();
	void testSyncDisable();
	void testRemove_data();
	void testRemove();

	void testSecondDevice();
	void testExportImport();

Q_SIGNALS:
	void unlockSignal();

private:
	AsyncDataStore *async;
	SyncController *controller;
	WsAuthenticator *auth;

	MockLocalStore *store;
	MockStateHolder *holder;
};

void WsRemoteConnectorTest::initTestCase()
{
#ifdef Q_OS_LINUX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	//create setup to "init" both of them, but datasync itself is not used here
	Setup setup;
	mockSetup(setup);
	store = static_cast<MockLocalStore*>(setup.localStore());
	holder = static_cast<MockStateHolder*>(setup.stateHolder());
	setup.setRemoteConnector(new WsRemoteConnector())
			.create();

	async = new AsyncDataStore(this);
	controller = new SyncController(this);
	auth = Setup::authenticatorForSetup<WsAuthenticator>(this);

	//now start the server!
	QProcess p;
	p.start(QStringLiteral("../../../../bin/qdatasyncserver start -c %1/../../../../tools/qdatasyncserver/docker_setup.conf")
			.arg(SRCDIR));
	p.waitForFinished();
	qDebug() << p.error() << p.exitStatus() << p.errorString();
	QVERIFY(p.waitForFinished());
	QCOMPARE(p.exitCode(), EXIT_SUCCESS);
}

void WsRemoteConnectorTest::cleanupTestCase()
{
	delete auth;
	delete controller;
	delete async;
	Setup::removeSetup(Setup::DefaultSetup);

	//and stop the server!
	QProcess p;
	p.start(QStringLiteral("../../../../bin/qdatasyncserver stop"));
	QVERIFY(p.waitForFinished());
	QCOMPARE(p.exitCode(), EXIT_SUCCESS);
}

void WsRemoteConnectorTest::testServerConnecting()
{
	QSignalSpy syncSpy(controller, &SyncController::syncStateChanged);
	QSignalSpy conSpy(auth, &WsAuthenticator::connectedChanged);

	QVERIFY(syncSpy.wait());
	QCOMPARE(controller->syncState(), SyncController::Disconnected);
	QVERIFY(!auth->isConnected());

	//empty store -> used INVALID id
	auth->setRemoteUrl(QStringLiteral("ws://localhost:4242"));
	auth->setServerSecret("baum42");
	auth->setUserIdentity("invalid");
	auth->reconnect();

	for(auto i = 0; i < 10 && syncSpy.count() < 3; i++)
		syncSpy.wait(500);
	QVERIFY(!auth->isConnected());
	QCOMPARE(syncSpy.count(), 3);
	QCOMPARE(syncSpy[0][0], QVariant::fromValue(SyncController::Disconnected));
	QCOMPARE(syncSpy[1][0], QVariant::fromValue(SyncController::Loading));
	QCOMPARE(syncSpy[2][0], QVariant::fromValue(SyncController::Disconnected));
	QCOMPARE(controller->syncState(), SyncController::Disconnected);

	//now connect again, but reset id first
	conSpy.clear();
	syncSpy.clear();
	auth->resetUserIdentity();
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

	//now reconnect, to test if "login" works as well
	conSpy.clear();
	syncSpy.clear();
	auth->reconnect();

	QVERIFY(conSpy.wait());//disconnect
	QVERIFY(conSpy.wait());//reconnect
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

void WsRemoteConnectorTest::testUpload_data()
{
	QTest::addColumn<TestData>("data");

	QTest::newRow("data0") << generateData(310);
	QTest::newRow("data1") << generateData(311);
	QTest::newRow("data2") << generateData(312);
	QTest::newRow("data3") << generateData(313);
	QTest::newRow("data4") << generateData(314);
	QTest::newRow("data5") << generateData(315);
	QTest::newRow("data6") << generateData(316);
	QTest::newRow("data7") << generateData(317);
}

void WsRemoteConnectorTest::testUpload()
{
	QFETCH(TestData, data);

	//IMPORTANT!!! clear pending notifies...
	QCoreApplication::processEvents();

	QSignalSpy syncSpy(controller, &SyncController::syncStateChanged);

	try {
		async->save<TestData>(data).waitForFinished();

		for(auto i = 0; i < 10 && syncSpy.count() < 2; i++)
			syncSpy.wait(500);
		QCOMPARE(syncSpy.count(), 2);
		QCOMPARE(syncSpy[0][0], QVariant::fromValue(SyncController::Syncing));
		QCOMPARE(syncSpy[1][0], QVariant::fromValue(SyncController::Synced));
		QCOMPARE(controller->syncState(), SyncController::Synced);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void WsRemoteConnectorTest::testReloadAndResync()
{
	//IMPORTANT!!! clear pending notifies...
	QCoreApplication::processEvents();

	QSignalSpy unlockSpy(this, &WsRemoteConnectorTest::unlockSignal);

	store->mutex.lock();
	store->enabled = true;
	store->mutex.unlock();

	controller->triggerSyncWithResult([&](SyncController::SyncState state){
		QCOMPARE(state, SyncController::Synced);
		emit unlockSignal();
	});

	QVERIFY(unlockSpy.wait());
	try {
		//sync wont help, no obvious changes
		QVERIFY(async->keys<TestData>().result().isEmpty());
	} catch(QException &e) {
		QFAIL(e.what());
	}

	controller->triggerResyncWithResult([&](SyncController::SyncState state){
		QCOMPARE(state, SyncController::Synced);
		emit unlockSignal();
	});

	QVERIFY(unlockSpy.wait());
	store->mutex.lock();
	[&](){
		QLISTCOMPARE(store->pseudoStore.keys(), generateDataJson(310, 318).keys());
	}();
	store->mutex.unlock();
}

void WsRemoteConnectorTest::testSyncDisable()
{
	//IMPORTANT!!! clear pending notifies...
	QCoreApplication::processEvents();

	QSignalSpy unlockSpy(this, &WsRemoteConnectorTest::unlockSignal);
	QSignalSpy syncChangedSpy(controller, &SyncController::syncEnabledChanged);

	store->mutex.lock();
	store->enabled = true;
	store->mutex.unlock();

	controller->setSyncEnabled(false);
	QVERIFY(syncChangedSpy.wait());

	controller->triggerSyncWithResult([&](SyncController::SyncState state){
		QCOMPARE(state, SyncController::Disconnected);
		emit unlockSignal();
	});
	QCOMPARE(unlockSpy.count(), 1);

	controller->setSyncEnabled(true);
	QVERIFY(syncChangedSpy.wait());

	controller->triggerSyncWithResult([&](SyncController::SyncState state){
		QCOMPARE(state, SyncController::Synced);
		emit unlockSignal();
	});
	QVERIFY(unlockSpy.wait());
}

void WsRemoteConnectorTest::testRemove_data()
{
	QTest::addColumn<int>("index");

	QTest::newRow("data0") << 310;
	QTest::newRow("data1") << 311;
	QTest::newRow("data2") << 312;
}

void WsRemoteConnectorTest::testRemove()
{
	QFETCH(int, index);

	//IMPORTANT!!! clear pending notifies...
	QCoreApplication::processEvents();

	QSignalSpy syncSpy(controller, &SyncController::syncStateChanged);

	store->mutex.lock();
	[&](){
		QVERIFY(store->pseudoStore.keys().contains(generateKey(index)));
	}();
	store->mutex.unlock();

	try {
		async->remove<TestData>(index).waitForFinished();

		for(auto i = 0; i < 10 && syncSpy.count() < 2; i++)
			syncSpy.wait(500);
		QCOMPARE(syncSpy.count(), 2);
		QCOMPARE(syncSpy[0][0], QVariant::fromValue(SyncController::Syncing));
		QCOMPARE(syncSpy[1][0], QVariant::fromValue(SyncController::Synced));
		QCOMPARE(controller->syncState(), SyncController::Synced);

	} catch(QException &e) {
		QFAIL(e.what());
	}

	store->mutex.lock();
	[&](){
		QVERIFY(!store->pseudoStore.keys().contains(generateKey(index)));
	}();
	store->mutex.unlock();
}

void WsRemoteConnectorTest::testSecondDevice()
{
	//IMPORTANT!!! clear pending notifies...
	QCoreApplication::processEvents();

	QSignalSpy syncSpy(controller, &SyncController::syncStateChanged);
	QSignalSpy conSpy(auth, &WsAuthenticator::connectedChanged);

	auth->setUserIdentity(auth->userIdentity());//reset device trick...
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

	store->mutex.lock();
	[&](){
		QLISTCOMPARE(store->pseudoStore.keys(), generateDataJson(313, 318).keys());
	}();
	store->mutex.unlock();
}

void WsRemoteConnectorTest::testExportImport()
{
	//IMPORTANT!!! clear pending notifies...
	QCoreApplication::processEvents();

	QSignalSpy syncSpy(controller, &SyncController::syncStateChanged);
	QSignalSpy conSpy(auth, &WsAuthenticator::connectedChanged);

	auto data = auth->exportUserData();

	//after export: test with different user to reset
	auth->resetUserIdentity();//reset device trick...
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

	store->mutex.lock();
	[&](){
		QVERIFY(store->pseudoStore.keys().isEmpty());
	}();
	store->mutex.unlock();

	//now reimport
	syncSpy.clear();
	conSpy.clear();

	auth->importUserData(data);

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

	store->mutex.lock();
	[&](){
		QLISTCOMPARE(store->pseudoStore.keys(), generateDataJson(313, 318).keys());
	}();
	store->mutex.unlock();
}

QTEST_MAIN(WsRemoteConnectorTest)

#include "tst_wsremoteconnector.moc"
