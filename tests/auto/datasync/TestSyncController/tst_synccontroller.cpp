#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <QtDataSync/private/synccontroller_p.h>
#include <QtDataSync/private/synchelper_p.h>

//fake private
#define private public
#include <QtDataSync/private/defaults_p.h>
#undef private

#include "testresolver.h"
using namespace QtDataSync;

class TestSyncController : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSyncOperation_data();
	void testSyncOperation();

	void testResolver_data();
	void testResolver();

private:
	LocalStore *store;
	SyncController *controller;
};

void TestSyncController::initTestCase()
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

		store = new LocalStore(DefaultsPrivate::obtainDefaults(DefaultSetup), this);
		controller = new SyncController(DefaultsPrivate::obtainDefaults(DefaultSetup), this);
		controller->initialize({{QStringLiteral("store"), QVariant::fromValue(store)}});
		controller->setSyncEnabled(true);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestSyncController::cleanupTestCase()
{
	controller->finalize();
	delete controller;
	controller = nullptr;
	delete store;
	store = nullptr;
	Setup::removeSetup(DefaultSetup, true);
}

void TestSyncController::testSyncOperation_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<bool>("persistent");
	QTest::addColumn<Setup::SyncPolicy>("policy");

	QTest::addColumn<LocalStore::ChangeType>("localChange");
	QTest::addColumn<quint64>("localVersion");
	QTest::addColumn<QJsonObject>("localData");
	QTest::addColumn<bool>("localChanged");

	QTest::addColumn<quint64>("remoteVersion");
	QTest::addColumn<QJsonObject>("remoteData");

	QTest::addColumn<LocalStore::ChangeType>("resultState");
	QTest::addColumn<quint64>("resultVersion");
	QTest::addColumn<QJsonObject>("resultData");
	QTest::addColumn<bool>("resultChanged");

	//exists<->deleted
	QTest::newRow("exists<->deleted:v1<v2:discarding") << TestLib::generateKey(10)
													   << false
													   << Setup::PreferChanged
													   << LocalStore::Exists
													   << 5ull
													   << TestLib::generateDataJson(10, QStringLiteral("local"))
													   << false
													   << 10ull
													   << QJsonObject()
													   << LocalStore::ExistsDeleted
													   << 10ull
													   << QJsonObject()
													   << true;
	QTest::newRow("exists<->deleted:v1<v2:persistent") << TestLib::generateKey(10)
													   << true
													   << Setup::PreferChanged
													   << LocalStore::Exists
													   << 5ull
													   << TestLib::generateDataJson(10, QStringLiteral("local"))
													   << true
													   << 10ull
													   << QJsonObject()
													   << LocalStore::ExistsDeleted
													   << 10ull
													   << QJsonObject()
													   << false;
	QTest::newRow("exists<->deleted:v1>v2") << TestLib::generateKey(10)
											<< false
											<< Setup::PreferChanged
											<< LocalStore::Exists
											<< 10ull
											<< TestLib::generateDataJson(10, QStringLiteral("local"))
											<< true
											<< 5ull
											<< QJsonObject()
											<< LocalStore::Exists
											<< 10ull
											<< TestLib::generateDataJson(10, QStringLiteral("local"))
											<< true;
	QTest::newRow("exists<->deleted:v1=v2:p(changed)") << TestLib::generateKey(10)
													   << false
													   << Setup::PreferChanged
													   << LocalStore::Exists
													   << 10ull
													   << TestLib::generateDataJson(10, QStringLiteral("local"))
													   << false
													   << 10ull
													   << QJsonObject()
													   << LocalStore::Exists
													   << 11ull
													   << TestLib::generateDataJson(10, QStringLiteral("local"))
													   << true;
	QTest::newRow("exists<->deleted:v1=v2:p(deleted)") << TestLib::generateKey(10)
													   << false
													   << Setup::PreferDeleted
													   << LocalStore::Exists
													   << 10ull
													   << TestLib::generateDataJson(10, QStringLiteral("local"))
													   << false
													   << 10ull
													   << QJsonObject()
													   << LocalStore::ExistsDeleted
													   << 11ull
													   << QJsonObject()
													   << true;

	//exists<->changed
	QTest::newRow("exists<->changed:v1<v2") << TestLib::generateKey(10)
											<< false
											<< Setup::PreferChanged
											<< LocalStore::Exists
											<< 5ull
											<< TestLib::generateDataJson(10, QStringLiteral("local"))
											<< true
											<< 10ull
											<< TestLib::generateDataJson(10, QStringLiteral("remote"))
											<< LocalStore::Exists
											<< 10ull
											<< TestLib::generateDataJson(10, QStringLiteral("remote"))
											<< false;
	QTest::newRow("exists<->changed:v1>v2") << TestLib::generateKey(10)
											<< false
											<< Setup::PreferChanged
											<< LocalStore::Exists
											<< 10ull
											<< TestLib::generateDataJson(10, QStringLiteral("local"))
											<< true
											<< 5ull
											<< TestLib::generateDataJson(10, QStringLiteral("remote"))
											<< LocalStore::Exists
											<< 10ull
											<< TestLib::generateDataJson(10, QStringLiteral("local"))
											<< true;
	QTest::newRow("exists<->changed:v1=v2:c1=c2") << TestLib::generateKey(10)
												  << false
												  << Setup::PreferChanged
												  << LocalStore::Exists
												  << 10ull
												  << TestLib::generateDataJson(10)
												  << true
												  << 10ull
												  << TestLib::generateDataJson(10)
												  << LocalStore::Exists
												  << 10ull
												  << TestLib::generateDataJson(10)
												  << false;
	QTest::newRow("exists<->changed:v1=v2:c1<c2") << TestLib::generateKey(10)
												  << false
												  << Setup::PreferChanged
												  << LocalStore::Exists
												  << 10ull
												  << TestLib::generateDataJson(10, QStringLiteral("dataB"))
												  << false
												  << 10ull
												  << TestLib::generateDataJson(10, QStringLiteral("dataA"))
												  << LocalStore::Exists
												  << 11ull
												  << TestLib::generateDataJson(10, QStringLiteral("dataB"))
												  << true;
	QTest::newRow("exists<->changed:v1=v2:c1>c2") << TestLib::generateKey(10)
												  << false
												  << Setup::PreferChanged
												  << LocalStore::Exists
												  << 10ull
												  << TestLib::generateDataJson(10, QStringLiteral("dataA"))
												  << false
												  << 10ull
												  << TestLib::generateDataJson(10, QStringLiteral("dataB"))
												  << LocalStore::Exists
												  << 11ull
												  << TestLib::generateDataJson(10, QStringLiteral("dataB"))
												  << true;

	//cachedDelete<->deleted
	QTest::newRow("cachedDelete<->deleted:v1<v2:discarding") << TestLib::generateKey(10)
															 << false
															 << Setup::PreferChanged
															 << LocalStore::ExistsDeleted
															 << 5ull
															 << QJsonObject()
															 << true
															 << 10ull
															 << QJsonObject()
															 << LocalStore::NoExists
															 << 0ull
															 << QJsonObject()
															 << false;
	QTest::newRow("cachedDelete<->deleted:v1<v2:persisting") << TestLib::generateKey(10)
															 << true
															 << Setup::PreferChanged
															 << LocalStore::ExistsDeleted
															 << 5ull
															 << QJsonObject()
															 << true
															 << 10ull
															 << QJsonObject()
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << false;
	QTest::newRow("cachedDelete<->deleted:v1=v2:discarding") << TestLib::generateKey(10)
															 << false
															 << Setup::PreferChanged
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << true
															 << 10ull
															 << QJsonObject()
															 << LocalStore::NoExists
															 << 0ull
															 << QJsonObject()
															 << false;
	QTest::newRow("cachedDelete<->deleted:v1=v2:persisting") << TestLib::generateKey(10)
															 << true
															 << Setup::PreferChanged
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << true
															 << 10ull
															 << QJsonObject()
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << false;
	QTest::newRow("cachedDelete<->deleted:v1>v2:discarding") << TestLib::generateKey(10)
															 << false
															 << Setup::PreferChanged
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << true
															 << 5ull
															 << QJsonObject()
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << true;
	QTest::newRow("cachedDelete<->deleted:v1>v2:persisting") << TestLib::generateKey(10)
															 << true
															 << Setup::PreferChanged
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << false
															 << 5ull
															 << QJsonObject()
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << false;

	//cachedDelete<->changed
	QTest::newRow("cachedDelete<->changed:v1<v2") << TestLib::generateKey(10)
												  << false
												  << Setup::PreferChanged
												  << LocalStore::ExistsDeleted
												  << 5ull
												  << QJsonObject()
												  << true
												  << 10ull
												  << TestLib::generateDataJson(10, QStringLiteral("remote"))
												  << LocalStore::Exists
												  << 10ull
												  << TestLib::generateDataJson(10, QStringLiteral("remote"))
												  << false;
	QTest::newRow("cachedDelete<->changed:v1>v2") << TestLib::generateKey(10)
												  << false
												  << Setup::PreferChanged
												  << LocalStore::ExistsDeleted
												  << 10ull
												  << QJsonObject()
												  << true
												  << 5ull
												  << TestLib::generateDataJson(10, QStringLiteral("remote"))
												  << LocalStore::ExistsDeleted
												  << 10ull
												  << QJsonObject()
												  << true;
	QTest::newRow("cachedDelete<->changed:v1=v2:p(changed)") << TestLib::generateKey(10)
															 << false
															 << Setup::PreferChanged
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << false
															 << 10ull
															 << TestLib::generateDataJson(10, QStringLiteral("remote"))
															 << LocalStore::Exists
															 << 11ull
															 << TestLib::generateDataJson(10, QStringLiteral("remote"))
															 << true;
	QTest::newRow("cachedDelete<->changed:v1=v2:p(deleted)") << TestLib::generateKey(10)
															 << false
															 << Setup::PreferDeleted
															 << LocalStore::ExistsDeleted
															 << 10ull
															 << QJsonObject()
															 << false
															 << 10ull
															 << TestLib::generateDataJson(10, QStringLiteral("remote"))
															 << LocalStore::ExistsDeleted
															 << 11ull
															 << QJsonObject()
															 << true;

	//noexists<->deleted
	QTest::newRow("noexists<->deleted:discarding") << TestLib::generateKey(10)
												   << false
												   << Setup::PreferChanged
												   << LocalStore::NoExists
												   << 0ull
												   << QJsonObject()
												   << false
												   << 10ull
												   << QJsonObject()
												   << LocalStore::NoExists
												   << 0ull
												   << QJsonObject()
												   << false;
	QTest::newRow("noexists<->deleted:persisting") << TestLib::generateKey(10)
												   << true
												   << Setup::PreferChanged
												   << LocalStore::NoExists
												   << 0ull
												   << QJsonObject()
												   << false
												   << 10ull
												   << QJsonObject()
												   << LocalStore::ExistsDeleted
												   << 10ull
												   << QJsonObject()
												   << false;

	//noexists<->changed
	QTest::newRow("noexists<->changed") << TestLib::generateKey(10)
										<< false
										<< Setup::PreferChanged
										<< LocalStore::NoExists
										<< 0ull
										<< QJsonObject()
										<< false
										<< 10ull
										<< TestLib::generateDataJson(10, QStringLiteral("remote"))
										<< LocalStore::Exists
										<< 10ull
										<< TestLib::generateDataJson(10, QStringLiteral("remote"))
										<< false;
}

