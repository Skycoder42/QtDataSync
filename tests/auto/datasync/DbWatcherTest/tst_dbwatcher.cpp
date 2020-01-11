#include <QtTest>
#include <QtDataSync>

#include <QtDataSync/private/databasewatcher_p.h>
using namespace QtDataSync;

#define VERIFY_CHANGED_STATE(key, before, after, state) do { \
	Query testChangedQuery{_watcher->database()}; \
	testChangedQuery.prepare(QStringLiteral("SELECT tstamp, changed " \
										"FROM %1 " \
										"WHERE pkey == ?;") \
							 .arg(DatabaseWatcher::TablePrefix + TestTable)); \
	testChangedQuery.addBindValue(key); \
	testChangedQuery.exec(); \
	QVERIFY(testChangedQuery.next()); \
	QVERIFY(testChangedQuery.value(0).toDateTime() >= before); \
	QVERIFY(testChangedQuery.value(0).toDateTime() <= after); \
	QCOMPARE(testChangedQuery.value(1).toInt(), static_cast<int>(state)); \
	QVERIFY(!testChangedQuery.next()); \
} while(false)

#define VERIFY_CHANGED(key, before, after) VERIFY_CHANGED_STATE(key, before, after, DatabaseWatcher::ChangeState::Changed)
#define VERIFY_UNCHANGED(key, when) VERIFY_CHANGED_STATE(key, when, when, DatabaseWatcher::ChangeState::Unchanged)

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

	void testResync();

	void testReactivate();
	void testUnsyncTable();
	void testDbActions();

private:
	static const QString TestTable;

	class Query : public ExQuery {
	public:
		inline Query(QSqlDatabase db) :
			ExQuery{db, DatabaseWatcher::ErrorScope::Database, {}}
		{}
	};

	QTemporaryDir _tDir;
	DatabaseWatcher *_watcher;

	void fillDb();
	void markChanged(DatabaseWatcher::ChangeState state, const QString &key = {});
};

const QString DbWatcherTest::TestTable = QStringLiteral("TestData");

