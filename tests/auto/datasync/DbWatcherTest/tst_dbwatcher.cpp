#include <QtTest>
#include <QtDataSync>

#include <QtDataSync/private/databasewatcher_p.h>
using namespace QtDataSync;

#define VERIFY_CHANGED_STATE(key, before, after, state) do { \
	Query testChangedQuery{_watcher->database()}; \
	testChangedQuery.prepare(QStringLiteral("SELECT tstamp, changed " \
										"FROM %1 " \
										"WHERE pkey == ?;") \
							 .arg(DatabaseWatcher::TablePrefix + QStringLiteral("TestData"))); \
	testChangedQuery.addBindValue(key); \
	testChangedQuery.exec(); \
	QVERIFY(testChangedQuery.next()); \
	QVERIFY(testChangedQuery.value(0).toDateTime() >= before); \
	QVERIFY(testChangedQuery.value(0).toDateTime() <= after); \
	QCOMPARE(testChangedQuery.value(1).toInt(), static_cast<int>(DatabaseWatcher::ChangeState::state)); \
	QVERIFY(!testChangedQuery.next()); \
} while(false)

#define VERIFY_CHANGED(key, before, after) VERIFY_CHANGED_STATE(key, before, after, Changed)
#define VERIFY_UNCHANGED(key, when) VERIFY_CHANGED_STATE(key, when, when, Unchanged)

class DbWatcherTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testAddTable();
	void testRemoveTable();
	void testReaddTable();
	void testUnsyncTable();

	void testDbActions();

	// TODO test resync