void TestSyncController::testSyncOperation()
{
	QFETCH(ObjectKey, key);
	QFETCH(bool, persistent);
	QFETCH(Setup::SyncPolicy, policy);

	QFETCH(LocalStore::ChangeType, localChange);
	QFETCH(quint64, localVersion);
	QFETCH(QJsonObject, localData);
	QFETCH(bool, localChanged);

	QFETCH(quint64, remoteVersion);
	QFETCH(QJsonObject, remoteData);

	QFETCH(LocalStore::ChangeType, resultState);
	QFETCH(quint64, resultVersion);
	QFETCH(QJsonObject, resultData);
	QFETCH(bool, resultChanged);

	QSignalSpy doneSpy(controller, &SyncController::syncDone);
	QSignalSpy errorSpy(controller, &SyncController::controllerError);

	try {
		store->reset(false);

		auto dPriv = DefaultsPrivate::obtainDefaults(DefaultSetup);
		dPriv->properties.insert(Defaults::PersistDeleted, persistent);
		dPriv->properties.insert(Defaults::ConflictPolicy, policy);

		//step 1: setup the local store
		{
			auto scope = store->startSync(key);
			switch ((LocalStore::ChangeType)localChange) {
			case QtDataSync::LocalStore::Exists:
				store->storeChanged(scope, localVersion, QString(), localData, localChanged, LocalStore::NoExists);
				break;
			case QtDataSync::LocalStore::ExistsDeleted:
				store->storeDeleted(scope, localVersion, localChanged, LocalStore::NoExists);
				break;
			case QtDataSync::LocalStore::NoExists: //assume no exists, so nothing to be done
				break;
			default:
				Q_UNREACHABLE();
				break;
			}
			store->commitSync(scope);
		}

		//step 2: generate the remote message
		QByteArray message;
		if(remoteData.isEmpty())
			message = SyncHelper::combine(key, remoteVersion);
		else
			message = SyncHelper::combine(key, remoteVersion, remoteData);

		//step 3: trigger the change
		QVERIFY(doneSpy.isEmpty());
		controller->syncChange(42ull, message);
		if(!errorSpy.isEmpty())
			QFAIL(errorSpy.takeFirst()[0].toString().toUtf8().constData());
		QCOMPARE(doneSpy.size(), 1);
		QCOMPARE(doneSpy.takeFirst()[0].toULongLong(), 42ull);

		//step 4: validate the result data
		{
			auto scope = store->startSync(key);
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), resultState);
			QCOMPARE(std::get<1>(info), resultVersion);
			if(resultState == LocalStore::Exists) {
				auto tJson = store->readJson(key, std::get<2>(info));
				QCOMPARE(tJson, resultData);
				QCOMPARE(std::get<3>(info), SyncHelper::jsonHash(resultData));
			}
			store->commitSync(scope);
		}

		//step 5: verify changed
		auto called = false;
		store->loadChanges(1000, [key, resultVersion, &called](ObjectKey k, quint64 v, QString, QUuid) -> bool {
			if(k == key) {
				if (!QTest::qCompare(v, resultVersion, "v", "resultVersion", __FILE__, __LINE__))
					return false;
				called = true;
				return false;
			} else
				return true;
		});
		QCOMPARE(called, resultChanged);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestSyncController::testResolver_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<QJsonObject>("localData");
	QTest::addColumn<QJsonObject>("remoteData");
	QTest::addColumn<QJsonObject>("resultData");

	QTest::newRow("conflict:local") << TestLib::generateKey(10)
									<< TestLib::generateDataJson(10, QStringLiteral("dataA"))
									<< TestLib::generateDataJson(10, QStringLiteral("dataB"))
									<< TestLib::generateDataJson(10, QStringLiteral("dataA+conflict"));
	QTest::newRow("conflict:remote") << TestLib::generateKey(10)
									 << TestLib::generateDataJson(10, QStringLiteral("dataB"))
									 << TestLib::generateDataJson(10, QStringLiteral("dataA"))
									 << TestLib::generateDataJson(10, QStringLiteral("dataA+conflict"));
}