void DbWatcherTest::initTestCase()
{
	_watcher = new DatabaseWatcher{QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"))};
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
		_watcher->addTable(TestTable, false);
		auto after = QDateTime::currentDateTimeUtc();

		QCOMPARE(_watcher->hasTables(), true);
		QVERIFY(_watcher->tables().contains(TestTable));

		QCOMPARE(addSpy.size(), 1);
		QCOMPARE(addSpy[0][0].toString(), TestTable);

		QCOMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();

		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::MetaTable));
		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::TablePrefix + TestTable));

		// verify meta state
		Query metaQuery{_watcher->database()};
		metaQuery.prepare(QStringLiteral("SELECT pkey, state, lastSync "
										 "FROM %1 "
										 "WHERE tableName == ?;")
							  .arg(DatabaseWatcher::MetaTable));
		metaQuery.addBindValue(TestTable);
		metaQuery.exec();
		QVERIFY(metaQuery.first());
		QCOMPARE(metaQuery.value(0).toString(), QStringLiteral("Key"));
		QCOMPARE(metaQuery.value(1).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
		QVERIFY(metaQuery.value(2).isNull());

		// verify sync state
		Query syncQuery{_watcher->database()};
		syncQuery.exec(QStringLiteral("SELECT pkey, tstamp, changed "
									  "FROM %1 "
									  "ORDER BY pkey ASC;")
						   .arg(DatabaseWatcher::TablePrefix + TestTable));
		for (auto i = 0; i < 10; ++i) {
			QVERIFY(syncQuery.next());
			QCOMPARE(syncQuery.value(0).toInt(), i);
			QVERIFY(syncQuery.value(1).toDateTime() >= before);
			QVERIFY(syncQuery.value(1).toDateTime() <= after);
			QCOMPARE(syncQuery.value(2).toInt(), static_cast<int>(DatabaseWatcher::ChangeState::Changed));
		}
		QVERIFY(!syncQuery.next());

		// verify insert trigger
		Query insertQuery{_watcher->database()};
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
		_watcher->markUnchanged({TestTable, QStringLiteral("4")}, after);
		VERIFY_UNCHANGED(4, after);

		// verify update trigger
		Query updateQuery{_watcher->database()};
		updateQuery.prepare(QStringLiteral("UPDATE TestData "
										   "SET Value = ? "
										   "WHERE Key == ?;"));
		updateQuery.addBindValue(QStringLiteral("baum42"));
		updateQuery.addBindValue(4);
		before = QDateTime::currentDateTimeUtc();
		updateQuery.exec();
		after = QDateTime::currentDateTimeUtc();
		VERIFY_CHANGED(4, before, after);

		QTRY_COMPARE(syncSpy.size(), 2);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));

		// clear changed flag
		_watcher->markUnchanged({TestTable, QStringLiteral("4")}, after);
		VERIFY_UNCHANGED(4, after);

		// verify rename trigger
		Query renameQuery{_watcher->database()};
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

		QTRY_COMPARE(syncSpy.size(), 3);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));

		// clear changed flag
		_watcher->markUnchanged({TestTable, QStringLiteral("12")}, after);
		VERIFY_UNCHANGED(12, after);

		// verify delete trigger
		Query deleteQuery{_watcher->database()};
		deleteQuery.prepare(QStringLiteral("DELETE FROM TestData "
										   "WHERE Key == ?;"));
		deleteQuery.addBindValue(12);
		before = QDateTime::currentDateTimeUtc();
		deleteQuery.exec();
		after = QDateTime::currentDateTimeUtc();
		VERIFY_CHANGED(12, before, after);

		QTRY_COMPARE(syncSpy.size(), 2);
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
		Query metaQuery{_watcher->database()};
		metaQuery.prepare(QStringLiteral("SELECT pkey, state, lastSync "
										 "FROM %1 "
										 "WHERE tableName == ?;")
							  .arg(DatabaseWatcher::MetaTable));
		metaQuery.addBindValue(TestTable);
		metaQuery.exec();
		QVERIFY(metaQuery.first());
		QCOMPARE(metaQuery.value(0).toString(), QStringLiteral("Key"));
		QCOMPARE(metaQuery.value(1).toInt(), static_cast<int>(DatabaseWatcher::TableState::Inactive));
		QVERIFY(metaQuery.value(2).isNull());

		// verify insert trigger
		Query insertQuery{_watcher->database()};
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
		_watcher->addTable(TestTable, false);
		auto after = QDateTime::currentDateTimeUtc();

		QCOMPARE(_watcher->hasTables(), true);
		QVERIFY(_watcher->tables().contains(TestTable));

		QCOMPARE(addSpy.size(), 1);
		QCOMPARE(addSpy[0][0].toString(), TestTable);

		QCOMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), TestTable);
		syncSpy.clear();

		// verify meta state
		Query metaQuery{_watcher->database()};
		metaQuery.prepare(QStringLiteral("SELECT pkey, state, lastSync "
										 "FROM %1 "
										 "WHERE tableName == ?;")
							  .arg(DatabaseWatcher::MetaTable));
		metaQuery.addBindValue(TestTable);
		metaQuery.exec();
		QVERIFY(metaQuery.first());
		QCOMPARE(metaQuery.value(0).toString(), QStringLiteral("Key"));
		QCOMPARE(metaQuery.value(1).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
		QVERIFY(metaQuery.value(2).isNull());

		// verify insert trigger
		Query insertQuery{_watcher->database()};
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
		QVERIFY(lSync);
		QCOMPARE(*lSync, lastSync);
		QVERIFY(errorSpy.isEmpty());

		QVariantList oldData;
		if (!isStored) {
			Query getOldData{_watcher->database()};
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
		Query checkDataQuery{_watcher->database()};
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
		QVERIFY(lSync);
		QCOMPARE(*lSync, data.uploaded());
		QVERIFY(errorSpy.isEmpty());

		// mark changed again
		markChanged(DatabaseWatcher::ChangeState::Changed, data.key().id);
		VERIFY_CHANGED(data.key().id, modified, modified);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testLoadData()
{
	try {
		// mark all unchanged
		markChanged(DatabaseWatcher::ChangeState::Unchanged);

		QVERIFY(!_watcher->loadData(TestTable));

		// mark changed
		const auto baseTime = QDateTime::currentDateTimeUtc();
		for (auto i = 0; i < 5; ++i) {
			Query markChangedQuery{_watcher->database()};
			markChangedQuery.prepare(QStringLiteral("UPDATE %1 "
													"SET changed = ?, tstamp = ? "
													"WHERE pkey == ?;")
										 .arg(DatabaseWatcher::TablePrefix + TestTable));
			markChangedQuery.addBindValue(static_cast<int>(DatabaseWatcher::ChangeState::Changed));
			markChangedQuery.addBindValue(baseTime.addSecs(-60 + (10 * i)));
			markChangedQuery.addBindValue(i);
			markChangedQuery.exec();
			QCOMPARE(markChangedQuery.numRowsAffected(), 1);
		}

		for (auto i = 0; i < 5; ++i) {
			const auto mTime = baseTime.addSecs(-60 + (10 * i));
			QVariantHash data;
			data[QStringLiteral("Key")] = i;
			data[QStringLiteral("Value")] = QStringLiteral("data_%1").arg(i);
			auto lChanged = _watcher->loadData(TestTable);
			QVERIFY(lChanged);
			QCOMPARE(lChanged->key().typeName, TestTable);
			QCOMPARE(lChanged->key().id, QString::number(i));
			QCOMPARE(lChanged->modified(), mTime);
			QVERIFY(!lChanged->uploaded().isValid());
			QCOMPARE(lChanged->data(), data);

			VERIFY_CHANGED(i, mTime, mTime);
			_watcher->markUnchanged({TestTable, QString::number(i)}, mTime);
			VERIFY_UNCHANGED(i, mTime);
		}

		QVERIFY(!_watcher->loadData(TestTable));
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testResync()
{
	// test Download
	auto lSync = _watcher->lastSync(TestTable);
	QVERIFY(lSync);
	QVERIFY(lSync->isValid());
	_watcher->resyncTable(TestTable, Engine::ResyncFlag::Download);
	lSync = _watcher->lastSync(TestTable);
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
	Query rmChanged{_watcher->database()};
	rmChanged.exec(QStringLiteral("DELETE FROM %1 "
								  "WHERE pkey >= 5;")
					   .arg(DatabaseWatcher::TablePrefix + TestTable));
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
	Query testEmpty{_watcher->database()};
	testEmpty.exec(QStringLiteral("SELECT COUNT(*) FROM TestData;"));
	QVERIFY(testEmpty.next());
	QCOMPARE(testEmpty.value(0).toInt(), 0);
}

void DbWatcherTest::testReactivate()
{
	try {
		QCOMPARE(_watcher->tables(), {TestTable});
		delete _watcher;
		_watcher = new DatabaseWatcher{QSqlDatabase::database(), this};
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

		QCOMPARE(removeSpy.size(), 1);
		QCOMPARE(removeSpy[0][0].toString(), TestTable);

		// verify meta state
		Query metaQuery{_watcher->database()};
		metaQuery.prepare(QStringLiteral("SELECT pkey, state, lastSync "
										 "FROM %1 "
										 "WHERE tableName == ?;")
							  .arg(DatabaseWatcher::MetaTable));
		metaQuery.addBindValue(TestTable);
		metaQuery.exec();
		QVERIFY(!metaQuery.first());

		QVERIFY(!_watcher->database().tables().contains(DatabaseWatcher::TablePrefix + TestTable));

		// verify insert trigger
		Query insertQuery{_watcher->database()};
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
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::fillDb()
{
	try {
		Query createQuery{_watcher->database()};
		createQuery.exec(QStringLiteral("CREATE TABLE TestData ("
										"	Key		INTEGER NOT NULL PRIMARY KEY, "
										"	Value	TEXT NOT NULL "
										");"));

		for (auto i = 0; i < 10; ++i) {
			Query fillQuery{_watcher->database()};
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
	if (key.isEmpty()) {
		Query markChangedQuery{_watcher->database()};
		markChangedQuery.prepare(QStringLiteral("UPDATE %1 "
												"SET changed = ?;")
									 .arg(DatabaseWatcher::TablePrefix + TestTable));
		markChangedQuery.addBindValue(static_cast<int>(state));
		markChangedQuery.exec();
	} else {
		Query markChangedQuery{_watcher->database()};
		markChangedQuery.prepare(QStringLiteral("UPDATE %1 "
												"SET changed = ? "
												"WHERE pkey == ?;")
									 .arg(DatabaseWatcher::TablePrefix + TestTable));
		markChangedQuery.addBindValue(static_cast<int>(state));
		markChangedQuery.addBindValue(key);
		markChangedQuery.exec();
	}
}

QTEST_MAIN(DbWatcherTest)

#include "tst_dbwatcher.moc"

