#include <QtTest>
#include <QtDataSync/AsyncWatcher>
#include <QtDataSync/Setup>
#include <QtDataSync/Engine>
#include <QtDataSync/TableSyncController>
#include <QtConcurrentRun>
#include <QRemoteObjectNode>
#include <QtDataSync/private/databasewatcher_p.h>
#include "testlib.h"
#include "anonauth.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

class AsyncWatcherTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSingleThreadSync();
	void testMultiThreadSync();
	void testProcessSync();

	void addSyncTable(const QString id,
					  const QString &table,
					  void *promise);
	void removeSyncTable(const QString &table,
						 void *promise);

private:
	using ptr = QScopedPointer<AsyncWatcher, QScopedPointerDeleteLater>;
	using SyncState = TableSyncController::SyncState;

	struct TestInfo {
		QString id;
		TableSyncController *controller = nullptr;
	};

	class Query : public ExQuery {
	public:
		inline Query(QSqlDatabase db) :
			ExQuery{db, DatabaseWatcher::ErrorScope::Database, {}}
		{}
	};

	QTemporaryDir _tDir;
	Engine *_engine;

	std::optional<TestInfo> prepareTest();
	void finalizeTest(const QString &id);
	void runTest(const TestInfo &info,
				 AsyncWatcher *watcher);
	void add(QSqlDatabase db, const QString &id, int key);

	static inline SyncState getState(QSignalSpy &spy) {
		return spy.takeFirst()[0].value<SyncState>();
	}
};