void TestSyncController::testResolver()
{
	QFETCH(ObjectKey, key);
	QFETCH(QJsonObject, localData);
	QFETCH(QJsonObject, remoteData);
	QFETCH(QJsonObject, resultData);
	QSignalSpy doneSpy(controller, &SyncController::syncDone);
	QSignalSpy errorSpy(controller, &SyncController::controllerError);

	auto version = 10ull;

	try {
		store->reset(false);

		auto dPriv = DefaultsPrivate::obtainDefaults(DefaultSetup);
		auto oldRes = dPriv->resolver;
		dPriv->resolver = new TestResolver(controller);
		dPriv->resolver->setDefaults(dPriv);

		//step 1: setup the local store
		{
			auto scope = store->startSync(key);
			store->storeChanged(scope, version, QString(), localData, false, LocalStore::NoExists);
			store->commitSync(scope);
		}

		//step 2: generate the remote message
		auto message = SyncHelper::combine(key, version, remoteData);

		//step 3: trigger the change
		QVERIFY(doneSpy.isEmpty());
		controller->syncChange(42ull, message);
		if(!errorSpy.isEmpty())
			QFAIL(errorSpy.takeFirst()[0].toString().toUtf8().constData());
		QCOMPARE(doneSpy.size(), 1);
		QCOMPARE(doneSpy.takeFirst()[0].toULongLong(), 42ull);

		//step 4: validate the result data
		{
			auto scope = store->startSync(key);
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::Exists);
			QCOMPARE(std::get<1>(info), version + 1ull);
			auto tJson = store->readJson(key, std::get<2>(info));
			QCOMPARE(tJson, resultData);
			QCOMPARE(std::get<3>(info), SyncHelper::jsonHash(resultData));
			store->commitSync(scope);
		}

		//step 5: verify changed
		auto called = false;
		store->loadChanges(1000, [key, version, &called](ObjectKey k, quint64 v, QString, QUuid) -> bool {
			if(k == key) {
				if (!QTest::qCompare(v, version + 1ull, "v", "resultVersion", __FILE__, __LINE__))
					return false;
				called = true;
				return false;
			} else
				return true;
		});
		QVERIFY(called);

		dPriv->resolver->deleteLater();
		dPriv->resolver = oldRes;
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(TestSyncController)

#include "tst_synccontroller.moc"
