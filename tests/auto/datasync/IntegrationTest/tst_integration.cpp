#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <testdata.h>
using namespace QtDataSync;

class IntegrationTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testPrepareData();
	void testAddAccount();

Q_SIGNALS:
	void unlock();

private:
	QProcess *server;

	AccountManager *acc1;
	SyncManager *sync1;
	DataTypeStore<TestData> *store1;

	AccountManager *acc2;
	SyncManager *sync2;
	DataTypeStore<TestData> *store2;
};

void IntegrationTest::initTestCase()
{
	QByteArray confPath { SETUP_FILE };
	QVERIFY(QFile::exists(QString::fromUtf8(confPath)));
	qputenv("QDSAPP_CONFIG_FILE", confPath);

#ifdef Q_OS_UNIX
	QString binPath { QStringLiteral(BUILD_BIN_DIR "qdsappd") };
#elif Q_OS_WIN
	QString binPath { QStringLiteral(BUILD_BIN_DIR "qdsappsvc") };
#else
	QString binPath { QStringLiteral(BUILD_BIN_DIR "qdsapp") };
#endif
	QVERIFY(QFile::exists(binPath));

	server = new QProcess(this);
	server->setProgram(binPath);
	server->setProcessChannelMode(QProcess::ForwardedErrorChannel);
	server->start();
	QVERIFY(server->waitForStarted(5000));
	QVERIFY(!server->waitForFinished(5000));

	try {
		TestLib::init();
		qRegisterMetaType<LoginRequest>();

		{
			QString setupName1 = QStringLiteral("setup1");
			Setup setup1;
			TestLib::setup(setup1);
			setup1.setLocalDir(setup1.localDir() + QLatin1Char('/') + setupName1)
					.setRemoteConfiguration(QUrl(QStringLiteral("ws://localhost:14242")));
			setup1.create(setupName1);

			acc1 = new AccountManager(setupName1, this);
			QVERIFY(acc1->replica()->waitForSource(5000));
			sync1 = new SyncManager(setupName1, this);
			QVERIFY(acc1->replica()->waitForSource(5000));
			store1 = new DataTypeStore<TestData>(setupName1, this);
		}

		{
			QString setupName2 = QStringLiteral("setup2");
			Setup setup2;
			TestLib::setup(setup2);
			setup2.setLocalDir(setup2.localDir() + QLatin1Char('/') + setupName2)
					.setRemoteConfiguration(QUrl(QStringLiteral("ws://localhost:14242")));
			setup2.create(setupName2);

			acc2 = new AccountManager(setupName2, this);
			QVERIFY(acc2->replica()->waitForSource(5000));
			sync2 = new SyncManager(setupName2, this);
			QVERIFY(sync2->replica()->waitForSource(5000));
			store2 = new DataTypeStore<TestData>(setupName2, this);
		}
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void IntegrationTest::cleanupTestCase()
{
	delete store1;
	store1 = nullptr;
	delete sync1;
	sync1 = nullptr;
	delete acc1;
	acc1 = nullptr;
	delete store2;
	store2 = nullptr;
	delete sync2;
	sync2 = nullptr;
	delete acc2;
	acc2 = nullptr;
	Setup::removeSetup(DefaultSetup, true);

	//send a signal to stop
#ifdef Q_OS_UNIX
	server->terminate(); //same as kill(SIGTERM)
#elif Q_OS_WIN
	GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, server->processId());
#endif
	QVERIFY(server->waitForFinished(5000));
	QCOMPARE(server->exitStatus(), QProcess::NormalExit);
	QCOMPARE(server->exitCode(), 0);
	server->close();
}

void IntegrationTest::testPrepareData()
{
	try {
		QSignalSpy errorSpy(acc1, &AccountManager::lastErrorChanged);
		QSignalSpy devSpy(acc1, &AccountManager::deviceNameChanged);
		QSignalSpy unlockSpy(this, &IntegrationTest::unlock);

		acc1->setDeviceName(QStringLiteral("master"));
		QVERIFY(devSpy.wait());
		QCOMPARE(devSpy.size(), 1);
		QCOMPARE(devSpy.takeFirst()[0].toString(), QStringLiteral("master"));

		for(auto i = 0; i < 10; i++)
			store1->save(TestLib::generateData(i));
		QCOMPARE(store1->count(), 10);

		sync1->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Synchronized);
		}, false);
		QVERIFY(unlockSpy.wait());

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
	try {
		QSignalSpy errorSpy(acc2, &AccountManager::lastErrorChanged);
		QSignalSpy devSpy(acc2, &AccountManager::deviceNameChanged);
		QSignalSpy unlockSpy(this, &IntegrationTest::unlock);

		acc2->setDeviceName(QStringLiteral("partner"));
		QVERIFY(devSpy.wait());
		QCOMPARE(devSpy.size(), 1);
		QCOMPARE(devSpy.takeFirst()[0].toString(), QStringLiteral("partner"));

		for(auto i = 10; i < 20; i++)
			store2->save(TestLib::generateData(i));
		QCOMPARE(store2->count(), 10);

		sync2->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Synchronized);
		}, false);
		QVERIFY(unlockSpy.wait());

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void IntegrationTest::testAddAccount()
{
	try {
		QSignalSpy error1Spy(acc1, &AccountManager::lastErrorChanged);
		QSignalSpy error2Spy(acc2, &AccountManager::lastErrorChanged);
		QSignalSpy fprintSpy(acc2, &AccountManager::deviceFingerprintChanged);
		QSignalSpy requestSpy(acc1, &AccountManager::loginRequested);
		QSignalSpy acceptSpy(acc2, &AccountManager::importAccepted);
		QSignalSpy grantSpy(acc1, &AccountManager::accountAccessGranted);
		QSignalSpy devices1Spy(acc1, &AccountManager::accountDevices);
		QSignalSpy devices2Spy(acc2, &AccountManager::accountDevices);
		QSignalSpy unlockSpy(this, &IntegrationTest::unlock);

		//export from acc1
		acc1->exportAccount(false, [this](QJsonObject exp) {
			acc2->importAccount(exp, [](bool ok, QString e) {
				QVERIFY2(ok, qUtf8Printable(e));
			}, true);
		}, [](QString e) {
			QFAIL(qUtf8Printable(e));
		});

		//wait for acc2 fingerprint update
		QVERIFY(fprintSpy.wait());
		QCOMPARE(fprintSpy.size(), 1);

		//wait and accept acc1 login request
		QVERIFY(requestSpy.wait());
		QCOMPARE(requestSpy.size(), 1);
		auto request = requestSpy.takeFirst()[0].value<LoginRequest>();
		QVERIFY(!request.handled());
		QCOMPARE(request.device().name(), acc2->deviceName());
		QCOMPARE(request.device().fingerprint(), acc2->deviceFingerprint());
		auto pId = request.device().deviceId();
		QVERIFY(!pId.isNull());
		request.accept();
		QVERIFY(request.handled());

		//wait for accept
		if(acceptSpy.isEmpty())
			QVERIFY(acceptSpy.wait());
		QCOMPARE(acceptSpy.size(), 1);

		//wait for grant
		if(grantSpy.isEmpty())
			QVERIFY(grantSpy.wait());
		QCOMPARE(grantSpy.size(), 1);
		QCOMPARE(grantSpy.takeFirst()[0].toUuid(), pId);

		//wait for sync
		sync1->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Synchronized);
		}, true);
		unlockSpy.wait();
		sync2->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Synchronized);
		});
		unlockSpy.wait();

		QCOMPARE(store1->count(), 20);
		QCOMPAREUNORDERED(store1->keys(), TestLib::generateDataKeys(0, 19));
		QCOMPARE(store2->count(), 20);
		QCOMPAREUNORDERED(store2->keys(), TestLib::generateDataKeys(0, 19));

		//check devices status
		acc1->listDevices();
		QVERIFY(devices1Spy.wait());
		QCOMPARE(devices1Spy.size(), 1);
		auto devices = devices1Spy.takeFirst()[0].value<QList<DeviceInfo>>();
		QCOMPARE(devices.size(), 1);
		QCOMPARE(devices[0].deviceId(), pId);
		QCOMPARE(devices[0].name(), acc2->deviceName());
		QCOMPARE(devices[0].fingerprint(), acc2->deviceFingerprint());

		acc2->listDevices();
		QVERIFY(devices2Spy.wait());
		QCOMPARE(devices2Spy.size(), 1);
		devices = devices2Spy.takeFirst()[0].value<QList<DeviceInfo>>();
		QCOMPARE(devices.size(), 1);
		QCOMPARE(devices[0].name(), acc1->deviceName());
		QCOMPARE(devices[0].fingerprint(), acc1->deviceFingerprint());

		QVERIFY(error1Spy.isEmpty());
		QVERIFY(error2Spy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(IntegrationTest)

#include "tst_integration.moc"
