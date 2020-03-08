#include <QtTest>
#include <QtDataSync/Engine>
#include <QtDataSync/Setup>
#include <QtDataSync/TableSyncController>
#include <QtDataSync/private/databasewatcher_p.h>
#include <QtDataSync/private/remoteconnector_p.h>
#include <QtDataSync/private/firebaseauthenticator_p.h>
#include "anonauth.h"
#include "testlib.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

#define VERIFY_SPY(sigSpy, errorSpy) QVERIFY2(sigSpy.wait(), qUtf8Printable(QStringLiteral("Error on table: ") + errorSpy.value(0).value(0).toString()))

class EngineTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSetup();

	void testDbSync();
	void testTableSync();

	void testSyncFlow();
	void testDynamicTables();

	void testDeleteAcc();

private:
	using SyncState = TableSyncController::SyncState;
	using EngineState = Engine::EngineState;
	using DbFlag = Engine::DbSyncFlag;
	using DbMode = Engine::DbSyncMode;
	using ptr = QScopedPointer<Engine, QScopedPointerDeleteLater>;

	class Query : public ExQuery {
	public:
		inline Query(QSqlDatabase db) :
			ExQuery{db, DatabaseWatcher::ErrorScope::Database, {}}
		{}
	};

	class ScopedDb : public QSqlDatabase
	{
		Q_DISABLE_COPY_MOVE(ScopedDb);
	public:
		inline ScopedDb(QSqlDatabase &&db) :
			QSqlDatabase{std::move(db)}
		{}
		inline ~ScopedDb() {
			const auto name = databaseName();
			close();
			static_cast<QSqlDatabase&>(*this) = QSqlDatabase{};
			QSqlDatabase::removeDatabase(name);
		}
	};

	static constexpr RemoteConnector::CancallationToken InvalidToken = std::numeric_limits<RemoteConnector::CancallationToken>::max();

	QTemporaryDir _dbDir;
	Engine *_engine = nullptr;
	RemoteConnector *_rmc = nullptr;

	QSqlDatabase createDb();
	void addTable(QSqlDatabase db, const QString &name, int fill = 5);
	int tableSize(QSqlDatabase db, const QString &name);
	bool testEntry(QSqlDatabase db, const QString &name, int key, bool exist = true);

	static inline EngineState getEState(QSignalSpy &spy) {
		return spy.takeFirst()[0].value<EngineState>();
	}
	static inline SyncState getSState(QSignalSpy &spy) {
		return spy.takeFirst()[0].value<SyncState>();
	}
};

