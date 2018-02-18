#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <QtDataSync/private/changecontroller_p.h>
#include <QtDataSync/private/synchelper_p.h>
#include <QtDataSync/private/defaults_p.h>

//DIRTY HACK: allow access for test
#define private public
#include <QtDataSync/private/exchangeengine_p.h>
#undef private

#include <QtDataSync/private/setup_p.h>
using namespace QtDataSync;

class TestChangeController : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testChanges_data();
	void testChanges();

	void testDeviceChanges();

	//last test, to avoid problems
	void testChangeTriggers();
	void testPassiveTriggers();

private:
	LocalStore *store;
	ChangeController *controller;
};

void TestChangeController::initTestCase()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
	try {
		TestLib::init();
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		auto engine = SetupPrivate::engine(DefaultSetup);

		store = new LocalStore(DefaultsPrivate::obtainDefaults(DefaultSetup), this);
		controller = new ChangeController(DefaultsPrivate::obtainDefaults(DefaultSetup), this);
		controller->initialize({
								   {QStringLiteral("store"), QVariant::fromValue(store)},
								   {QStringLiteral("emitter"), QVariant::fromValue<QObject*>(reinterpret_cast<QObject*>(engine->emitter()))}, //trick to pass the unexported type to qvariant
							   });
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestChangeController::cleanupTestCase()
{
	controller->finalize();
	delete controller;
	controller = nullptr;
	delete store;
	store = nullptr;
	Setup::removeSetup(DefaultSetup, true);
}

void TestChangeController::testChanges_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<QJsonObject>("data");
	QTest::addColumn<bool>("isDelete");
	QTest::addColumn<quint64>("version");
	QTest::addColumn<bool>("complete");

	QTest::newRow("change1") << TestLib::generateKey(42)
							 << TestLib::generateDataJson(42)
							 << false
							 << 1ull
							 << false;
	QTest::newRow("change2") << TestLib::generateKey(42)
							 << TestLib::generateDataJson(42, QStringLiteral("hello world"))
							 << false
							 << 2ull
							 << false;
	QTest::newRow("delete3") << TestLib::generateKey(42)
							 << QJsonObject()
							 << true
							 << 3ull
							 << false;
	QTest::newRow("change4") << TestLib::generateKey(42)
							 << TestLib::generateDataJson(42)
							 << false
							 << 4ull
							 << true;
	QTest::newRow("delete5") << TestLib::generateKey(42)
							 << QJsonObject()
							 << true
							 << 5ull
							 << true;
	QTest::newRow("delete0") << TestLib::generateKey(42)
							 << QJsonObject()
							 << true
							 << 0ull
							 << true;
	QTest::newRow("change1") << TestLib::generateKey(42)
							 << TestLib::generateDataJson(42)
							 << false
							 << 1ull
							 << true;
}

void TestChangeController::testChanges()
{
	QFETCH(ObjectKey, key);
	QFETCH(QJsonObject, data);
	QFETCH(bool, isDelete);
	QFETCH(quint64, version);
	QFETCH(bool, complete);

	controller->setUploadingEnabled(false);
	QCoreApplication::processEvents();
	QSignalSpy activeSpy(controller, &ChangeController::uploadingChanged);
	QSignalSpy changeSpy(controller, &ChangeController::uploadChange);
	QSignalSpy addedSpy(controller, &ChangeController::progressAdded);
	QSignalSpy incrementSpy(controller, &ChangeController::progressIncrement);
	QSignalSpy errorSpy(controller, &ChangeController::controllerError);

	try {
		//trigger the change
		if(isDelete)
			store->remove(key);
		else
			store->save(key, data);

		//enable changes
		controller->setUploadingEnabled(true);
		if(!errorSpy.isEmpty())
			QFAIL(errorSpy.takeFirst()[0].toString().toUtf8().constData());

		if(version == 0) {
			QVERIFY(!activeSpy.last()[0].toBool());
			QVERIFY(changeSpy.isEmpty());
			QVERIFY(errorSpy.isEmpty());
			return;
		}

		QCOMPARE(changeSpy.size(), 1);
		QVERIFY(activeSpy.last()[0].toBool());
		QCOMPARE(addedSpy.size(), 1);
		QCOMPARE(addedSpy.takeFirst()[0].toUInt(), 1u);
		auto change = changeSpy.takeFirst();
		auto keyHash = change[0].toByteArray();
		auto syncData = SyncHelper::extract(change[1].toByteArray());
		QCOMPARE(std::get<0>(syncData), isDelete);
		QCOMPARE(std::get<1>(syncData), key);
		QCOMPARE(std::get<2>(syncData), version);
		QCOMPARE(std::get<3>(syncData), data);

		if(complete) {
			controller->uploadDone(keyHash);
			QCOMPARE(store->changeCount(), 0u);
			QCOMPARE(incrementSpy.size(), 1);
		} else
			QVERIFY(incrementSpy.isEmpty());
	} catch(QException &e) {
		QFAIL(e.what());
	}

	controller->clearUploads();
}