private:
	class Query : public ExQuery {
	public:
		inline Query(QSqlDatabase db) :
			ExQuery{db, DatabaseWatcher::ErrorScope::Database, {}}
		{}
	};

	QTemporaryDir _tDir;
	DatabaseWatcher *_watcher;

	void fillDb();
};

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
		QVERIFY(!_watcher->tables().contains(QStringLiteral("TestData")));

		QSignalSpy addSpy{_watcher, &DatabaseWatcher::tableAdded};
		QVERIFY(addSpy.isValid());
		QSignalSpy syncSpy{_watcher, &DatabaseWatcher::triggerSync};
		QVERIFY(syncSpy.isValid());

		auto before = QDateTime::currentDateTimeUtc();
		_watcher->addTable(QStringLiteral("TestData"));
		auto after = QDateTime::currentDateTimeUtc();

		QCOMPARE(_watcher->hasTables(), true);
		QVERIFY(_watcher->tables().contains(QStringLiteral("TestData")));

		QCOMPARE(addSpy.size(), 1);
		QCOMPARE(addSpy[0][0].toString(), QStringLiteral("TestData"));

		QCOMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), QStringLiteral("TestData"));
		syncSpy.clear();

		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::MetaTable));
		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::TablePrefix + QStringLiteral("TestData")));

		// verify meta state
		Query metaQuery{_watcher->database()};
		metaQuery.prepare(QStringLiteral("SELECT pkey, state, lastSync "
										 "FROM %1 "
										 "WHERE tableName == ?;")
							  .arg(DatabaseWatcher::MetaTable));
		metaQuery.addBindValue(QStringLiteral("TestData"));
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
						   .arg(DatabaseWatcher::TablePrefix + QStringLiteral("TestData")));
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
		QCOMPARE(syncSpy[0][0].toString(), QStringLiteral("TestData"));
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));

		// clear changed flag
		_watcher->markUnchanged({QStringLiteral("TestData"), QStringLiteral("4")}, after);
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
		QCOMPARE(syncSpy[0][0].toString(), QStringLiteral("TestData"));
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));

		// clear changed flag
		_watcher->markUnchanged({QStringLiteral("TestData"), QStringLiteral("4")}, after);
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
		QCOMPARE(syncSpy[0][0].toString(), QStringLiteral("TestData"));
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));

		// clear changed flag
		_watcher->markUnchanged({QStringLiteral("TestData"), QStringLiteral("12")}, after);
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
		QCOMPARE(syncSpy[0][0].toString(), QStringLiteral("TestData"));
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
		QVERIFY(_watcher->tables().contains(QStringLiteral("TestData")));

		QSignalSpy removeSpy{_watcher, &DatabaseWatcher::tableRemoved};
		QVERIFY(removeSpy.isValid());
		QSignalSpy syncSpy{_watcher, &DatabaseWatcher::triggerSync};
		QVERIFY(syncSpy.isValid());

		_watcher->removeTable(QStringLiteral("TestData"));

		QCOMPARE(_watcher->hasTables(), false);
		QVERIFY(!_watcher->tables().contains(QStringLiteral("TestData")));

		QCOMPARE(removeSpy.size(), 1);
		QCOMPARE(removeSpy[0][0].toString(), QStringLiteral("TestData"));

		// verify meta state
		Query metaQuery{_watcher->database()};
		metaQuery.prepare(QStringLiteral("SELECT pkey, state, lastSync "
										 "FROM %1 "
										 "WHERE tableName == ?;")
							  .arg(DatabaseWatcher::MetaTable));
		metaQuery.addBindValue(QStringLiteral("TestData"));
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
		QVERIFY(!_watcher->tables().contains(QStringLiteral("TestData")));

		QSignalSpy addSpy{_watcher, &DatabaseWatcher::tableAdded};
		QVERIFY(addSpy.isValid());
		QSignalSpy syncSpy{_watcher, &DatabaseWatcher::triggerSync};
		QVERIFY(syncSpy.isValid());

		auto before = QDateTime::currentDateTimeUtc();
		_watcher->addTable(QStringLiteral("TestData"));
		auto after = QDateTime::currentDateTimeUtc();

		QCOMPARE(_watcher->hasTables(), true);
		QVERIFY(_watcher->tables().contains(QStringLiteral("TestData")));

		QCOMPARE(addSpy.size(), 1);
		QCOMPARE(addSpy[0][0].toString(), QStringLiteral("TestData"));

		QCOMPARE(syncSpy.size(), 1);
		QCOMPARE(syncSpy[0][0].toString(), QStringLiteral("TestData"));
		syncSpy.clear();

		// verify meta state
		Query metaQuery{_watcher->database()};
		metaQuery.prepare(QStringLiteral("SELECT pkey, state, lastSync "
										 "FROM %1 "
										 "WHERE tableName == ?;")
							  .arg(DatabaseWatcher::MetaTable));
		metaQuery.addBindValue(QStringLiteral("TestData"));
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
		QCOMPARE(syncSpy[0][0].toString(), QStringLiteral("TestData"));
		syncSpy.clear();
		QVERIFY(!syncSpy.wait(1000));
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testUnsyncTable()
{
	try {
		QCOMPARE(_watcher->hasTables(), true);
		QVERIFY(_watcher->tables().contains(QStringLiteral("TestData")));

		QSignalSpy removeSpy{_watcher, &DatabaseWatcher::tableRemoved};
		QVERIFY(removeSpy.isValid());
		QSignalSpy syncSpy{_watcher, &DatabaseWatcher::triggerSync};
		QVERIFY(syncSpy.isValid());

		_watcher->unsyncTable(QStringLiteral("TestData"));

		QCOMPARE(_watcher->hasTables(), false);
		QVERIFY(!_watcher->tables().contains(QStringLiteral("TestData")));
		QVERIFY(_watcher->database().tables().contains(DatabaseWatcher::MetaTable));

		QCOMPARE(removeSpy.size(), 1);
		QCOMPARE(removeSpy[0][0].toString(), QStringLiteral("TestData"));

		// verify meta state
		Query metaQuery{_watcher->database()};
		metaQuery.prepare(QStringLiteral("SELECT pkey, state, lastSync "
										 "FROM %1 "
										 "WHERE tableName == ?;")
							  .arg(DatabaseWatcher::MetaTable));
		metaQuery.addBindValue(QStringLiteral("TestData"));
		metaQuery.exec();
		QVERIFY(!metaQuery.first());

		QVERIFY(!_watcher->database().tables().contains(DatabaseWatcher::TablePrefix + QStringLiteral("TestData")));

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
		_watcher->addAllTables();
		QCOMPARE(_watcher->tables(), {QStringLiteral("TestData")});
		_watcher->removeAllTables();
		QCOMPARE(_watcher->hasTables(), false);
		_watcher->addAllTables();
		QCOMPARE(_watcher->tables(), {QStringLiteral("TestData")});
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

QTEST_MAIN(DbWatcherTest)

#include "tst_dbwatcher.moc"

