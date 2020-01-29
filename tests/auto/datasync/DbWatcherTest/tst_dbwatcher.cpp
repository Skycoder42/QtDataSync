#include <QtTest>
#include <QtDataSync>

#include <QtDataSync/private/databasewatcher_p.h>
using namespace QtDataSync;

namespace {

const QDateTime past = QDateTime::currentDateTimeUtc().addDays(-1);
const QDateTime future = QDateTime::currentDateTimeUtc().addDays(1);

}

#define VERIFY_CHANGED_STATE_BASE(key, before, after, state, table) do { \
	Query testChangedQuery{_watcher}; \
	testChangedQuery.prepare(QStringLiteral("SELECT tstamp, changed " \
											"FROM %1 " \
											"WHERE pkey == ?;") \
									.arg(DatabaseWatcher::TablePrefix + table)); \
	testChangedQuery.addBindValue(key); \
	testChangedQuery.exec(); \
	QVERIFY(testChangedQuery.next()); \
	/*qDebug() << testChangedQuery.value(0) << before << after;*/ \
	QVERIFY(testChangedQuery.value(0).toDateTime() >= before); \
	QVERIFY(testChangedQuery.value(0).toDateTime() <= after); \
	/*qDebug() << testChangedQuery.value(1) << state;*/ \
	QCOMPARE(testChangedQuery.value(1).toInt(), static_cast<int>(state)); \
	QVERIFY(!testChangedQuery.next()); \
} while(false)

#define VERIFY_CHANGED_STATE(key, before, after, state) VERIFY_CHANGED_STATE_BASE(key, before, after, state, TestTable)
#define VERIFY_CHANGED(key, before, after) VERIFY_CHANGED_STATE(key, before, after, DatabaseWatcher::ChangeState::Changed)
#define VERIFY_UNCHANGED0(key) VERIFY_CHANGED_STATE(key, past, future, DatabaseWatcher::ChangeState::Unchanged)
#define VERIFY_UNCHANGED1(key, when) VERIFY_CHANGED_STATE(key, when, when, DatabaseWatcher::ChangeState::Unchanged)
#define VERIFY_UNCHANGED2(key, before, after) VERIFY_CHANGED_STATE(key, before, after, DatabaseWatcher::ChangeState::Unchanged)

#define VERIFY_CHANGED_STATE_KEY(key, before, after, state) VERIFY_CHANGED_STATE_BASE((key).id, before, after, state, (key).typeName)

class DbWatcherTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testAddTable();
	void testRemoveTable();
	void testReaddTable();

	void testStoreData_data();
	void testStoreData();
	void testLoadData();
	void testMarkCorrupted();

	void testResync();

	void testDropAndReactivate();
	void testUnsyncTable();
	void testDbActions();

	void testBinaryKey();

	void testSyncCustomFields();
	void testStoreCustomFields_data();
	void testStoreCustomFields();
	void testUnsyncCustomFields();

private:
	static const QString TestTable;

	class Query : public ExQuery {
	public:
		inline Query(QSqlDatabase db) :
			ExQuery{db, DatabaseWatcher::ErrorScope::Database, {}}
		{}
		inline Query(DatabaseWatcher *watcher) :
			Query{watcher->database()}
		{}
	};

	QTemporaryDir _tDir;
	DatabaseWatcher *_watcher;

	void fillDb();
	void markChanged(DatabaseWatcher::ChangeState state, const QString &key = {});
	void markChanged(DatabaseWatcher::ChangeState state, const QtDataSync::ObjectKey &key);
};

const QString DbWatcherTest::TestTable = QStringLiteral("TestData");

void DbWatcherTest::initTestCase()
{
	qRegisterMetaType<DatabaseWatcher::ErrorScope>("ErrorScope");

	_watcher = new DatabaseWatcher {
		QSqlDatabase::addDatabase(QStringLiteral("QSQLITE")),
		TransactionMode::Default,
		this
	};
	auto db = _watcher->database();
	QVERIFY(db.isValid());
	db.setDatabaseName(_tDir.filePath(QStringLiteral("test.db")));
	QVERIFY2(db.open(), qUtf8Printable(db.lastError().text()));
	fillDb();
}

void DbWatcherTest::cleanupTestCase()
{
	_watcher->database().close();
	delete _watcher;
	QSqlDatabase::removeDatabase(QLatin1String(QSqlDatabase::defaultConnection));
	//_tDir.setAutoRemove(false);
}