void TestChangeController::testDeviceChanges()
{
	controller->setUploadingEnabled(false);
	QCoreApplication::processEvents();
	QSignalSpy activeSpy(controller, &ChangeController::uploadingChanged);
	QSignalSpy changeSpy(controller, &ChangeController::uploadChange);
	QSignalSpy deviceChangeSpy(controller, &ChangeController::uploadDeviceChange);
	QSignalSpy addedSpy(controller, &ChangeController::progressAdded);
	QSignalSpy incrementSpy(controller, &ChangeController::progressIncrement);
	QSignalSpy errorSpy(controller, &ChangeController::controllerError);

	try {
		auto devId = QUuid::createUuid();

		//Create the device changes
		store->reset(false);
		store->save(TestLib::generateKey(42), TestLib::generateDataJson(42));
		store->save(TestLib::generateKey(43), TestLib::generateDataJson(43));
		store->remove(TestLib::generateKey(43));
		store->save(TestLib::generateKey(44), TestLib::generateDataJson(44));
		store->save(TestLib::generateKey(45), TestLib::generateDataJson(45));
		store->remove(TestLib::generateKey(45));
		store->markUnchanged(TestLib::generateKey(42), 1, false);
		store->markUnchanged(TestLib::generateKey(43), 2, false); //fake persistet
		store->prepareAccountAdded(devId);

		//enable changes
		controller->setUploadingEnabled(true);
		if(!errorSpy.isEmpty())
			QFAIL(errorSpy.takeFirst()[0].toString().toUtf8().constData());

		QCOMPARE(changeSpy.size(), 2);
		QCOMPARE(deviceChangeSpy.size(), 3);
		QCOMPARE(addedSpy.size(), 1);
		QCOMPARE(addedSpy.takeFirst()[0].toUInt(), 5u);
		QVERIFY(activeSpy.last()[0].toBool());
		QCOMPARE(store->changeCount(), 5u);

		auto change = changeSpy.takeFirst();
		auto keyHash = change[0].toByteArray();
		controller->uploadDone(keyHash);
		QCOMPARE(changeSpy.size(), 1);
		QCOMPARE(deviceChangeSpy.size(), 3);
		QCOMPARE(store->changeCount(), 4u);
		QCOMPARE(incrementSpy.size(), 1);

		change = changeSpy.takeFirst();
		keyHash = change[0].toByteArray();
		controller->uploadDone(keyHash);
		QCOMPARE(changeSpy.size(), 0);
		QCOMPARE(deviceChangeSpy.size(), 3);
		QCOMPARE(store->changeCount(), 3u);
		QCOMPARE(incrementSpy.size(), 2);

		change = deviceChangeSpy.takeFirst();
		keyHash = change[0].toByteArray();
		controller->deviceUploadDone(keyHash, devId);
		QCOMPARE(changeSpy.size(), 0);
		QCOMPARE(deviceChangeSpy.size(), 2);
		QCOMPARE(store->changeCount(), 2u);
		QCOMPARE(incrementSpy.size(), 3);

		change = deviceChangeSpy.takeFirst();
		keyHash = change[0].toByteArray();
		controller->deviceUploadDone(keyHash, devId);
		QCOMPARE(changeSpy.size(), 0);
		QCOMPARE(deviceChangeSpy.size(), 1);
		QCOMPARE(store->changeCount(), 1u);
		QCOMPARE(incrementSpy.size(), 4);

		change = deviceChangeSpy.takeFirst();
		keyHash = change[0].toByteArray();
		controller->deviceUploadDone(keyHash, devId);
		QCOMPARE(changeSpy.size(), 0);
		QCOMPARE(deviceChangeSpy.size(), 0);
		QCOMPARE(store->changeCount(), 0u);
		QCOMPARE(incrementSpy.size(), 5);

		QVERIFY(!deviceChangeSpy.wait());
		QVERIFY(changeSpy.isEmpty());
		QVERIFY(deviceChangeSpy.isEmpty());

		store->reset(false);
	} catch(QException &e) {
		QFAIL(e.what());
	}
	controller->clearUploads();
}