void EngineTest::initTestCase()
{
	qRegisterMetaType<EngineState>("Engine::EngineState");
	qRegisterMetaType<Engine::ErrorType>("Engine::ErrorType");
	qRegisterMetaType<SyncState>("TableSyncController::SyncState");

	try {
		const auto settings = TestLib::createSettings(this);
		const auto nam = new QNetworkAccessManager{this};

		_engine = Setup<AnonAuth>()
					  .setFirebaseProjectId(QStringLiteral(FIREBASE_PROJECT_ID))
					  .setFirebaseApiKey(QStringLiteral(FIREBASE_API_KEY))
					  .setSettings(settings)
					  .setNetworkAccessManager(nam)
					  .createEngine(this);
		QVERIFY(_engine);

		__private::SetupPrivate::FirebaseConfig config {
			QStringLiteral(FIREBASE_PROJECT_ID),
			QStringLiteral(FIREBASE_API_KEY)
		};

		auto rmcSettings = TestLib::createSettings(this);
		rmcSettings->beginGroup(QStringLiteral("test"));
		_rmc = new RemoteConnector {
			config,
			rmcSettings,
			nam,
			std::nullopt,
			this
		};
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void EngineTest::cleanupTestCase()
{
	try {
		if (_engine) {
			if (_engine->isRunning()) {
				QSignalSpy stateSpy{_engine, &Engine::stateChanged};
				QVERIFY(stateSpy.isValid());
				_engine->deleteAccount();
				for (auto i = 0; i < 50; ++i) {
					if (stateSpy.wait(100)) {
						if (stateSpy.last()[0].value<EngineState>() == EngineState::Inactive) {
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

void EngineTest::testSetup()
{
	QVERIFY(qobject_cast<AnonAuth*>(_engine->authenticator()));
	QVERIFY(qobject_cast<PlainCloudTransformer*>(_engine->transformer()));
	QCOMPARE(_engine->state(), EngineState::Inactive);
	QCOMPARE(_engine->isRunning(), false);
	QCOMPARE(_engine->tables().size(), 0);
}

void EngineTest::testDbSync()
{
	static const QString testTable1 {QStringLiteral("testTable1")};
	static const QString testTable2 {QStringLiteral("testTable2")};

	try {
		QSignalSpy tableSpy{_engine, &Engine::tablesChanged};
		QVERIFY(tableSpy.isValid());
		QSignalSpy stateSpy{_engine, &Engine::stateChanged};
		QVERIFY(stateSpy.isValid());
		QSignalSpy errorSpy{_engine, &Engine::errorOccured};
		QVERIFY(errorSpy.isValid());

		QCOMPARE(_engine->isRunning(), false);

		// create and add database with no args -> does nothing
		ScopedDb db = createDb();
		QVERIFY(db.isValid());
		addTable(db, testTable1);
		addTable(db, testTable2);

		QCOMPARE(_engine->tables().size(), 0);
		_engine->syncDatabase(DbFlag::Default, db);
		QTRY_COMPARE(_engine->tables().size(), 0);

		// add all tables
		_engine->syncDatabase(DbFlag::SyncAllTables, db);
		QTRY_COMPARE(_engine->tables().size(), 2);
		QVERIFY(_engine->tables().contains(testTable1));
		QVERIFY(_engine->tables().contains(testTable2));
		QCOMPARE(tableSpy.size(), 2);
		tableSpy.clear();

		// remove table sync soft
		_engine->removeDatabaseSync(db, false);
		QTRY_COMPARE(_engine->tables().size(), 0);
		QCOMPARE(tableSpy.size(), 2);
		tableSpy.clear();

		// readd without auto does nothing
		_engine->syncDatabase(DbFlag::None, db);
		QTRY_COMPARE(_engine->tables().size(), 0);
		QCOMPARE(tableSpy.size(), 0);

		// readd but with auto does
		_engine->syncDatabase(DbFlag::ResyncTables, db);
		QTRY_COMPARE(_engine->tables().size(), 2);
		QVERIFY(_engine->tables().contains(testTable1));
		QVERIFY(_engine->tables().contains(testTable2));
		QCOMPARE(tableSpy.size(), 2);
		tableSpy.clear();

		// remove again, hard
		_engine->removeDatabaseSync(db, true);
		QTRY_COMPARE(_engine->tables().size(), 0);
		QCOMPARE(tableSpy.size(), 2);
		tableSpy.clear();

		// readd with auto does nothing
		_engine->syncDatabase(DbFlag::ResyncTables, db);
		QTRY_COMPARE(_engine->tables().size(), 0);
		QCOMPARE(tableSpy.size(), 0);

		// add all tables again
		_engine->syncDatabase(DbFlag::SyncAllTables, db);
		QTRY_COMPARE(_engine->tables().size(), 2);
		QVERIFY(_engine->tables().contains(testTable1));
		QVERIFY(_engine->tables().contains(testTable2));
		QCOMPARE(tableSpy.size(), 2);
		tableSpy.clear();

		// and unsync
		_engine->unsyncDatabase(db);
		QTRY_COMPARE(_engine->tables().size(), 0);
		QCOMPARE(tableSpy.size(), 2);
		tableSpy.clear();
		QCOMPARE(db.tables().size(), 2); // only table1&2

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void EngineTest::testTableSync()
{
	static const QString testTable {QStringLiteral("testTable")};

	try {
		QSignalSpy tableSpy{_engine, &Engine::tablesChanged};
		QVERIFY(tableSpy.isValid());
		QSignalSpy stateSpy{_engine, &Engine::stateChanged};
		QVERIFY(stateSpy.isValid());
		QSignalSpy errorSpy{_engine, &Engine::errorOccured};
		QVERIFY(errorSpy.isValid());

		QCOMPARE(_engine->isRunning(), false);

		// create table
		ScopedDb db = createDb();
		QVERIFY(db.isValid());
		addTable(db, testTable);

		// add table
		_engine->syncTable(testTable, false, db);
		QTRY_COMPARE(_engine->tables().size(), 1);
		QVERIFY(_engine->tables().contains(testTable));
		QCOMPARE(tableSpy.size(), 1);
		tableSpy.clear();

		auto tCtr = _engine->createController(testTable, this);
		QVERIFY(tCtr);
		QSignalSpy validSpy{tCtr, &TableSyncController::validChanged};
		QCOMPARE(tCtr->isValid(), true);
		QCOMPARE(tCtr->table(), testTable);
		QCOMPARE(tCtr->syncState(), SyncState::Disabled);
		QCOMPARE(tCtr->isLiveSyncEnabled(), false);
		QCOMPARE(tCtr->database().databaseName(), db.databaseName());

		// remove table (soft)
		_engine->removeTableSync(testTable);
		QTRY_COMPARE(_engine->tables().size(), 0);
		QCOMPARE(tableSpy.size(), 1);
		tableSpy.clear();

		QTRY_COMPARE(tCtr->isValid(), false);
		QCOMPARE(validSpy.size(), 1);
		QCOMPARE(validSpy[0][0], false);
		tCtr->deleteLater();
		tCtr = nullptr;

		// re-add table
		_engine->syncTable(testTable, true, db);
		QTRY_COMPARE(_engine->tables().size(), 1);
		QVERIFY(_engine->tables().contains(testTable));
		QCOMPARE(tableSpy.size(), 1);
		tableSpy.clear();

		tCtr = _engine->createController(testTable, this);
		QVERIFY(tCtr);
		QCOMPARE(tCtr->isValid(), true);
		QCOMPARE(tCtr->isLiveSyncEnabled(), true);

		// unsync table
		_engine->unsyncTable(testTable);
		QTRY_COMPARE(_engine->tables().size(), 0);
		QCOMPARE(tableSpy.size(), 1);
		tableSpy.clear();
		QTRY_COMPARE(tCtr->isValid(), false);
		tCtr->deleteLater();
		tCtr = nullptr;

		// remove db before deleting it
		_engine->removeDatabaseSync(db);

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void EngineTest::testSyncFlow()
{
	static const QString testTable {QStringLiteral("testTable")};

	try {
		QSignalSpy tableSpy{_engine, &Engine::tablesChanged};
		QVERIFY(tableSpy.isValid());
		QSignalSpy stateSpy{_engine, &Engine::stateChanged};
		QVERIFY(stateSpy.isValid());
		QSignalSpy errorSpy{_engine, &Engine::errorOccured};
		QVERIFY(errorSpy.isValid());

		QSignalSpy uploadSpy{_rmc, &RemoteConnector::uploadedData};
		QVERIFY(uploadSpy.isValid());
		QSignalSpy downloadSpy{_rmc, &RemoteConnector::downloadedData};
		QVERIFY(downloadSpy.isValid());
		QSignalSpy changesSpy{_rmc, &RemoteConnector::syncDone};
		QVERIFY(changesSpy.isValid());
		QSignalSpy rmcErrSpy{_rmc, &RemoteConnector::networkError};
		QVERIFY(rmcErrSpy.isValid());

		QCOMPARE(_engine->isRunning(), false);
		const auto auth = qobject_cast<AnonAuth*>(_engine->authenticator());
		QVERIFY(auth);

		// create and add table
		ScopedDb db = createDb();
		QVERIFY(db.isValid());
		addTable(db, testTable);
		_engine->syncTable(testTable, false, db);
		QTRY_COMPARE(_engine->tables().size(), 1);
		QVERIFY(_engine->tables().contains(testTable));
		QCOMPARE(tableSpy.size(), 1);
		tableSpy.clear();

		// create controller
		const auto controller = _engine->createController(testTable, this);
		QVERIFY(controller);
		QVERIFY(controller->isValid());
		QSignalSpy syncSpy{controller, &TableSyncController::syncStateChanged};
		QVERIFY(syncSpy.isValid());
		QSignalSpy liveSyncSpy{controller, &TableSyncController::liveSyncEnabledChanged};
		QVERIFY(liveSyncSpy.isValid());

		// start sync
		_engine->start();
		QTRY_COMPARE(_engine->state(), EngineState::TableSync);
		QCOMPARE(_engine->isRunning(), true);
		QCOMPARE(stateSpy.size(), 2);
		QCOMPARE(getEState(stateSpy), EngineState::SigningIn);
		QCOMPARE(getEState(stateSpy), EngineState::TableSync);

		// initialize rmc
		_rmc->setUser(auth->localId);
		_rmc->setIdToken(auth->idToken);

		// wait for controller to sync
		QTRY_COMPARE_WITH_TIMEOUT(controller->syncState(), SyncState::Synchronized, 10000);
		QCOMPARE(syncSpy.size(), 3);
		QCOMPARE(getSState(syncSpy), SyncState::Downloading);
		QCOMPARE(getSState(syncSpy), SyncState::Uploading);
		QCOMPARE(getSState(syncSpy), SyncState::Synchronized);
		QCOMPARE(tableSize(db, testTable), 5);

		// verify server data
		auto token = _rmc->getChanges(testTable, QDateTime{});
		QVERIFY(token != InvalidToken);
		VERIFY_SPY(changesSpy, rmcErrSpy);
		QCOMPARE(changesSpy.size(), 1);
		QCOMPARE(changesSpy[0][0], testTable);
		QCOMPARE(downloadSpy.size(), 1);
		QCOMPARE(downloadSpy[0][0], testTable);
		QCOMPARE(downloadSpy[0][1].value<QList<CloudData>>().size(), 5);
		changesSpy.clear();
		downloadSpy.clear();

		// trigger sync via ctr
		controller->triggerSync();
		QTRY_COMPARE(syncSpy.size(), 2);
		QCOMPARE(controller->syncState(), SyncState::Synchronized);
		QCOMPARE(getSState(syncSpy), SyncState::Downloading);
		QCOMPARE(getSState(syncSpy), SyncState::Synchronized);
		QCOMPARE(tableSize(db, testTable), 5);

		// upload data
		token = _rmc->uploadChange({
			testTable, QStringLiteral("_5"),
			QJsonObject{
				{QStringLiteral("Key"), 5},
				{QStringLiteral("Value"), 0.5}
			},
			QDateTime::currentDateTimeUtc(),
			std::nullopt
		});
		QVERIFY(token != InvalidToken);
		VERIFY_SPY(uploadSpy, rmcErrSpy);
		QCOMPARE(uploadSpy.size(), 1);
		uploadSpy.clear();

		// trigger sync via engine
		_engine->triggerSync();
		QTRY_COMPARE(syncSpy.size(), 2);
		QCOMPARE(controller->syncState(), SyncState::Synchronized);
		QCOMPARE(getSState(syncSpy), SyncState::Downloading);
		QCOMPARE(getSState(syncSpy), SyncState::Synchronized);
		QCOMPARE(tableSize(db, testTable), 6);
		QVERIFY(testEntry(db, testTable, 5));

		// again (downloads that 1 entry without effect)
		controller->triggerSync();
		QTRY_COMPARE(syncSpy.size(), 2);
		QCOMPARE(controller->syncState(), SyncState::Synchronized);
		QCOMPARE(getSState(syncSpy), SyncState::Downloading);
		QCOMPARE(getSState(syncSpy), SyncState::Synchronized);
		QCOMPARE(tableSize(db, testTable), 6);

		// switch to live sync via engine
		_engine->setLiveSyncEnabled(true);
		QCOMPARE(controller->isLiveSyncEnabled(), true);
		QCOMPARE(liveSyncSpy.size(), 1);
		QCOMPARE(liveSyncSpy.takeFirst()[0], true);
		// wait for live sync
		QTRY_COMPARE(controller->syncState(), SyncState::LiveSync);
		QCOMPARE(syncSpy.size(), 2);
		QCOMPARE(getSState(syncSpy), SyncState::Initializing);
		QCOMPARE(getSState(syncSpy), SyncState::LiveSync);

		// upload live change
		token = _rmc->uploadChange({
			testTable, QStringLiteral("_2"),
			std::nullopt,
			QDateTime::currentDateTimeUtc(),
			std::nullopt
		});
		QVERIFY(token != InvalidToken);
		VERIFY_SPY(uploadSpy, rmcErrSpy);
		QCOMPARE(uploadSpy.size(), 1);
		uploadSpy.clear();
		// wait
		QVERIFY(!syncSpy.wait(3000));
		QCOMPARE(controller->syncState(), SyncState::LiveSync);
		QCOMPARE(tableSize(db, testTable), 5);
		QVERIFY(testEntry(db, testTable, 5));
		QVERIFY(testEntry(db, testTable, 2, false));

		// switch back to passive sync
		controller->setLiveSyncEnabled(false);
		QCOMPARE(controller->isLiveSyncEnabled(), false);
		QCOMPARE(liveSyncSpy.size(), 1);
		QCOMPARE(liveSyncSpy.takeFirst()[0], false);
		// wait for passive sync
		QTRY_COMPARE(controller->syncState(), SyncState::Synchronized);
		QCOMPARE(syncSpy.size(), 1);
		QCOMPARE(getSState(syncSpy), SyncState::Synchronized);

		// stop sync and wait for stopped
		_engine->stop();
		QVERIFY(_engine->waitForStopped(5s));
		QCOMPARE(_engine->state(), EngineState::Inactive);
		QCOMPARE(_engine->isRunning(), false);
		QCOMPARE(stateSpy.size(), 2);
		QCOMPARE(getEState(stateSpy), EngineState::Stopping);
		QCOMPARE(getEState(stateSpy), EngineState::Inactive);

		// remove db before deleting it
		_engine->unsyncDatabase(db);

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void EngineTest::testDynamicTables()
{
	static const QString testTable {QStringLiteral("dynamicTable")};

	try {
		QSignalSpy tableSpy{_engine, &Engine::tablesChanged};
		QVERIFY(tableSpy.isValid());
		QSignalSpy stateSpy{_engine, &Engine::stateChanged};
		QVERIFY(stateSpy.isValid());
		QSignalSpy errorSpy{_engine, &Engine::errorOccured};
		QVERIFY(errorSpy.isValid());

		// start engine
		QCOMPARE(_engine->isRunning(), false);
		_engine->start();
		QTRY_COMPARE(_engine->state(), EngineState::TableSync);
		QCOMPARE(_engine->isRunning(), true);
		QCOMPARE(stateSpy.size(), 1);
		QCOMPARE(getEState(stateSpy), EngineState::TableSync);

		// create and add table
		ScopedDb db = createDb();
		QVERIFY(db.isValid());
		addTable(db, testTable);

		_engine->syncTable(testTable, true, db);
		QTRY_COMPARE(_engine->tables().size(), 1);
		QVERIFY(_engine->tables().contains(testTable));
		QCOMPARE(tableSpy.size(), 1);
		tableSpy.clear();

		// create controller and wait for ready
		const auto controller = _engine->createController(testTable, this);
		QVERIFY(controller);
		QVERIFY(controller->isValid());
		QSignalSpy syncSpy{controller, &TableSyncController::syncStateChanged};
		QVERIFY(syncSpy.isValid());

		QTRY_COMPARE(controller->syncState(), SyncState::LiveSync);
		QCOMPARE(syncSpy.size(), 2);
		QCOMPARE(getSState(syncSpy), SyncState::Initializing);
		QCOMPARE(getSState(syncSpy), SyncState::LiveSync);
		QCOMPARE(tableSize(db, testTable), 5);

		// trigger a full resync
		_engine->resyncTable(testTable, Engine::ResyncFlag::SyncAll | Engine::ResyncFlag::ClearAll);
		QTRY_COMPARE(syncSpy.size(), 2);
		QCOMPARE(controller->syncState(), SyncState::LiveSync);
		QCOMPARE(getSState(syncSpy), SyncState::Initializing);
		QCOMPARE(getSState(syncSpy), SyncState::LiveSync);
		QCOMPARE(tableSize(db, testTable), 0);

		// remove table
		_engine->removeTableSync(testTable);
		QTRY_COMPARE(_engine->tables().size(), 0);
		QCOMPARE(tableSpy.size(), 1);
		tableSpy.clear();

		// stop sync and wait for stopped
		_engine->stop();
		QVERIFY(_engine->waitForStopped(5s));
		QCOMPARE(_engine->state(), EngineState::Inactive);
		QCOMPARE(_engine->isRunning(), false);
		QCOMPARE(stateSpy.size(), 1);
		QCOMPARE(getEState(stateSpy), EngineState::Inactive);

		// remove db before deleting it
		_engine->unsyncDatabase(db);

		QVERIFY(errorSpy.isEmpty());
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void EngineTest::testDeleteAcc()
{
	try {
		QSignalSpy stateSpy{_engine, &Engine::stateChanged};
		QVERIFY(stateSpy.isValid());
		QSignalSpy errorSpy{_engine, &Engine::errorOccured};
		QVERIFY(errorSpy.isValid());

		QCOMPARE(_engine->isRunning(), false);
		const auto auth = qobject_cast<AnonAuth*>(_engine->authenticator());
		QVERIFY(auth);

		// start sync
		_engine->start();
		QTRY_COMPARE(_engine->state(), EngineState::TableSync);
		QCOMPARE(_engine->isRunning(), true);
		QCOMPARE(stateSpy.size(), 1);
		QCOMPARE(getEState(stateSpy), EngineState::TableSync);

		// delete account
		auth->block = true;
		_engine->deleteAccount();
		QTRY_COMPARE(_engine->state(), EngineState::SigningIn);
		QCOMPARE(stateSpy.size(), 2);
		QCOMPARE(getEState(stateSpy), EngineState::DeletingAcc);
		QCOMPARE(getEState(stateSpy), EngineState::SigningIn);

		// stop sync and wait for stopped
		_engine->stop();
		QVERIFY(_engine->waitForStopped(5s));
		QCOMPARE(_engine->state(), EngineState::Inactive);
		QCOMPARE(_engine->isRunning(), false);
		QCOMPARE(stateSpy.size(), 1);
		QCOMPARE(getEState(stateSpy), EngineState::Inactive);

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

QSqlDatabase EngineTest::createDb()
{
	QSqlDatabase outDb;
	[&](){
		const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
		auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), id);
		QVERIFY2(db.isValid(), qUtf8Printable(db.lastError().text()));
		db.setDatabaseName(_dbDir.filePath(id));
		QVERIFY2(db.open(), qUtf8Printable(db.lastError().text()));
		outDb = db;
	}();
	return outDb;
}

void EngineTest::addTable(QSqlDatabase db, const QString &name, int fill)
{
	Query createQuery{db};
	createQuery.exec(QStringLiteral("CREATE TABLE %1 ("
									"	Key		INTEGER NOT NULL PRIMARY KEY, "
									"	Value	REAL NOT NULL "
									");")
						 .arg(db.driver()->escapeIdentifier(name, QSqlDriver::TableName)));

	for (auto i = 0; i < fill; ++i) {
		Query fillQuery{db};
		fillQuery.prepare(QStringLiteral("INSERT INTO %1 "
										 "(Key, Value) "
										 "VALUES(?, ?);")
							  .arg(db.driver()->escapeIdentifier(name, QSqlDriver::TableName)));
		fillQuery.addBindValue(i);
		fillQuery.addBindValue(i * 0.1);
		fillQuery.exec();
	}
}

int EngineTest::tableSize(QSqlDatabase db, const QString &name)
{
	Query sizeQuery{db};
	sizeQuery.prepare(QStringLiteral("SELECT COUNT(*)"
									 "FROM %1;")
						  .arg(db.driver()->escapeIdentifier(name, QSqlDriver::TableName)));
	sizeQuery.exec();
	if (!sizeQuery.first()) {
		throw SqlException{
			SqlException::ErrorScope::Table,
			QStringLiteral("COUNT"),
			sizeQuery.lastError()
		};
	}

	return sizeQuery.value(0).toInt();
}

bool EngineTest::testEntry(QSqlDatabase db, const QString &name, int key, bool exist)
{
	Query testQuery{db};
	testQuery.prepare(QStringLiteral("SELECT Value "
									 "FROM %1 "
									 "WHERE Key == ?;")
						  .arg(db.driver()->escapeIdentifier(name, QSqlDriver::TableName)));
	testQuery.addBindValue(key);
	testQuery.exec();
	auto ok = false;
	[&](){
		QCOMPARE(testQuery.first(), exist);
		if (exist)
			QCOMPARE(testQuery.value(0).toReal(), key * 0.1);
		ok = true;
	}();
	return ok;
}

QTEST_MAIN(EngineTest)

#include "tst_engine.moc"
