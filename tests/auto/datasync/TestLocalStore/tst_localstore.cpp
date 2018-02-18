#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QtConcurrent>
#include <testlib.h>
#include <QtDataSync/private/localstore_p.h>
#include <QtDataSync/private/defaults_p.h>
using namespace QtDataSync;

class TestLocalStore : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	//normal access
	void testEmpty();
	void testSave_data();
	void testSave();
	void testAll();
	void testFind_data();
	void testFind();
	void testRemove_data();
	void testRemove();
	void testClear();

	//change access
	void testChangeLoading();
	void testMarkUnchanged();
	void testDeviceChanges();

	//sync access
	void testInfoLoading();
	void testInfoOperations();

	//special
	void testChangeSignals();
	void testAsync();
	void testPassiveSetup();

private:
	LocalStore *store;
};

void TestLocalStore::initTestCase()
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
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::cleanupTestCase()
{
	delete store;
	store = nullptr;
	Setup::removeSetup(DefaultSetup, true);
}

void TestLocalStore::testEmpty()
{
	try {
		QCOMPARE(store->count(TestLib::TypeName), 0ull);
		QVERIFY(store->keys(TestLib::TypeName).isEmpty());
		QVERIFY_EXCEPTION_THROWN(store->load(TestLib::generateKey(1)), NoDataException);
		QCOMPARE(store->remove(TestLib::generateKey(2)), false);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testSave_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<QJsonObject>("data");

	QTest::newRow("data0") << TestLib::generateKey(429)
						   << TestLib::generateDataJson(429);
	QTest::newRow("data1") << TestLib::generateKey(430)
						   << TestLib::generateDataJson(430);
	QTest::newRow("data2") << TestLib::generateKey(431)
						   << TestLib::generateDataJson(431);
	QTest::newRow("data3") << TestLib::generateKey(432)
						   << TestLib::generateDataJson(432);
}

void TestLocalStore::testSave()
{
	QFETCH(ObjectKey, key);
	QFETCH(QJsonObject, data);

	try {
		store->save(key, data);
		auto res = store->load(key);
		QCOMPARE(res, data);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testAll()
{
	const quint64 count = 4;
	const QStringList keys {
		QStringLiteral("429"),
		QStringLiteral("430"),
		QStringLiteral("431"),
		QStringLiteral("432")
	};
	const QList<QJsonObject> objects = TestLib::generateDataJson(429, 432).values();

	try {
		QCOMPARE(store->count(TestLib::TypeName), count);
		QCOMPAREUNORDERED(store->keys(TestLib::TypeName), keys);
		QCOMPAREUNORDERED(store->loadAll(TestLib::TypeName), objects);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testFind_data()
{
	QTest::addColumn<QString>("query");
	QTest::addColumn<DataStore::SearchMode>("mode");
	QTest::addColumn<QList<QJsonObject>>("result");

	QTest::newRow("regexp") << QStringLiteral(R"__((3(?!1)|.*2.+))__")
							<< DataStore::RegexpMode
							<< QList<QJsonObject> {
								  TestLib::generateDataJson(429),
								  TestLib::generateDataJson(430),
								  TestLib::generateDataJson(432)
							   };
	QTest::newRow("wildcard") << QStringLiteral("*2*")
							  << DataStore::WildcardMode
							  << QList<QJsonObject> {
									TestLib::generateDataJson(429),
									TestLib::generateDataJson(432)
								 };
	QTest::newRow("startswith") << QStringLiteral("4")
								<< DataStore::StartsWithMode
								<< QList<QJsonObject> {
									  TestLib::generateDataJson(429),
									  TestLib::generateDataJson(430),
									  TestLib::generateDataJson(431),
									  TestLib::generateDataJson(432)
								   };
	QTest::newRow("endswith") << QStringLiteral("1")
							  << DataStore::EndsWithMode
							  << QList<QJsonObject> {
									TestLib::generateDataJson(431)
								 };
	QTest::newRow("contains") << QStringLiteral("2")
							  << DataStore::ContainsMode
							  << QList<QJsonObject> {
									TestLib::generateDataJson(429),
									TestLib::generateDataJson(432)
								 };
}

void TestLocalStore::testFind()
{
	QFETCH(QString, query);
	QFETCH(DataStore::SearchMode, mode);
	QFETCH(QList<QJsonObject>, result);

	try {
		QCOMPAREUNORDERED(store->find(TestLib::TypeName, query, mode), result);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testRemove_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<bool>("result");

	QTest::newRow("data0") << TestLib::generateKey(429)
						   << true;
	QTest::newRow("data1") << TestLib::generateKey(430)
						   << true;
	QTest::newRow("data2") << TestLib::generateKey(430)
						   << false;
}

void TestLocalStore::testRemove()
{
	QFETCH(ObjectKey, key);
	QFETCH(bool, result);

	try {
		QCOMPARE(store->remove(key), result);
		QVERIFY_EXCEPTION_THROWN(store->load(key), NoDataException);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testClear()
{
	QSignalSpy clearSpy(store, &LocalStore::dataCleared);
	QSignalSpy resetSpy(store, &LocalStore::dataResetted);

	try {
		//clear
		QCOMPARE(store->count(TestLib::TypeName), 2ull);
		store->clear(TestLib::TypeName);
		QCOMPARE(store->count(TestLib::TypeName), 0ull);
		QCOMPARE(clearSpy.count(), 1);
		QCOMPARE(clearSpy.takeFirst()[0].toByteArray(), TestLib::TypeName);

		//reset
		store->save(TestLib::generateKey(42), TestLib::generateDataJson(42));
		QCOMPARE(store->count(TestLib::TypeName), 1ull);
		store->reset(false);
		QCOMPARE(store->count(TestLib::TypeName), 0ull);
		QCOMPARE(resetSpy.count(), 1);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testChangeLoading()
{
	try {
		store->reset(false);

		//create 2 changes
		store->save(TestLib::generateKey(42), TestLib::generateDataJson(42));
		store->save(TestLib::generateKey(13), TestLib::generateDataJson(13));
		store->remove(TestLib::generateKey(13));

		QCOMPARE(store->changeCount(), 2u);
		//test loadChanges behaviours
		auto cCount = 0;
		store->loadChanges(10, [&](ObjectKey, quint64, QString, QUuid) {
			cCount++;
			return true;
		});
		QCOMPARE(cCount, 2);
		cCount = 0;
		store->loadChanges(10, [&](ObjectKey, quint64, QString, QUuid) {
			cCount++;
			return false;
		});
		QCOMPARE(cCount, 1);
		cCount = 0;
		store->loadChanges(1, [&](ObjectKey, quint64, QString, QUuid) {
			cCount++;
			return true;
		});
		QCOMPARE(cCount, 1);
		//verify contents
		store->loadChanges(10, [&](ObjectKey k, quint64 v, QString f, QUuid d) {
			[&](){
				if(k == TestLib::generateKey(42)) {
					QCOMPARE(k, TestLib::generateKey(42));
					QCOMPARE(v, 1ull);
					QVERIFY(!f.isNull());
					QVERIFY(d.isNull());
				} else {
					QCOMPARE(k, TestLib::generateKey(13));
					QCOMPARE(v, 2ull); //because of the delete
					QVERIFY(f.isNull());
					QVERIFY(d.isNull());
				}
			}();
			return true;
		});
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testMarkUnchanged()
{
	try {
		QCOMPARE(store->changeCount(), 2u);

		//mark data unchanged
		store->markUnchanged(TestLib::generateKey(42), 1, false);
		QCOMPARE(store->changeCount(), 1u);

		//mark deleted unchanged with wrong version
		store->markUnchanged(TestLib::generateKey(13), 1, true);
		QCOMPARE(store->changeCount(), 1u);

		//mark deleted unchanged
		store->markUnchanged(TestLib::generateKey(13), 2, true);
		QCOMPARE(store->changeCount(), 0u);

		//mark everything existing as changed
		store->reset(true);
		QCOMPARE(store->changeCount(), 1u);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testDeviceChanges()
{
	try {
		store->reset(false);
		store->save(TestLib::generateKey(42), TestLib::generateDataJson(42));
		store->save(TestLib::generateKey(43), TestLib::generateDataJson(43));
		store->markUnchanged(TestLib::generateKey(42), 1, false);
		store->markUnchanged(TestLib::generateKey(43), 1, false);
		QCOMPARE(store->changeCount(), 0u);

		//trigger device change
		auto devId = QUuid::createUuid();
		store->prepareAccountAdded(devId);
		QCOMPARE(store->count(TestLib::TypeName), 2ull);
		QCOMPARE(store->changeCount(), 2u);

		//test loadChanges behaviours
		auto cCount = 0;
		store->loadChanges(10, [&](ObjectKey, quint64, QString, QUuid) {
			cCount++;
			return true;
		});
		QCOMPARE(cCount, 2);
		cCount = 0;
		store->loadChanges(10, [&](ObjectKey, quint64, QString, QUuid) {
			cCount++;
			return false;
		});
		QCOMPARE(cCount, 1);
		cCount = 0;
		store->loadChanges(1, [&](ObjectKey, quint64, QString, QUuid) {
			cCount++;
			return true;
		});
		QCOMPARE(cCount, 1);
		//verify contents
		store->loadChanges(10, [&](ObjectKey k, quint64 v, QString f, QUuid d) {
			[&](){
				if(k == TestLib::generateKey(42)) {
					QCOMPARE(k, TestLib::generateKey(42));
					QCOMPARE(v, 1ull);
					QVERIFY(!f.isNull());
					QCOMPARE(d, devId);
				} else {
					QCOMPARE(k, TestLib::generateKey(43));
					QCOMPARE(v, 1ull);
					QVERIFY(!f.isNull());
					QCOMPARE(d, devId);
				}
			}();
			return true;
		});

		store->removeDeviceChange(TestLib::generateKey(42), QUuid::createUuid());
		QCOMPARE(store->changeCount(), 2u);
		store->removeDeviceChange(TestLib::generateKey(42), devId);
		QCOMPARE(store->changeCount(), 1u);
		store->remove(TestLib::generateKey(43));
		QCOMPARE(store->changeCount(), 1u);
		store->markUnchanged(TestLib::generateKey(43), 2, true);
		QCOMPARE(store->changeCount(), 0u);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testInfoLoading()
{
	try {
		store->reset(false);
		store->save(TestLib::generateKey(42), TestLib::generateDataJson(42));
		store->save(TestLib::generateKey(43), TestLib::generateDataJson(43));
		store->remove(TestLib::generateKey(43));

		{
			auto scope = store->startSync(TestLib::generateKey(42));
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::Exists);
			QCOMPARE(std::get<1>(info), 1ull);
			QVERIFY(!std::get<2>(info).isNull());
			QVERIFY(!std::get<3>(info).isNull());
			store->commitSync(scope);
		}
		{
			auto scope = store->startSync(TestLib::generateKey(43));
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::ExistsDeleted);
			QCOMPARE(std::get<1>(info), 2ull);
			QVERIFY(std::get<2>(info).isNull());
			QVERIFY(std::get<3>(info).isNull());
			store->commitSync(scope);
		}
		{
			auto scope = store->startSync(TestLib::generateKey(44));
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::NoExists);
			QCOMPARE(std::get<1>(info), 0ull);
			QVERIFY(std::get<2>(info).isNull());
			QVERIFY(std::get<3>(info).isNull());
			store->commitSync(scope);
		}
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testInfoOperations()
{
	try {
		QCOMPARE(store->changeCount(), 2u);

		//update the version and mark unchanged
		{
			auto scope = store->startSync(TestLib::generateKey(42));
			store->updateVersion(scope, 1ull, 4ull, false);
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::Exists);
			QCOMPARE(std::get<1>(info), 4ull);
			QVERIFY(!std::get<2>(info).isNull());
			QVERIFY(!std::get<3>(info).isNull());
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 1u);
		}

		//update the version with wrong currentversion
		{
			auto scope = store->startSync(TestLib::generateKey(42));
			store->updateVersion(scope, 1ull, 5ull, false);
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::Exists);
			QCOMPARE(std::get<1>(info), 4ull);
			QVERIFY(!std::get<2>(info).isNull());
			QVERIFY(!std::get<3>(info).isNull());
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 1u);
		}

		//update the version with change
		{
			auto scope = store->startSync(TestLib::generateKey(42));
			store->updateVersion(scope, 4ull, 5ull, true);
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::Exists);
			QCOMPARE(std::get<1>(info), 5ull);
			QVERIFY(!std::get<2>(info).isNull());
			QVERIFY(!std::get<3>(info).isNull());
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 2u);
		}

		//store changed of new data
		{
			auto scope = store->startSync(TestLib::generateKey(44));
			store->storeChanged(scope, 11ull, QString(), TestLib::generateDataJson(44), true, LocalStore::NoExists);
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::Exists);
			QCOMPARE(std::get<1>(info), 11ull);
			QVERIFY(!std::get<2>(info).isNull());
			QVERIFY(!std::get<3>(info).isNull());
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 3u);
			QCOMPARE(store->load(TestLib::generateKey(44)), TestLib::generateDataJson(44));
		}

		//store changed of existing data
		{
			auto scope = store->startSync(TestLib::generateKey(42));
			auto info = store->loadChangeInfo(scope);
			store->storeChanged(scope, 20ull, std::get<2>(info), TestLib::generateDataJson(24), false, LocalStore::Exists);
			info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::Exists);
			QCOMPARE(std::get<1>(info), 20ull);
			QVERIFY(!std::get<2>(info).isNull());
			QVERIFY(!std::get<3>(info).isNull());
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 2u);
			QCOMPARE(store->load(TestLib::generateKey(42)), TestLib::generateDataJson(24));
		}

		//store delete of existing
		{
			auto scope = store->startSync(TestLib::generateKey(42));
			store->storeDeleted(scope, 30ull, true, LocalStore::Exists);
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::ExistsDeleted);
			QCOMPARE(std::get<1>(info), 30ull);
			QVERIFY(std::get<2>(info).isNull());
			QVERIFY(std::get<3>(info).isNull());
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 3u);
		}

		//store delete of noexisting
		{
			auto scope = store->startSync(TestLib::generateKey(45));
			store->storeDeleted(scope, 5ull, false, LocalStore::NoExists);
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::ExistsDeleted);
			QCOMPARE(std::get<1>(info), 5ull);
			QVERIFY(std::get<2>(info).isNull());
			QVERIFY(std::get<3>(info).isNull());
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 3u);
		}

		//mark unchanged correct version
		{
			auto scope = store->startSync(TestLib::generateKey(42));
			store->markUnchanged(scope, 30ull, true);
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 2u);
		}

		//mark unchanged wrong version
		{
			auto scope = store->startSync(TestLib::generateKey(44));
			store->markUnchanged(scope, 77ull, false);
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 2u);
		}

		//mark unchanged data
		{
			auto scope = store->startSync(TestLib::generateKey(44));
			store->markUnchanged(scope, 11ull, false);
			auto info = store->loadChangeInfo(scope);
			QCOMPARE(std::get<0>(info), LocalStore::Exists);
			QCOMPARE(std::get<1>(info), 11ull);
			QVERIFY(!std::get<2>(info).isNull());
			QVERIFY(!std::get<3>(info).isNull());
			store->commitSync(scope);
			QCOMPARE(store->changeCount(), 1u);
		}
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testChangeSignals()
{
	const auto key = TestLib::generateKey(77);
	auto data = TestLib::generateDataJson(77);

	QCoreApplication::processEvents();
	QThread::sleep(1);
	QCoreApplication::processEvents();

	LocalStore second(DefaultsPrivate::obtainDefaults(DefaultSetup));

	QSignalSpy store1Spy(store, &LocalStore::dataChanged);
	QSignalSpy store2Spy(&second, &LocalStore::dataChanged);

	try {
		store->reset(false);

		store->save(key, data);
		QCOMPARE(store->load(key), data);
		QCOMPARE(second.load(key), data);

		QCOMPARE(store1Spy.size(), 1);
		auto sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		QVERIFY(store2Spy.wait());
		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		data.insert(QStringLiteral("baum"), 42);
		second.save(key, data);
		QCOMPARE(second.load(key), data);
		QCOMPARE(store->load(key), data);

		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		QVERIFY(store1Spy.wait());
		QCOMPARE(store1Spy.size(), 1);
		sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		QVERIFY(store->remove(key));
		QVERIFY_EXCEPTION_THROWN(store->load(key), NoDataException);
		QVERIFY_EXCEPTION_THROWN(second.load(key), NoDataException);

		QCOMPARE(store1Spy.size(), 1);
		sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), true);

		QVERIFY(store2Spy.wait());
		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), true);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testAsync()
{
	try {
		store->reset(false);

		auto cnt = 10 * QThread::idealThreadCount();

		QList<QFuture<void>> futures;
		for(auto i = 0; i < cnt; i++) {
			futures.append(QtConcurrent::run([&](){
				LocalStore lStore(DefaultsPrivate::obtainDefaults(DefaultSetup));//thread without eventloop!
				auto key = TestLib::generateKey(66);
				auto data = TestLib::generateDataJson(66);

				//try to provoke conflicts
				lStore.save(key, data);
				lStore.load(key);
				lStore.load(key);
				lStore.save(key, data);
				lStore.load(key);
				if(lStore.count(TestLib::TypeName) != 1)
					throw Exception(DefaultSetup, QStringLiteral("Expected count is not 1"));
			}));
		}

		for(auto f : futures)
			f.waitForFinished();
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testPassiveSetup()
{
	const auto key = TestLib::generateKey(77);
	auto data = TestLib::generateDataJson(77);

	QCoreApplication::processEvents();
	QThread::sleep(1);
	QCoreApplication::processEvents();

	try {
		auto nName = QStringLiteral("setup2");
		Setup setup;
		TestLib::setup(setup);
		setup.setRemoteObjectHost(QStringLiteral("threaded:/qtdatasync/default/enginenode"));
		QVERIFY(setup.createPassive(nName, 5000));

		LocalStore second(DefaultsPrivate::obtainDefaults(nName));

		QSignalSpy store1Spy(store, &LocalStore::dataChanged);
		QSignalSpy store2Spy(&second, &LocalStore::dataChanged);

		store->reset(false);

		store->save(key, data);
		QCOMPARE(store->load(key), data);
		QCOMPARE(second.load(key), data);//only works because nothing is cached yet

		QCOMPARE(store1Spy.size(), 1);
		auto sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		QVERIFY(store2Spy.wait());
		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		data.insert(QStringLiteral("baum"), 42);
		second.save(key, data);
		QCOMPARE(second.load(key), data);
		QVERIFY(store->load(key) != data);//must be different, because change did not reach it yet (no atomicity for passive setups!)

		QVERIFY(store2Spy.wait());
		QCOMPARE(store2Spy.size(), 1);
		sig = store2Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		if(store1Spy.isEmpty())
			QVERIFY(store1Spy.wait());
		QCOMPARE(store->load(key), data);//now it's here
		QCOMPARE(store1Spy.size(), 1);
		sig = store1Spy.takeFirst();
		QCOMPARE(sig[0].value<ObjectKey>(), key);
		QCOMPARE(sig[1].toBool(), false);

		Setup::removeSetup(nName);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(TestLocalStore)

#include "tst_localstore.moc"