void TestChangeController::testChangeTriggers()
{
	for(auto i = 0; i < 5; i++) { //wait for the engine to init itself
		QCoreApplication::processEvents();
		QThread::msleep(100);
	}

	//DIRTY HACK: force the engine pass the local controller to anyone asking
	auto engine = SetupPrivate::engine(DefaultSetup);
	auto old = engine->_changeController;
	engine->_changeController = controller;

	//enable changes
	controller->setUploadingEnabled(true);
	QCoreApplication::processEvents();

	QSignalSpy activeSpy(controller, &ChangeController::uploadingChanged);
	QSignalSpy changeSpy(controller, &ChangeController::uploadChange);
	QSignalSpy addedSpy(controller, &ChangeController::progressAdded);
	QSignalSpy incrementSpy(controller, &ChangeController::progressIncrement);
	QSignalSpy errorSpy(controller, &ChangeController::controllerError);

	try {
		//clear
		store->reset(false);
		QVERIFY(changeSpy.isEmpty());
		QVERIFY(errorSpy.isEmpty());

		auto key = TestLib::generateKey(10);
		auto data = TestLib::generateDataJson(10);

		//save data
		store->save(key, data);
		QVERIFY(changeSpy.wait());
		QVERIFY(activeSpy.last()[0].toBool());
		QCOMPARE(addedSpy.size(), 1);
		QCOMPARE(addedSpy.takeFirst()[0].toUInt(), 1u);

		if(!errorSpy.isEmpty())
			QFAIL(errorSpy.takeFirst()[0].toString().toUtf8().constData());

		QCOMPARE(changeSpy.size(), 1);
		auto change = changeSpy.takeFirst();
		auto keyHash = change[0].toByteArray();
		QCOMPARE(change[1].toByteArray(), SyncHelper::combine(key, 1, data));

		controller->uploadDone(keyHash);
		QVERIFY(activeSpy.wait());
		QVERIFY(!activeSpy.last()[0].toBool());
		QVERIFY(changeSpy.isEmpty());
		QCOMPARE(incrementSpy.size(), 1);
		QVERIFY(errorSpy.isEmpty());

		//now a delete
		store->remove(key);
		QVERIFY(changeSpy.wait());
		QVERIFY(activeSpy.last()[0].toBool());
		QCOMPARE(addedSpy.size(), 1);
		QCOMPARE(addedSpy.takeFirst()[0].toUInt(), 1u);

		if(!errorSpy.isEmpty())
			QFAIL(errorSpy.takeFirst()[0].toString().toUtf8().constData());

		QCOMPARE(changeSpy.size(), 1);
		change = changeSpy.takeFirst();
		keyHash = change[0].toByteArray();
		QCOMPARE(change[1].toByteArray(), SyncHelper::combine(key, 2));

		controller->uploadDone(keyHash);
		QVERIFY(activeSpy.wait());
		QVERIFY(!activeSpy.last()[0].toBool());
		QVERIFY(changeSpy.isEmpty());
		QVERIFY(errorSpy.isEmpty());
		QCOMPARE(incrementSpy.size(), 2);

		engine->_changeController = old;
	} catch(QException &e) {
		engine->_changeController = old;
		QFAIL(e.what());
	}
}

void TestChangeController::testPassiveTriggers()
{
	for(auto i = 0; i < 5; i++) { //wait for the engine to init itself
		QCoreApplication::processEvents();
		QThread::msleep(100);
	}

	//DIRTY HACK: force the engine pass the local controller to anyone asking
	auto engine = SetupPrivate::engine(DefaultSetup);
	auto old = engine->_changeController;
	engine->_changeController = controller;

	//enable changes
	controller->setUploadingEnabled(true);
	QCoreApplication::processEvents();

	QSignalSpy changeSpy(controller, &ChangeController::uploadChange);

	try {
		auto nName = QStringLiteral("setup2");
		Setup setup;
		TestLib::setup(setup);
		setup.setRemoteObjectHost(QStringLiteral("threaded:/qtdatasync/default/enginenode"));
		QVERIFY(setup.createPassive(nName, 5000));

		auto key = TestLib::generateKey(22);
		auto data = TestLib::generateDataJson(22);

		{
			LocalStore second(DefaultsPrivate::obtainDefaults(nName));
			second.save(key, data);
			QVERIFY(changeSpy.wait());
			QCOMPARE(changeSpy.size(), 1);
			auto change = changeSpy.takeFirst();
			QCOMPARE(change[1].toByteArray(), SyncHelper::combine(key, 1, data));
		}

		Setup::removeSetup(nName);
		engine->_changeController = old;
	} catch(QException &e) {
		engine->_changeController = old;
		QFAIL(e.what());
	}
}

QTEST_MAIN(TestChangeController)

#include "tst_changecontroller.moc"