void AsyncWatcherTest::initTestCase()
{
	qRegisterMetaType<SyncState>("TableSyncController::SyncState");

	try {
		_engine = Setup<AnonAuth>()
					  .setSettings(TestLib::createSettings(this))
					  .setFirebaseProjectId(QStringLiteral(FIREBASE_PROJECT_ID))
					  .setFirebaseApiKey(QStringLiteral(FIREBASE_API_KEY))
					  .setAsyncUrl(QStringLiteral("local:qtdatasync_tst_asyncwatcher"))
					  .createEngine(this);
		QVERIFY(_engine);

		_engine->start();
		QTRY_COMPARE(_engine->state(), Engine::EngineState::TableSync);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void AsyncWatcherTest::cleanupTestCase()
{
	try {
		if (_engine) {
			if (_engine->isRunning()) {
				QSignalSpy stateSpy{_engine, &Engine::stateChanged};
				QVERIFY(stateSpy.isValid());
				_engine->deleteAccount();
				for (auto i = 0; i < 50; ++i) {
					if (stateSpy.wait(100)) {
						if (stateSpy.last()[0].value<Engine::EngineState>() == Engine::EngineState::Inactive) {
							qDebug() << "deleted account";
							return;
						}
					}
				}
				qDebug() << "failed to delete account";

				_engine->stop();
				_engine->waitForStopped(10s);
			} else
				qDebug() << "engine inactive - not deleting account";
			_engine->deleteLater();
		} else
			qDebug() << "engine inactive - not deleting account";

	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void AsyncWatcherTest::testSingleThreadSync()
{
	const auto data = prepareTest();
	QVERIFY(data);
	ptr watcher{new AsyncWatcher{_engine, this}};
	runTest(*data, watcher.data());
	finalizeTest(data->id);
}

void AsyncWatcherTest::testMultiThreadSync()
{
	const auto data = prepareTest();
	QVERIFY(data);
	const auto future = QtConcurrent::run([&](){
		ptr watcher{new AsyncWatcher{_engine}};
		runTest(*data, watcher.data());
	});
	QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 15000);
	finalizeTest(data->id);
}

void AsyncWatcherTest::testProcessSync()
{
	const auto data = prepareTest();
	QVERIFY(data);
	ptr watcher{new AsyncWatcher{QStringLiteral("local:qtdatasync_tst_asyncwatcher"), this}};
	runTest(*data, watcher.data());
	finalizeTest(data->id);
}

void AsyncWatcherTest::addSyncTable(const QString id, const QString &table, void *promise)
{
	const auto fi = static_cast<QFutureInterface<TableSyncController*>*>(promise);
	fi->reportStarted();
	auto guard = qScopeGuard([fi](){
		TableSyncController *controller = nullptr;
		fi->reportFinished(&controller);
	});
	try {
		auto db = QSqlDatabase::database(id);
		QVERIFY(db.isValid());
		Query createQuery{db};
		createQuery.exec(QStringLiteral("CREATE TABLE \"%1\" ("
										"	Key		INT NOT NULL PRIMARY KEY,"
										"	Value	REAL NOT NULL"
										");")
							 .arg(table));

		_engine->syncTable(table, false, id);
		const auto controller = _engine->createController(table, this);
		QTRY_COMPARE(controller->syncState(), SyncState::Synchronized);
		fi->reportFinished(&controller);
	} catch (QException &e) {
		fi->reportException(e);
		fi->reportFinished();
	}
	guard.dismiss();
}

void AsyncWatcherTest::removeSyncTable(const QString &table, void *promise)
{
	const auto fi = static_cast<QFutureInterface<bool>*>(promise);
	fi->reportStarted();
	auto guard = qScopeGuard([fi](){
		const auto ok = false;
		fi->reportFinished(&ok);
	});
	try {
		_engine->removeTableSync(table);
		const auto ok = true;
		fi->reportFinished(&ok);
	} catch (QException &e) {
		fi->reportException(e);
		fi->reportFinished();
	}
	guard.dismiss();
}

std::optional<AsyncWatcherTest::TestInfo> AsyncWatcherTest::prepareTest()
{
	std::optional<TestInfo> res;
	[&](){
		try {
			auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
			auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), id);
			QVERIFY2(db.isValid(), qUtf8Printable(db.lastError().text()));
			db.setDatabaseName(_tDir.filePath(id));
			QVERIFY2(db.open(), qUtf8Printable(db.lastError().text()));

			Query createQuery{db};
			createQuery.exec(QStringLiteral("CREATE TABLE \"%1\" ("
											"	Key		INT NOT NULL PRIMARY KEY,"
											"	Value	REAL NOT NULL"
											");")
							 .arg(id));

			_engine->syncTable(id, false, db);
			const auto controller = _engine->createController(id, this);
			QTRY_COMPARE(controller->syncState(), SyncState::Synchronized);
			res = {id, controller};
		} catch(std::exception &e) {
			QFAIL(e.what());
		}
	}();
	return res;
}

void AsyncWatcherTest::finalizeTest(const QString &id)
{
	try {
		_engine->removeDatabaseSync(id);

		{
			auto db = QSqlDatabase::database(id);
			if (!db.isValid())
				return;
			db.close();
		}
		QSqlDatabase::removeDatabase(id);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void AsyncWatcherTest::runTest(const TestInfo &info, AsyncWatcher *watcher)
{
	const auto &id = info.id;
	const auto id2 = id + id;
	const auto &controller = info.controller;

	QSignalSpy initSpy{watcher, &AsyncWatcher::initialized};
	QVERIFY(initSpy.isValid());
	QSignalSpy stateSpy{controller, &TableSyncController::syncStateChanged};
	QVERIFY(stateSpy.isValid());

	QVERIFY(initSpy.wait());
	try {
		auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), id2);
		QVERIFY2(db.isValid(), qUtf8Printable(db.lastError().text()));
		db.setDatabaseName(_tDir.filePath(id));
		QVERIFY2(db.open(), qUtf8Printable(db.lastError().text()));
		QVERIFY(db.tables().contains(id));

		// add data without adding -> no event
		add(db, id, 0);
		QVERIFY(!stateSpy.wait(3000));

		// add db named
		watcher->addDatabase(db, id);
		add(db, id, 1);
		QTRY_COMPARE(stateSpy.size(), 2);
		QCOMPARE(getState(stateSpy), SyncState::Uploading);
		QCOMPARE(getState(stateSpy), SyncState::Synchronized);

		{
			// add table
			QFutureInterface<TableSyncController*> addFi;
			QFuture<TableSyncController*> addFuture{&addFi};
			QVERIFY(QMetaObject::invokeMethod(this, "addSyncTable", Qt::QueuedConnection,
											  Q_ARG(QString, id),
											  Q_ARG(QString, id2),
											  Q_ARG(void*, &addFi)));
			QTRY_VERIFY(addFuture.isFinished());
			const auto controller2 = addFuture.result();
			QVERIFY(controller2);
			QSignalSpy stateSpy2{controller2, &TableSyncController::syncStateChanged};
			QVERIFY(stateSpy2.isValid());

			// add data to new table
			add(db, id2, 0);
			QTRY_COMPARE(stateSpy2.size(), 2);
			QCOMPARE(getState(stateSpy2), SyncState::Uploading);
			QCOMPARE(getState(stateSpy2), SyncState::Synchronized);

			// remove table
			QFutureInterface<bool> rmFi;
			QFuture<bool> rmFuture{&rmFi};
			QVERIFY(QMetaObject::invokeMethod(this, "removeSyncTable", Qt::QueuedConnection,
											  Q_ARG(QString, id2),
											  Q_ARG(void*, &rmFi)));
			QTRY_VERIFY(rmFuture.isFinished());
			QVERIFY(rmFuture.result());

			// add data -> no event
			add(db, id2, 1);
			QVERIFY(!stateSpy2.wait(3000));
		}

		// remove -> nothing again
		watcher->removeDatabase(db);
		add(db, id, 2);
		QVERIFY2(!stateSpy.wait(3000), qUtf8Printable(stateSpy.value(0).value(0).toString()));

		// add unnamed
		watcher->addDatabase(db);
		add(db, id, 3);
		QTRY_COMPARE(stateSpy.size(), 2);
		QCOMPARE(getState(stateSpy), SyncState::Uploading);
		QCOMPARE(getState(stateSpy), SyncState::Synchronized);

		// remove -> nothing again
		watcher->removeDatabase(db);
		add(db, id, 4);
		QVERIFY(!stateSpy.wait(3000));

		db.close();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}

	QSqlDatabase::removeDatabase(id + id);
}

void AsyncWatcherTest::add(QSqlDatabase db, const QString &id, int key)
{
	Query addQuery{db};
	addQuery.prepare(QStringLiteral("INSERT INTO \"%1\" "
									"(Key, Value) "
									"VALUES(?, ?);")
						 .arg(id));
	addQuery.addBindValue(key);
	addQuery.addBindValue(key * 0.1);
	addQuery.exec();
}

QTEST_MAIN(AsyncWatcherTest)

#include "tst_asyncwatcher.moc"
