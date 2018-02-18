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
	void testLiveSync();
	void testPassiveSync();
	void testRemoveAndResetAccount();
	void testAddAccountTrusted();

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

	QUuid dev2Id;
};

void IntegrationTest::initTestCase()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
	//QLoggingCategory::setFilterRules(QStringLiteral("qtdatasync.*.debug=true"));

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
	Setup::removeSetup(QStringLiteral("setup1"), true);

	delete store2;
	store2 = nullptr;
	delete sync2;
	sync2 = nullptr;
	delete acc2;
	acc2 = nullptr;
	Setup::removeSetup(QStringLiteral("setup2"), true);

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
			QVERIFY(!AccountManager::isTrustedImport(exp));
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
		dev2Id = request.device().deviceId();
		QVERIFY(!dev2Id.isNull());
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
		QCOMPARE(grantSpy.takeFirst()[0].toUuid(), dev2Id);

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
		QCOMPARE(devices[0].deviceId(), dev2Id);
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

void IntegrationTest::testLiveSync()
{
	try {
		QSignalSpy error1Spy(sync1, &SyncManager::lastErrorChanged);
		QSignalSpy error2Spy(sync2, &SyncManager::lastErrorChanged);
		QSignalSpy dataSpy(store2, &DataTypeStoreBase::dataChanged);
		QSignalSpy sync1Spy(sync1, &SyncManager::syncStateChanged);
		QSignalSpy sync2Spy(sync2, &SyncManager::syncStateChanged);
		QSignalSpy unlockSpy(this, &IntegrationTest::unlock);

		//sync data from 1
		for(auto i = 20; i < 40; i++)
			store1->save(TestLib::generateData(i));
		sync1->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Synchronized);
		}, false);
		QVERIFY(sync1Spy.wait());
		QVERIFY(sync1Spy.size() > 0);
		QCOMPARE(sync1Spy.takeFirst()[0].toInt(), SyncManager::Uploading);
		QVERIFY(unlockSpy.wait());
		QVERIFY(sync1Spy.size() > 0);
		QCOMPARE(sync1Spy.last()[0].toInt(), SyncManager::Synchronized);
		sync1Spy.clear();

		//sync data to 2
		sync2->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QVERIFY(s == SyncManager::Synchronized);
		}, false);
		if(sync2Spy.isEmpty())
			QVERIFY(sync2Spy.wait());
		QVERIFY(sync2Spy.size() > 0);
		QCOMPARE(sync2Spy.takeFirst()[0].toInt(), SyncManager::Downloading);
		QVERIFY(unlockSpy.wait());
		QVERIFY(sync2Spy.size() > 0);
		QCOMPARE(sync2Spy.last()[0].toInt(), SyncManager::Synchronized);
		sync2Spy.clear();

		//verify data changes
		QCOMPARE(dataSpy.size(), 20);
		QStringList keys;
		for(auto sig : QList<QList<QVariant>>(dataSpy))
			keys.append(sig[0].toString());
		QCOMPAREUNORDERED(keys, TestLib::generateDataKeys(20, 39));

		QVERIFY(!unlockSpy.wait());
		QVERIFY(error1Spy.isEmpty());
		QVERIFY(error2Spy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void IntegrationTest::testPassiveSync()
{
	try {
		QSignalSpy error1Spy(sync1, &SyncManager::lastErrorChanged);
		QSignalSpy error2Spy(sync2, &SyncManager::lastErrorChanged);
		QSignalSpy data1Spy(store1, &DataTypeStoreBase::dataChanged);
		QSignalSpy data2Spy(store2, &DataTypeStoreBase::dataChanged);
		QSignalSpy enabled1Spy(sync1, &SyncManager::syncEnabledChanged);
		QSignalSpy enabled2Spy(sync2, &SyncManager::syncEnabledChanged);
		QSignalSpy sync1Spy(sync1, &SyncManager::syncStateChanged);
		QSignalSpy sync2Spy(sync2, &SyncManager::syncStateChanged);
		QSignalSpy unlockSpy(this, &IntegrationTest::unlock);

		//disable 1
		sync1->setSyncEnabled(false);
		QVERIFY(enabled1Spy.wait());
		QCOMPARE(enabled1Spy.size(), 1);
		QCOMPARE(enabled1Spy.takeFirst()[0].toBool(), false);
		//wait for disconnected
		if(sync1Spy.isEmpty())
			QVERIFY(sync1Spy.wait());
		QCOMPARE(sync1Spy.size(), 1);
		QCOMPARE(sync1Spy.takeFirst()[0].toInt(), SyncManager::Disconnected);
		//disable 2
		sync2->setSyncEnabled(false);
		QVERIFY(enabled2Spy.wait());
		QCOMPARE(enabled2Spy.size(), 1);
		QCOMPARE(enabled2Spy.takeFirst()[0].toBool(), false);
		//wait for disconnected
		if(sync2Spy.isEmpty())
			QVERIFY(sync2Spy.wait());
		QCOMPARE(sync2Spy.size(), 1);
		QCOMPARE(sync2Spy.takeFirst()[0].toInt(), SyncManager::Disconnected);

		//sync data from 2
		for(auto i = 20; i < 30; i++)
			store2->remove(TestLib::generateDataKey(i));
		sync2->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Disconnected);
		}, false);
		QVERIFY(unlockSpy.wait());

		//enable sync
		sync2->setSyncEnabled(true);
		QVERIFY(enabled2Spy.wait());
		QCOMPARE(enabled2Spy.size(), 1);
		QCOMPARE(enabled2Spy.takeFirst()[0].toBool(), true);
		//wait for init
		QVERIFY(sync2Spy.wait());
		QVERIFY(sync2Spy.size() > 0);
		QCOMPARE(sync2Spy.takeFirst()[0].toInt(), SyncManager::Initializing);
		//wait for uploading
		if(sync2Spy.isEmpty())
			QVERIFY(sync2Spy.wait());
		QVERIFY(sync2Spy.size() > 0);
		QCOMPARE(sync2Spy.takeFirst()[0].toInt(), SyncManager::Uploading);
		//wait for synced
		sync2->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Synchronized);
		}, false);
		QVERIFY(unlockSpy.wait());
		QCOMPARE(sync2Spy.size(), 1);
		QCOMPARE(sync2Spy.takeFirst()[0].toInt(), SyncManager::Synchronized);

		//sync data from and to 1
		for(auto i = 30; i < 40; i++)
			store1->remove(TestLib::generateDataKey(i));
		sync1->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Disconnected);
		}, false);
		QVERIFY(unlockSpy.wait());

		//enable sync
		sync1->setSyncEnabled(true);
		QVERIFY(enabled1Spy.wait());
		QCOMPARE(enabled1Spy.size(), 1);
		QCOMPARE(enabled1Spy.takeFirst()[0].toBool(), true);
		//wait for init
		QVERIFY(sync1Spy.wait());
		QVERIFY(sync1Spy.size() > 0);
		QCOMPARE(sync1Spy.takeFirst()[0].toInt(), SyncManager::Initializing);
		//wait for downloading
		if(sync1Spy.isEmpty())
			QVERIFY(sync1Spy.wait());
		QVERIFY(sync1Spy.size() > 0);
		QCOMPARE(sync1Spy.takeFirst()[0].toInt(), SyncManager::Downloading);
		unlockSpy.clear();
		sync1->runOnDownloaded([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Uploading);
		}, false);
		sync1->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Synchronized);
		}, false);
		//wait for uploading
		if(sync1Spy.isEmpty())
			QVERIFY(sync1Spy.wait());
		QVERIFY(sync1Spy.size() > 0);
		QCOMPARE(sync1Spy.takeFirst()[0].toInt(), SyncManager::Uploading);
		//wait for synced
		QVERIFY(unlockSpy.wait());
		if(unlockSpy.size() != 2)
			QVERIFY(unlockSpy.wait());
		QVERIFY(sync1Spy.size() > 0);
		QCOMPARE(sync1Spy.last()[0].toInt(), SyncManager::Synchronized);

		//sync data to 2 again
		sync2->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Synchronized);
		}, false);
		if(sync2Spy.isEmpty())
			QVERIFY(sync2Spy.wait());
		QVERIFY(sync2Spy.size() > 0);
		QCOMPARE(sync2Spy.takeFirst()[0].toInt(), SyncManager::Downloading);
		QVERIFY(unlockSpy.wait());
		QVERIFY(sync2Spy.size() > 0);
		QCOMPARE(sync2Spy.last()[0].toInt(), SyncManager::Synchronized);
		sync2Spy.clear();

		//verify data changes on 1
		QCOMPARE(data1Spy.size(), 20);
		QStringList keys;
		for(auto sig : QList<QList<QVariant>>(data1Spy))
			keys.append(sig[0].toString());
		QCOMPAREUNORDERED(keys, TestLib::generateDataKeys(20, 39));
		//and on 2
		QCOMPARE(data2Spy.size(), 20);
		keys.clear();
		for(auto sig : QList<QList<QVariant>>(data2Spy))
			keys.append(sig[0].toString());
		QCOMPAREUNORDERED(keys, TestLib::generateDataKeys(20, 39));

		QVERIFY(!unlockSpy.wait());
		QVERIFY(error1Spy.isEmpty());
		QVERIFY(error2Spy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void IntegrationTest::testRemoveAndResetAccount()
{
	try {
		QSignalSpy error1Spy(acc1, &AccountManager::lastErrorChanged);
		QSignalSpy error2Spy(acc2, &AccountManager::lastErrorChanged);
		QSignalSpy sync1Spy(sync1, &SyncManager::syncStateChanged);
		QSignalSpy sync2Spy(sync2, &SyncManager::syncStateChanged);
		QSignalSpy devices1Spy(acc1, &AccountManager::accountDevices);
		QSignalSpy data1Spy(store1, &DataTypeStoreBase::dataResetted);

		//remove 2
		acc1->removeDevice(dev2Id);
		QVERIFY(sync2Spy.wait());
		QCOMPARE(sync2Spy.size(), 1);
		QCOMPARE(sync2Spy.takeFirst()[0].toInt(), SyncManager::Disconnected);

		//verify devices not there
		acc1->listDevices();
		QVERIFY(devices1Spy.wait());
		QCOMPARE(devices1Spy.size(), 1);
		auto devices = devices1Spy.takeFirst()[0].value<QList<DeviceInfo>>();
		QVERIFY(devices.isEmpty());

		//verify connect fails
		sync2->reconnect();
		//wait for init
		if(sync2Spy.isEmpty())
			QVERIFY(sync2Spy.wait());
		QVERIFY(sync2Spy.size() > 0);
		QCOMPARE(sync2Spy.takeFirst()[0].toInt(), SyncManager::Initializing);
		//wait for error
		if(sync2Spy.isEmpty())
			QVERIFY(sync2Spy.wait());
		QVERIFY(sync2Spy.size() > 0);
		QCOMPARE(sync2Spy.takeFirst()[0].toInt(), SyncManager::Error);
		//on error
		sync2->runOnSynchronized([this](SyncManager::SyncState s) {
			emit unlock();
			QCOMPARE(s, SyncManager::Error);
		}, false);
		if(error2Spy.isEmpty())
			QVERIFY(error2Spy.wait());
		QCOMPARE(error2Spy.size(), 1);
		error2Spy.clear();

		//reset 1 to clear it
		acc1->resetAccount(false);
		//disconnect
		if(sync1Spy.isEmpty())
			QVERIFY(sync1Spy.wait());
		QVERIFY(sync1Spy.size() > 0);
		QCOMPARE(sync1Spy.takeFirst()[0].toInt(), SyncManager::Disconnected);
		//reconnect
		if(sync1Spy.isEmpty())
			QVERIFY(sync1Spy.wait());
		QVERIFY(sync1Spy.size() > 0);
		QCOMPARE(sync1Spy.takeFirst()[0].toInt(), SyncManager::Initializing);
		//uploading
		if(sync1Spy.isEmpty())
			QVERIFY(sync1Spy.wait());
		QVERIFY(sync1Spy.size() > 0);
		QCOMPARE(sync1Spy.takeFirst()[0].toInt(), SyncManager::Uploading);
		//synced
		if(sync1Spy.isEmpty())
			QVERIFY(sync1Spy.wait());
		QCOMPARE(sync1Spy.size(), 1);
		QCOMPARE(sync1Spy.takeFirst()[0].toInt(), SyncManager::Synchronized);

		if(data1Spy.isEmpty())
			QVERIFY(data1Spy.wait());

		QVERIFY(error1Spy.isEmpty());
		QVERIFY(error2Spy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void IntegrationTest::testAddAccountTrusted()
{
	try {
		QSignalSpy error1Spy(acc1, &AccountManager::lastErrorChanged);
		QSignalSpy error2Spy(acc2, &AccountManager::lastErrorChanged);
		QSignalSpy fprintSpy(acc2, &AccountManager::deviceFingerprintChanged);
		QSignalSpy requestSpy(acc1, &AccountManager::loginRequested);
		QSignalSpy acceptSpy(acc2, &AccountManager::importAccepted);
		QSignalSpy grantSpy(acc1, &AccountManager::accountAccessGranted);
		QSignalSpy unlockSpy(this, &IntegrationTest::unlock);

		//export from acc1
		auto password = QStringLiteral("password");
		acc1->exportAccountTrusted(false, password, [&](QJsonObject exp) {
			QVERIFY(AccountManager::isTrustedImport(exp));
			acc2->importAccountTrusted(exp, password, [&](bool ok, QString e) {
				QVERIFY2(ok, qUtf8Printable(e));
				if(error2Spy.wait())
					error2Spy.clear();
			}, false);
		}, [](QString e) {
			QFAIL(qUtf8Printable(e));
		});

		//wait for acc2 fingerprint update
		QVERIFY(fprintSpy.wait());
		QVERIFY(fprintSpy.size() > 0);

		//wait for login request to not come...
		QVERIFY(!requestSpy.wait());

		//wait for accept
		if(acceptSpy.isEmpty())
			QVERIFY(acceptSpy.wait());
		QCOMPARE(acceptSpy.size(), 1);

		//wait for grant
		if(grantSpy.isEmpty())
			QVERIFY(grantSpy.wait());
		QCOMPARE(grantSpy.size(), 1);

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

		QCOMPARE(store1->count(), 0);
		QCOMPARE(store2->count(), 0);

		QVERIFY(error1Spy.isEmpty());
		QVERIFY(error2Spy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(IntegrationTest)

#include "tst_integration.moc"