void DbWatcherTest::testAddTable()
{
	try {
		QCOMPARE(_watcher->hasTables(), false);
		QVERIFY(!_watcher->tables().contains(TestTable));

		QSignalSpy addSpy{_watcher, &DatabaseWatcher::tableAdded};
		QVERIFY(addSpy.isValid());
		QSignalSpy syncSpy{_watcher, &DatabaseWatcher::triggerSync};
		QVERIFY(syncSpy.isValid());

		auto before = QDateTime::currentDateTimeUtc();
		_watcher->addTable(TableConfig{TestTable, _watcher->database()});
		auto after = QDateTime::currentDateTimeUtc();

		QCOMPARE(_watcher->hasTables(), true);
		QVERIFY(_watcher->tables().contains(TestTable));

		QCOMPARE(addSpy.size(), 1);
		QCOMPARE(addSpy[0][0].toString(), TestTable);

		QCOMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();

		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::MetaTable));
		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::FieldTable));
		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::ReferenceTable));
		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::TablePrefix + TestTable));

		// verify meta state
		Query metaQuery{_watcher};
		metaQuery.prepare(QStringLiteral("SELECT version, pkeyName, pkeyType, state, lastSync "
										 "FROM __qtdatasync_meta_data "
										 "WHERE tableName == ?;"));
		metaQuery.addBindValue(TestTable);
		metaQuery.exec();
		QVERIFY(metaQuery.first());
		QVERIFY(metaQuery.value(0).isNull());
		QCOMPARE(metaQuery.value(1).toString(), QStringLiteral("Key"));
		QCOMPARE(metaQuery.value(2).toInt(), static_cast<int>(QMetaType::Int));
		QCOMPARE(metaQuery.value(3).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
		QVERIFY(metaQuery.value(4).isNull());

		// verify no fields
		Query fieldsQuery{_watcher};
		fieldsQuery.prepare(QStringLiteral("SELECT syncField "
										   "FROM __qtdatasync_sync_fields "
										   "WHERE tableName == ?;"));
		fieldsQuery.addBindValue(TestTable);
		fieldsQuery.exec();
		QVERIFY(!fieldsQuery.first());

		// verify no references
		Query refsQuery{_watcher};
		refsQuery.prepare(QStringLiteral("SELECT fKeyTable, fKeyField "
										 "FROM __qtdatasync_sync_references "
										 "WHERE tableName == ?;"));
		refsQuery.addBindValue(TestTable);
		refsQuery.exec();
		QVERIFY(!refsQuery.first());

		// verify sync state
		Query syncQuery{_watcher};
		syncQuery.exec(QStringLiteral("SELECT pkey, tstamp, changed "
									  "FROM __qtdatasync_sync_data_TestData "
									  "ORDER BY pkey ASC;"));
		for (auto i = 0; i < 10; ++i) {
			QVERIFY(syncQuery.next());
			QCOMPARE(syncQuery.value(0).toInt(), i);
			QVERIFY(syncQuery.value(1).toDateTime() >= before);
			QVERIFY(syncQuery.value(1).toDateTime() <= after);
			QCOMPARE(syncQuery.value(2).toInt(), static_cast<int>(DatabaseWatcher::ChangeState::Changed));
		}
		QVERIFY(!syncQuery.next());

		// verify insert trigger
		Query insertQuery{_watcher};
		insertQuery.prepare(QStringLiteral("INSERT INTO TestData "
										   "(Key, Value) "
										   "VALUES(?, ?);"));
		insertQuery.addBindValue(11);
		insertQuery.addBindValue(QStringLiteral("data_11"));
		before = QDateTime::currentDateTimeUtc();
		insertQuery.exec();
		after = QDateTime::currentDateTimeUtc();
		VERIFY_CHANGED(11, before, after);

		QTRY_COMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));

		// clear changed flag
		markChanged(DatabaseWatcher::ChangeState::Unchanged);
		VERIFY_UNCHANGED0(4);

		// verify update trigger
		Query updateQuery{_watcher};
		updateQuery.prepare(QStringLiteral("UPDATE TestData "
										   "SET Value = ? "
										   "WHERE Key == ?;"));
		updateQuery.addBindValue(QStringLiteral("baum42"));
		updateQuery.addBindValue(4);
		before = QDateTime::currentDateTimeUtc();
		updateQuery.exec();
		after = QDateTime::currentDateTimeUtc();
		VERIFY_CHANGED(4, before, after);

		QTRY_COMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));

		// clear changed flag
		markChanged(DatabaseWatcher::ChangeState::Unchanged);
		VERIFY_UNCHANGED0(4);

		// verify rename trigger
		Query renameQuery{_watcher};
		renameQuery.prepare(QStringLiteral("UPDATE TestData "
										   "SET Key = ? "
										   "WHERE Key == ?;"));
		renameQuery.addBindValue(12);
		renameQuery.addBindValue(4);
		before = QDateTime::currentDateTimeUtc();
		renameQuery.exec();
		after = QDateTime::currentDateTimeUtc();
		VERIFY_CHANGED(4, before, after);
		VERIFY_CHANGED(12, before, after);

		QTRY_COMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));

		// clear changed flag
		markChanged(DatabaseWatcher::ChangeState::Unchanged);
		VERIFY_UNCHANGED0(12);

		// verify delete trigger
		Query deleteQuery{_watcher};
		deleteQuery.prepare(QStringLiteral("DELETE FROM TestData "
										   "WHERE Key == ?;"));
		deleteQuery.addBindValue(12);
		before = QDateTime::currentDateTimeUtc();
		deleteQuery.exec();
		after = QDateTime::currentDateTimeUtc();
		VERIFY_CHANGED(12, before, after);

		QTRY_COMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testRemoveTable()
{
	try {
		QCOMPARE(_watcher->hasTables(), true);
		QVERIFY(_watcher->tables().contains(TestTable));

		QSignalSpy removeSpy{_watcher, &DatabaseWatcher::tableRemoved};
		QVERIFY(removeSpy.isValid());
		QSignalSpy syncSpy{_watcher, &DatabaseWatcher::triggerSync};
		QVERIFY(syncSpy.isValid());

		_watcher->removeTable(TestTable);

		QCOMPARE(_watcher->hasTables(), false);
		QVERIFY(!_watcher->tables().contains(TestTable));

		QCOMPARE(removeSpy.size(), 1);
		QCOMPARE(removeSpy[0][0].toString(), TestTable);

		// verify meta state
		Query metaQuery{_watcher};
		metaQuery.prepare(QStringLiteral("SELECT version, pkeyName, pkeyType, state, lastSync "
										 "FROM __qtdatasync_meta_data "
										 "WHERE tableName == ?;"));
		metaQuery.addBindValue(TestTable);
		metaQuery.exec();
		QVERIFY(metaQuery.first());
		QVERIFY(metaQuery.value(0).isNull());
		QCOMPARE(metaQuery.value(1).toString(), QStringLiteral("Key"));
		QCOMPARE(metaQuery.value(2).toInt(), static_cast<int>(QMetaType::Int));
		QCOMPARE(metaQuery.value(3).toInt(), static_cast<int>(DatabaseWatcher::TableState::Inactive));
		QVERIFY(metaQuery.value(4).isNull());

		// verify insert trigger
		Query insertQuery{_watcher};
		insertQuery.prepare(QStringLiteral("INSERT INTO TestData "
										   "(Key, Value) "
										   "VALUES(?, ?);"));
		insertQuery.addBindValue(4);
		insertQuery.addBindValue(QStringLiteral("data_4"));
		auto before = QDateTime::currentDateTimeUtc();
		insertQuery.exec();
		auto after = QDateTime::currentDateTimeUtc();
		VERIFY_CHANGED(4, before, after);

		QVERIFY(!syncSpy.wait(1000));
		QCOMPARE(syncSpy.size(), 0);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testReaddTable()
{
	try {
		QCOMPARE(_watcher->hasTables(), false);
		QVERIFY(!_watcher->tables().contains(TestTable));

		QSignalSpy addSpy{_watcher, &DatabaseWatcher::tableAdded};
		QVERIFY(addSpy.isValid());
		QSignalSpy syncSpy{_watcher, &DatabaseWatcher::triggerSync};
		QVERIFY(syncSpy.isValid());

		auto before = QDateTime::currentDateTimeUtc();
		_watcher->addTable(TableConfig{TestTable, _watcher->database()});
		auto after = QDateTime::currentDateTimeUtc();

		QCOMPARE(_watcher->hasTables(), true);
		QVERIFY(_watcher->tables().contains(TestTable));

		QCOMPARE(addSpy.size(), 1);
		QCOMPARE(addSpy[0][0].toString(), TestTable);

		QCOMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();

		// verify meta state
		Query metaQuery{_watcher};
		metaQuery.prepare(QStringLiteral("SELECT version, pkeyName, pkeyType, state, lastSync "
										 "FROM __qtdatasync_meta_data "
										 "WHERE tableName == ?;"));
		metaQuery.addBindValue(TestTable);
		metaQuery.exec();
		QVERIFY(metaQuery.first());
		QVERIFY(metaQuery.value(0).isNull());
		QCOMPARE(metaQuery.value(1).toString(), QStringLiteral("Key"));
		QCOMPARE(metaQuery.value(2).toInt(), static_cast<int>(QMetaType::Int));
		QCOMPARE(metaQuery.value(3).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
		QVERIFY(metaQuery.value(4).isNull());

		// verify insert trigger
		Query insertQuery{_watcher};
		insertQuery.prepare(QStringLiteral("INSERT INTO TestData "
										   "(Key, Value) "
										   "VALUES(?, ?);"));
		insertQuery.addBindValue(12);
		insertQuery.addBindValue(QStringLiteral("data_12"));
		before = QDateTime::currentDateTimeUtc();
		insertQuery.exec();
		after = QDateTime::currentDateTimeUtc();
		VERIFY_CHANGED(12, before, after);

		QTRY_COMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testStoreData_data()
{
	QTest::addColumn<QDateTime>("lastSync");
	QTest::addColumn<LocalData>("data");
	QTest::addColumn<QDateTime>("modified");
	QTest::addColumn<DatabaseWatcher::ChangeState>("changed");
	QTest::addColumn<bool>("isStored");

	auto lastSync = QDateTime::currentDateTimeUtc().addSecs(-5);
	QTest::newRow("storeNew") << QDateTime{}
							  << LocalData {
									 TestTable, QString::number(20),
									 QVariantHash{
										 {QStringLiteral("Key"), 20},
										 {QStringLiteral("Value"), 20}
									 },
									 QDateTime::currentDateTimeUtc().addSecs(-10),
									 std::nullopt,
									 lastSync
								 }
							  << QDateTime::currentDateTimeUtc().addSecs(-10)
							  << DatabaseWatcher::ChangeState::Unchanged
							  << true;

	lastSync = lastSync.addSecs(1);
	QTest::newRow("storeUpdated") << lastSync.addSecs(-1)
								  << LocalData {
										 TestTable, QString::number(20),
										 QVariantHash{
											 {QStringLiteral("Key"), 20},
											 {QStringLiteral("Value"), 42}
										 },
										 QDateTime::currentDateTimeUtc().addSecs(-5),
										 std::nullopt,
										 lastSync
									 }
								  << QDateTime::currentDateTimeUtc().addSecs(-5)
								  << DatabaseWatcher::ChangeState::Unchanged
								  << true;

	lastSync = lastSync.addSecs(1);
	QTest::newRow("olderUpdated") << lastSync.addSecs(-1)
								  << LocalData {
										 TestTable, QString::number(20),
										 QVariantHash{
											 {QStringLiteral("Key"), 20},
											 {QStringLiteral("Value"), 20}
										 },
										 QDateTime::currentDateTimeUtc().addSecs(-10),
										 std::nullopt,
										 lastSync
									 }
								  << QDateTime::currentDateTimeUtc().addSecs(-5)
								  << DatabaseWatcher::ChangeState::Changed
								  << false;

	lastSync = lastSync.addSecs(1);
	QTest::newRow("storeDelete") << lastSync.addSecs(-1)
								 << LocalData {
										TestTable, QString::number(20),
										std::nullopt,
										QDateTime::currentDateTimeUtc().addSecs(-5),
										std::nullopt,
										lastSync
									}
								 << QDateTime::currentDateTimeUtc().addSecs(-5)
								 << DatabaseWatcher::ChangeState::Unchanged
								 << true;
}

void DbWatcherTest::testStoreData()
{
	QFETCH(QDateTime, lastSync);
	QFETCH(LocalData, data);
	QFETCH(QDateTime, modified);
	QFETCH(DatabaseWatcher::ChangeState, changed);
	QFETCH(bool, isStored);

	try {
		QSignalSpy errorSpy{_watcher, &DatabaseWatcher::databaseError};
		QVERIFY(errorSpy.isValid());

		auto lSync = _watcher->lastSync(TestTable);
		QVERIFY(errorSpy.isEmpty());
		QVERIFY(lSync);
		QCOMPARE(*lSync, lastSync);

		QVariantList oldData;
		if (!isStored) {
			Query getOldData{_watcher};
			getOldData.prepare(QStringLiteral("SELECT Key, Value "
											  "FROM TestData "
											  "WHERE Key == ?;"));
			getOldData.addBindValue(data.key().id);
			getOldData.exec();
			if (getOldData.next()) {
				oldData.append(getOldData.value(0));
				oldData.append(getOldData.value(1));
				QVERIFY(!getOldData.next());
			}
		}

		// store new data
		_watcher->storeData(data);
		QVERIFY(errorSpy.isEmpty());

		VERIFY_CHANGED_STATE(data.key().id, modified, modified, changed);
		Query checkDataQuery{_watcher};
		checkDataQuery.prepare(QStringLiteral("SELECT Key, Value "
											  "FROM TestData "
											  "WHERE Key == ?;"));
		checkDataQuery.addBindValue(data.key().id);
		checkDataQuery.exec();
		if (isStored) {
			if (data.data()) {
				QVERIFY(checkDataQuery.next());
				QCOMPARE(checkDataQuery.value(0), data.data()->value(QStringLiteral("Key")));
				QCOMPARE(checkDataQuery.value(1), data.data()->value(QStringLiteral("Value")));
				QVERIFY(!checkDataQuery.next());
			} else
				QVERIFY(!checkDataQuery.next());
		} else {
			if (oldData.isEmpty())
				QVERIFY(!checkDataQuery.next());
			else {
				QVERIFY(checkDataQuery.next());
				QCOMPARE(checkDataQuery.value(0), oldData.at(0));
				QCOMPARE(checkDataQuery.value(1), oldData.at(1));
				QVERIFY(!checkDataQuery.next());
			}
		}

		lSync = _watcher->lastSync(TestTable);
		QVERIFY(errorSpy.isEmpty());
		QVERIFY(lSync);
		QCOMPARE(*lSync, data.uploaded());

		// mark changed again
		markChanged(DatabaseWatcher::ChangeState::Changed, data.key());
		VERIFY_CHANGED(data.key().id, modified, modified);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testLoadData()
{
	try {
		QSignalSpy errorSpy{_watcher, &DatabaseWatcher::databaseError};
		QVERIFY(errorSpy.isValid());

		// mark all unchanged
		markChanged(DatabaseWatcher::ChangeState::Unchanged);

		QVERIFY(!_watcher->loadData(TestTable));
		QVERIFY(errorSpy.isEmpty());

		// mark changed
		const auto baseTime = QDateTime::currentDateTimeUtc();
		for (auto i = 0; i < 5; ++i) {
			Query markChangedQuery{_watcher};
			markChangedQuery.prepare(QStringLiteral("UPDATE __qtdatasync_sync_data_TestData "
													"SET changed = ?, tstamp = ? "
													"WHERE pkey == ?;"));
			markChangedQuery.addBindValue(static_cast<int>(DatabaseWatcher::ChangeState::Changed));
			markChangedQuery.addBindValue(baseTime.addSecs(-60 + (10 * i)));
			markChangedQuery.addBindValue(i);
			markChangedQuery.exec();
			QCOMPARE(markChangedQuery.numRowsAffected(), 1);
		}

		// test changed
		for (auto i = 0; i < 5; ++i) {
			const auto mTime = baseTime.addSecs(-60 + (10 * i));
			QVariantHash data;
			data[QStringLiteral("Key")] = i;
			data[QStringLiteral("Value")] = QStringLiteral("data_%1").arg(i);
			auto lChanged = _watcher->loadData(TestTable);
			QVERIFY(errorSpy.isEmpty());
			QVERIFY(lChanged);
			QCOMPARE(lChanged->key().typeName, TestTable);
			QCOMPARE(lChanged->key().id, QString::number(i));
			QCOMPARE(lChanged->modified(), mTime);
			QCOMPARE(lChanged->version(), std::nullopt);
			QVERIFY(!lChanged->uploaded().isValid());
			QCOMPARE(lChanged->data(), data);

			VERIFY_CHANGED(i, mTime, mTime);
			// mark unchanged invalid tstamp
			_watcher->markUnchanged({TestTable, QString::number(i)}, mTime.addSecs(3));
			QVERIFY(errorSpy.isEmpty());
			VERIFY_CHANGED(i, mTime, mTime);
			// mark unchanged valid tstamp
			_watcher->markUnchanged({TestTable, QString::number(i)}, mTime);
			QVERIFY(errorSpy.isEmpty());
			VERIFY_UNCHANGED1(i, mTime);
		}

		QVERIFY(!_watcher->loadData(TestTable));
		QVERIFY(errorSpy.isEmpty());
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testMarkCorrupted()
{
	try {
		QSignalSpy errorSpy{_watcher, &DatabaseWatcher::databaseError};
		QVERIFY(errorSpy.isValid());

		// modify corrupted
		const auto tStamp = QDateTime::currentDateTimeUtc();
		markChanged(DatabaseWatcher::ChangeState::Unchanged);
		VERIFY_UNCHANGED2(0, past, tStamp);
		_watcher->markCorrupted({TestTable, QString::number(0)}, tStamp.addSecs(5));
		QVERIFY(errorSpy.isEmpty());
		VERIFY_CHANGED_STATE(0, past, tStamp, DatabaseWatcher::ChangeState::Corrupted); // old timestamp

		// add corrupted
		Query testNonExistantQuery{_watcher};
		testNonExistantQuery.prepare(QStringLiteral("SELECT * "
													"FROM TestData "
													"WHERE Key == ?"));
		testNonExistantQuery.addBindValue(4711);
		testNonExistantQuery.exec();
		QVERIFY(!testNonExistantQuery.first());
		_watcher->markCorrupted({TestTable, QString::number(4711)}, tStamp);
		QVERIFY(errorSpy.isEmpty());
		VERIFY_CHANGED_STATE(4711, tStamp, tStamp, DatabaseWatcher::ChangeState::Corrupted);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testResync()
{
	QSignalSpy errorSpy{_watcher, &DatabaseWatcher::databaseError};
	QVERIFY(errorSpy.isValid());

	// test Download
	auto lSync = _watcher->lastSync(TestTable);
	QVERIFY(errorSpy.isEmpty());
	QVERIFY(lSync);
	QVERIFY(lSync->isValid());
	_watcher->resyncTable(TestTable, Engine::ResyncFlag::Download);
	lSync = _watcher->lastSync(TestTable);
	QVERIFY(errorSpy.isEmpty());
	QVERIFY(lSync);
	QVERIFY(!lSync->isValid());

	// test Upload
	markChanged(DatabaseWatcher::ChangeState::Unchanged);
	auto before = QDateTime::currentDateTimeUtc();
	_watcher->resyncTable(TestTable, Engine::ResyncFlag::Upload);
	auto after = QDateTime::currentDateTimeUtc();
	for (auto i = 0; i < 10; ++i)
		VERIFY_CHANGED(i, QDateTime{}, before);

	// test CheckLocalData
	markChanged(DatabaseWatcher::ChangeState::Unchanged);
	Query rmChanged{_watcher};
	rmChanged.exec(QStringLiteral("DELETE FROM __qtdatasync_sync_data_TestData "
								  "WHERE pkey >= 5;"));
	before = QDateTime::currentDateTimeUtc();
	_watcher->resyncTable(TestTable, Engine::ResyncFlag::CheckLocalData);
	after = QDateTime::currentDateTimeUtc();
	for (auto i = 0; i < 5; ++i)
		VERIFY_CHANGED_STATE(i, QDateTime{}, before, DatabaseWatcher::ChangeState::Unchanged);
	for (auto i = 5; i < 10; ++i)
		VERIFY_CHANGED(i, before, after);

	// test CleanLocalData
	markChanged(DatabaseWatcher::ChangeState::Corrupted);
	before = QDateTime::currentDateTimeUtc();
	_watcher->resyncTable(TestTable, Engine::ResyncFlag::CleanLocalData);
	after = QDateTime::currentDateTimeUtc();
	for (auto i = 0; i < 10; ++i)
		VERIFY_CHANGED(i, before, after);

	// test ClearLocalData
	_watcher->resyncTable(TestTable, Engine::ResyncFlag::ClearLocalData);
	Query testEmpty{_watcher};
	testEmpty.exec(QStringLiteral("SELECT COUNT(*) FROM TestData;"));
	QVERIFY(testEmpty.next());
	QCOMPARE(testEmpty.value(0).toInt(), 0);
}

void DbWatcherTest::testDropAndReactivate()
{
	try {
		QCOMPARE(_watcher->tables(), {TestTable});
		_watcher->dropAllTables();
		QCOMPARE(_watcher->hasTables(), false);
		_watcher->reactivateTables(false);
		QCOMPARE(_watcher->tables(), {TestTable});
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testUnsyncTable()
{
	try {
		QCOMPARE(_watcher->hasTables(), true);
		QVERIFY(_watcher->tables().contains(TestTable));

		QSignalSpy removeSpy{_watcher, &DatabaseWatcher::tableRemoved};
		QVERIFY(removeSpy.isValid());
		QSignalSpy syncSpy{_watcher, &DatabaseWatcher::triggerSync};
		QVERIFY(syncSpy.isValid());

		_watcher->unsyncTable(TestTable);

		QCOMPARE(_watcher->hasTables(), false);
		QVERIFY(!_watcher->tables().contains(TestTable));
		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::MetaTable));
		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::FieldTable));
		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::ReferenceTable));

		QCOMPARE(removeSpy.size(), 1);
		QCOMPARE(removeSpy[0][0].toString(), TestTable);

		// verify meta state
		Query metaQuery{_watcher};
		metaQuery.prepare(QStringLiteral("SELECT * "
										 "FROM __qtdatasync_meta_data "
										 "WHERE tableName == ?;"));
		metaQuery.addBindValue(TestTable);
		metaQuery.exec();
		QVERIFY(!metaQuery.first());

		QVERIFY(!_watcher->database().tables().contains(DatabaseWatcher::TablePrefix + TestTable));

		// verify no insert trigger
		Query insertQuery{_watcher};
		insertQuery.prepare(QStringLiteral("INSERT INTO TestData "
										   "(Key, Value) "
										   "VALUES(?, ?);"));
		insertQuery.addBindValue(13);
		insertQuery.addBindValue(QStringLiteral("data_13"));
		insertQuery.exec();

		QVERIFY(!syncSpy.wait(1000));
		QCOMPARE(syncSpy.size(), 0);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testDbActions()
{
	try {
		_watcher->addAllTables(false);
		QCOMPARE(_watcher->tables(), {TestTable});
		_watcher->removeAllTables();
		QCOMPARE(_watcher->hasTables(), false);
		_watcher->addAllTables(false);
		QCOMPARE(_watcher->tables(), {TestTable});
		_watcher->unsyncAllTables();
		QCOMPARE(_watcher->hasTables(), false);
		QVERIFY(!_watcher->database().tables().contains(DatabaseWatcher::MetaTable));
		QVERIFY(!_watcher->database().tables().contains(DatabaseWatcher::FieldTable));
		QVERIFY(!_watcher->database().tables().contains(DatabaseWatcher::ReferenceTable));
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testBinaryKey()
{
	try {
		QSignalSpy errorSpy{_watcher, &DatabaseWatcher::databaseError};
		QVERIFY(errorSpy.isValid());

		// create and sync table
		Query createQuery{_watcher};
		createQuery.exec(QStringLiteral("CREATE TABLE BinTest ("
										"	Key		BLOB NOT NULL PRIMARY KEY, "
										"	Value	REAL NOT NULL "
										");"));

		const auto table = QStringLiteral("BinTest");
		_watcher->addTable(TableConfig{table, _watcher->database()});

		for (auto i = 5; i < 10; ++i) {
			const auto key = QByteArray(i, static_cast<char>(i));
			_watcher->storeData({
				table, QString::fromUtf8(key.toBase64()),
				QVariantHash {
					{QStringLiteral("Key"), key},
					{QStringLiteral("Value"), i/10.0},
				},
				QDateTime::currentDateTimeUtc(),
				std::nullopt
			});
			QVERIFY(errorSpy.isEmpty());

			Query dataQuery{_watcher};
			dataQuery.prepare(QStringLiteral("SELECT Value "
											 "FROM BinTest "
											 "WHERE Key == ?"));
			dataQuery.addBindValue(key);
			dataQuery.exec();
			QVERIFY(dataQuery.first());
			QCOMPARE(dataQuery.value(0).toDouble(), i/10.0);

			Query metaQuery{_watcher};
			metaQuery.prepare(QStringLiteral("UPDATE __qtdatasync_sync_data_BinTest "
											 "SET changed = ? "
											 "WHERE pkey == ?"));
			metaQuery.addBindValue(static_cast<int>(DatabaseWatcher::ChangeState::Changed));
			metaQuery.addBindValue(key);
			metaQuery.exec();
			QCOMPARE(metaQuery.numRowsAffected(), 1);
		}

		for (auto i = 0; i < 5; ++i) {
			const auto data = _watcher->loadData(table);
			QVERIFY(errorSpy.isEmpty());
			QVERIFY(data);
			QVERIFY(data->data());
			QCOMPARE(data->key().id,
					 QString::fromUtf8(data->data()->value(QStringLiteral("Key")).toByteArray().toBase64()));
			QCOMPARE(QByteArray::fromBase64(data->key().id.toUtf8()),
					 data->data()->value(QStringLiteral("Key")).toByteArray());
			_watcher->markUnchanged(data->key(), data->modified());
			QVERIFY(errorSpy.isEmpty());
		}
		QVERIFY(!_watcher->loadData(table));
		QVERIFY(errorSpy.isEmpty());

		_watcher->removeTable(table);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testSyncCustomFields()
{
	try {
		// create and sync table
		Query createQuery{_watcher};
		createQuery.exec(QStringLiteral("CREATE TABLE FieldTest ("
										"	Key	INTEGER NOT NULL PRIMARY KEY, "
										"	a	TEXT NOT NULL, "
										"	b	TEXT NOT NULL DEFAULT 'B', "
										"	c	TEXT NOT NULL DEFAULT 'C', "
										"	d	TEXT NOT NULL DEFAULT 'D', "
										"	e	TEXT NOT NULL DEFAULT 'E'"
										");"));

		const auto table = QStringLiteral("FieldTest");
		TableConfig config{table, _watcher->database()};

		{
			config.setFields({QStringLiteral("Key"), QStringLiteral("a"), QStringLiteral("c"), QStringLiteral("d")});
			_watcher->addTable(config);

			// verify correct fields
			Query fieldsQuery{_watcher};
			fieldsQuery.prepare(QStringLiteral("SELECT syncField "
											   "FROM __qtdatasync_sync_fields "
											   "WHERE tableName == ?;"));
			fieldsQuery.addBindValue(table);
			fieldsQuery.exec();
			auto xFields = config.fields();
			while (!xFields.isEmpty()) {
				QVERIFY(fieldsQuery.next());
				QCOMPARE(xFields.removeAll(fieldsQuery.value(0).toString()), 1);
			}
			QVERIFY(!fieldsQuery.next());
		}

		{
			// remove sync
			config.setFields({QStringLiteral("Key"), QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
			_watcher->removeTable(table);

			// test fields still there
			Query rmFieldsQuery{_watcher};
			rmFieldsQuery.prepare(QStringLiteral("SELECT syncField "
											   "FROM __qtdatasync_sync_fields "
											   "WHERE tableName == ?;"));
			rmFieldsQuery.addBindValue(table);
			rmFieldsQuery.exec();
			QStringList xFields {QStringLiteral("Key"), QStringLiteral("a"), QStringLiteral("c"), QStringLiteral("d")};
			while (!xFields.isEmpty()) {
				QVERIFY(rmFieldsQuery.next());
				QCOMPARE(xFields.removeAll(rmFieldsQuery.value(0).toString()), 1);
			}
			QVERIFY(!rmFieldsQuery.next());

			// add again, no force
			_watcher->addTable(config);

			// verify correct fields
			Query addFieldsQuery{_watcher};
			addFieldsQuery.prepare(QStringLiteral("SELECT syncField "
											   "FROM __qtdatasync_sync_fields "
											   "WHERE tableName == ?;"));
			addFieldsQuery.addBindValue(table);
			addFieldsQuery.exec();
			xFields = QStringList {QStringLiteral("Key"), QStringLiteral("a"), QStringLiteral("c"), QStringLiteral("d")};
			while (!xFields.isEmpty()) {
				QVERIFY(addFieldsQuery.next());
				QCOMPARE(xFields.removeAll(addFieldsQuery.value(0).toString()), 1);
			}
			QVERIFY(!addFieldsQuery.next());
		}

		{
			// add again, with force
			config.setFields({QStringLiteral("Key"), QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
			config.setForceCreate(true);
			_watcher->dropTable(table);  // no test needed is part of remove
			_watcher->addTable(config);

			// verify correct fields
			Query fieldsQuery{_watcher};
			fieldsQuery.prepare(QStringLiteral("SELECT syncField "
											   "FROM __qtdatasync_sync_fields "
											   "WHERE tableName == ?;"));
			fieldsQuery.addBindValue(table);
			fieldsQuery.exec();
			auto xFields = config.fields();
			while (!xFields.isEmpty()) {
				QVERIFY(fieldsQuery.next());
				QCOMPARE(xFields.removeAll(fieldsQuery.value(0).toString()), 1);
			}
			QVERIFY(!fieldsQuery.next());
		}
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testStoreCustomFields_data()
{
	QTest::addColumn<LocalData>("storeData");
	QTest::addColumn<QVariantList>("values");
	QTest::addColumn<LocalData>("loadData");

	const auto table = QStringLiteral("FieldTest");
	auto when = QDateTime::currentDateTimeUtc().addSecs(-10);
	QTest::newRow("basic.insert") << LocalData {
		table, QString::number(0),
		QVariantHash {
			{QStringLiteral("Key"), 0},
			{QStringLiteral("a"), 1},
			{QStringLiteral("b"), 2},
			{QStringLiteral("c"), 3},
		},
		when,
		std::nullopt
	} << QVariantList {
		1, 2, 3, QStringLiteral("D"), QStringLiteral("E")
	} << LocalData {
		table, QString::number(0),
		QVariantHash {
			{QStringLiteral("Key"), 0},
			{QStringLiteral("a"), 1},
			{QStringLiteral("b"), 2},
			{QStringLiteral("c"), 3},
			},
		when,
		std::nullopt
	};

	when = when.addSecs(1);
	QTest::newRow("basic.update") << LocalData {
		table, QString::number(0),
		QVariantHash {
			{QStringLiteral("Key"), 0},
			{QStringLiteral("a"), 10},
			{QStringLiteral("b"), 20},
			{QStringLiteral("c"), 30},
		},
		when,
		std::nullopt
	} << QVariantList {
		10, 20, 30, QStringLiteral("D"), QStringLiteral("E")
	} << LocalData {
		table, QString::number(0),
		QVariantHash {
			{QStringLiteral("Key"), 0},
			{QStringLiteral("a"), 10},
			{QStringLiteral("b"), 20},
			{QStringLiteral("c"), 30},
			},
		when,
		std::nullopt
	};

	when = when.addSecs(1);
	QTest::newRow("extra.insert") << LocalData {
		table, QString::number(1),
		QVariantHash {
			{QStringLiteral("Key"), 1},
			{QStringLiteral("a"), 1},
			{QStringLiteral("b"), 2},
			{QStringLiteral("d"), 3},
			{QStringLiteral("f"), 4},
		},
		when,
		std::nullopt
	} << QVariantList {
		1, 2, QStringLiteral("C"), QStringLiteral("D"), QStringLiteral("E")
	} << LocalData {
		table, QString::number(1),
		QVariantHash {
			{QStringLiteral("Key"), 1},
			{QStringLiteral("a"), 1},
			{QStringLiteral("b"), 2},
			{QStringLiteral("c"), QStringLiteral("C")},
			},
		when,
		std::nullopt
	};

	when = when.addSecs(1);
	QTest::newRow("extra.update") << LocalData {
		table, QString::number(1),
		QVariantHash {
			{QStringLiteral("Key"), 1},
			{QStringLiteral("a"), 10},
			{QStringLiteral("c"), 20},
			{QStringLiteral("e"), 30},
			{QStringLiteral("g"), 30},
		},
		when,
		std::nullopt
	} << QVariantList {
		10, 2, 20, QStringLiteral("D"), QStringLiteral("E")
	} << LocalData {
		table, QString::number(1),
		QVariantHash {
			{QStringLiteral("Key"), 1},
			{QStringLiteral("a"), 10},
			{QStringLiteral("b"), 2},
			{QStringLiteral("c"), 20},
			},
		when,
		std::nullopt
	};

	QTest::newRow("remove") << LocalData {
		table, QString::number(1),
		std::nullopt,
		when,
		std::nullopt
	} << QVariantList{
	} << LocalData {
		table, QString::number(1),
		std::nullopt,
		when,
		std::nullopt
	};
}

void DbWatcherTest::testStoreCustomFields()
{
	QFETCH(LocalData, storeData);
	QFETCH(QVariantList, values);
	QFETCH(LocalData, loadData);

	try {
		QSignalSpy errorSpy{_watcher, &DatabaseWatcher::databaseError};
		QVERIFY(errorSpy.isValid());

		// "download" remote data
		_watcher->storeData(storeData);
		QVERIFY(errorSpy.isEmpty());

		// verify data
		Query localDataQuery{_watcher};
		localDataQuery.prepare(QStringLiteral("SELECT a, b, c, d, e "
											  "FROM FieldTest "
											  "WHERE Key == ?"));
		localDataQuery.addBindValue(storeData.key().id);
		localDataQuery.exec();
		if (values.isEmpty())
			QVERIFY(!localDataQuery.next());
		else {
			QVERIFY(localDataQuery.next());
			QCOMPARE(localDataQuery.value(0), values[0]);
			QCOMPARE(localDataQuery.value(1), values[1]);
			QCOMPARE(localDataQuery.value(2), values[2]);
			QCOMPARE(localDataQuery.value(3), values[3]);
			QCOMPARE(localDataQuery.value(4), values[4]);
			QVERIFY(!localDataQuery.next());
		}

		// mark changed and load
		VERIFY_CHANGED_STATE_KEY(storeData.key(), storeData.modified(), storeData.modified(), DatabaseWatcher::ChangeState::Unchanged);
		markChanged(DatabaseWatcher::ChangeState::Changed, storeData.key());
		auto lData = _watcher->loadData(QStringLiteral("FieldTest"));
		QCOMPARE(*lData, loadData);
		// mark unchanged
		_watcher->markUnchanged(loadData.key(), loadData.modified());
		VERIFY_CHANGED_STATE_KEY(loadData.key(), loadData.modified(), loadData.modified(), DatabaseWatcher::ChangeState::Unchanged);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testUnsyncCustomFields()
{
	// unsync the table
	const auto table = QStringLiteral("FieldTest");
	_watcher->unsyncTable(table);

	// verify all fields removed
	Query fieldsQuery{_watcher};
	fieldsQuery.prepare(QStringLiteral("SELECT syncField "
									   "FROM __qtdatasync_sync_fields "
									   "WHERE tableName == ?;"));
	fieldsQuery.addBindValue(table);
	fieldsQuery.exec();
	QVERIFY(!fieldsQuery.first());
}

void DbWatcherTest::fillDb()
{
	try {
		Query createQuery{_watcher};
		createQuery.exec(QStringLiteral("CREATE TABLE TestData ("
										"	Key		INTEGER NOT NULL PRIMARY KEY, "
										"	Value	TEXT NOT NULL "
										");"));

		for (auto i = 0; i < 10; ++i) {
			Query fillQuery{_watcher};
			fillQuery.prepare(QStringLiteral("INSERT INTO TestData "
											 "(Key, Value) "
											 "VALUES(?, ?)"));
			fillQuery.addBindValue(i);
			fillQuery.addBindValue(QStringLiteral("data_%1").arg(i));
			fillQuery.exec();
		}
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::markChanged(DatabaseWatcher::ChangeState state, const QString &key)
{
	markChanged(state, {TestTable, key});
}

void DbWatcherTest::markChanged(DatabaseWatcher::ChangeState state, const ObjectKey &key)
{
	if (key.id.isEmpty()) {
		Query markChangedQuery{_watcher};
		markChangedQuery.prepare(QStringLiteral("UPDATE %1 "
												"SET changed = ?;")
									 .arg(DatabaseWatcher::TablePrefix + key.typeName));
		markChangedQuery.addBindValue(static_cast<int>(state));
		markChangedQuery.exec();
	} else {
		Query markChangedQuery{_watcher};
		markChangedQuery.prepare(QStringLiteral("UPDATE %1 "
												"SET changed = ? "
												"WHERE pkey == ?;")
									 .arg(DatabaseWatcher::TablePrefix + key.typeName));
		markChangedQuery.addBindValue(static_cast<int>(state));
		markChangedQuery.addBindValue(key.id);
		markChangedQuery.exec();
	}
}

QTEST_MAIN(DbWatcherTest)

#include "tst_dbwatcher.moc"

