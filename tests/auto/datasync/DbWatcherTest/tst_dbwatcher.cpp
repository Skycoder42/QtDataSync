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

#define VERIFY_CHANGED_STATE_KEY(datasetId, before, after, state) VERIFY_CHANGED_STATE_BASE((datasetId).key, before, after, state, (datasetId).tableName)

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

	void testPKey();
	void testSyncCustomFields();
	void testStoreCustomFields_data();
	void testStoreCustomFields();
	void testUnsyncCustomFields();
	void testVersionedTable();

	void testReferences();

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
	void markChanged(DatabaseWatcher::ChangeState state, const QtDataSync::DatasetId &key);
};

const QString DbWatcherTest::TestTable = QStringLiteral("TestData");

void DbWatcherTest::initTestCase()
{
	qRegisterMetaType<DatabaseWatcher::ErrorScope>("ErrorScope");

	qDebug().noquote() << "Test directory:" << _tDir.path();
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

	auto lastSync = QDateTime::currentDateTimeUtc().addSecs(-10);
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

	lastSync = lastSync.addSecs(1);
	QTest::newRow("storeVersioned") << lastSync.addSecs(-1)
									<< LocalData {
										   TestTable, QString::number(77),
										   QVariantHash{
											   {QStringLiteral("Key"), 77},
											   {QStringLiteral("Value"), 77}
										   },
										   QDateTime::currentDateTimeUtc().addSecs(-10),
										   QVersionNumber{1,2,3},
										   lastSync
									   }
									<< QDateTime::currentDateTimeUtc().addSecs(-10)
									<< DatabaseWatcher::ChangeState::Unchanged
									<< true;

	lastSync = lastSync.addSecs(1);
	QTest::newRow("deleteVersioned") << lastSync.addSecs(-1)
									 << LocalData {
											TestTable, QString::number(77),
											std::nullopt,
											QDateTime::currentDateTimeUtc().addSecs(-5),
											QVersionNumber{3,2,1},
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
			getOldData.addBindValue(data.key().key);
			getOldData.exec();
			if (getOldData.next()) {
				oldData.append(getOldData.value(0));
				oldData.append(getOldData.value(1));
				QVERIFY(!getOldData.next());
			}
		}

		// check if new data should be stored
		auto canStore = _watcher->shouldStore(data.key(), CloudData{data.key(), std::nullopt, data});
		QVERIFY(errorSpy.isEmpty());
		QCOMPARE(canStore, isStored);

		// store new data
		_watcher->storeData(data);
		QVERIFY(errorSpy.isEmpty());

		VERIFY_CHANGED_STATE(data.key().key, modified, modified, changed);
		Query checkDataQuery{_watcher};
		checkDataQuery.prepare(QStringLiteral("SELECT Key, Value "
											  "FROM TestData "
											  "WHERE Key == ?;"));
		checkDataQuery.addBindValue(data.key().key);
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
		VERIFY_CHANGED(data.key().key, modified, modified);
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
			QCOMPARE(lChanged->key().tableName, TestTable);
			QCOMPARE(lChanged->key().key, QString::number(i));
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
			QCOMPARE(data->key().key,
					 QString::fromUtf8(data->data()->value(QStringLiteral("Key")).toByteArray().toBase64()));
			QCOMPARE(QByteArray::fromBase64(data->key().key.toUtf8()),
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

void DbWatcherTest::testPKey()
{
	try {
		QSignalSpy errorSpy{_watcher, &DatabaseWatcher::databaseError};
		QVERIFY(errorSpy.isValid());

		// create custom pkey table
		Query createQuery{_watcher};
		createQuery.exec(QStringLiteral("CREATE TABLE PKeyTest ("
										"	AutoKey		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
										"	TestKey		INTEGER NOT NULL UNIQUE, "
										"	Value		REAL NOT NULL"
										");"));

		const auto table = QStringLiteral("PKeyTest");
		TableConfig config{table, _watcher->database()};

		{
			config.setPrimaryKey(QStringLiteral("TestKey"));
			_watcher->addTable(config);

			// verify correct pkey
			Query pKeyQuery{_watcher};
			pKeyQuery.prepare(QStringLiteral("SELECT pkeyName, pkeyType, state "
											   "FROM __qtdatasync_meta_data "
											   "WHERE tableName == ?;"));
			pKeyQuery.addBindValue(table);
			pKeyQuery.exec();
			QVERIFY(pKeyQuery.next());
			QCOMPARE(pKeyQuery.value(0).toString(), config.primaryKey());
			QCOMPARE(pKeyQuery.value(1).toInt(), static_cast<int>(QMetaType::Int));
			QCOMPARE(pKeyQuery.value(2).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
			QVERIFY(!pKeyQuery.next());
		}

		{
			// remove sync
			config.setPrimaryKey(QStringLiteral("Value"));
			_watcher->removeTable(table);

			// test pkey still there
			Query rmPKeyQuery{_watcher};
			rmPKeyQuery.prepare(QStringLiteral("SELECT pkeyName, pkeyType, state "
											   "FROM __qtdatasync_meta_data "
											   "WHERE tableName == ?;"));
			rmPKeyQuery.addBindValue(table);
			rmPKeyQuery.exec();
			QVERIFY(rmPKeyQuery.next());
			QCOMPARE(rmPKeyQuery.value(0).toString(), QStringLiteral("TestKey"));
			QCOMPARE(rmPKeyQuery.value(1).toInt(), static_cast<int>(QMetaType::Int));
			QCOMPARE(rmPKeyQuery.value(2).toInt(), static_cast<int>(DatabaseWatcher::TableState::Inactive));
			QVERIFY(!rmPKeyQuery.next());

			// add again, no force
			_watcher->addTable(config);

			// verify correct pkey
			Query addPkeyQuery{_watcher};
			addPkeyQuery.prepare(QStringLiteral("SELECT pkeyName, pkeyType, state "
												"FROM __qtdatasync_meta_data "
												"WHERE tableName == ?;"));
			addPkeyQuery.addBindValue(table);
			addPkeyQuery.exec();
			QVERIFY(addPkeyQuery.next());
			QCOMPARE(addPkeyQuery.value(0).toString(), QStringLiteral("TestKey"));
			QCOMPARE(addPkeyQuery.value(1).toInt(), static_cast<int>(QMetaType::Int));
			QCOMPARE(addPkeyQuery.value(2).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
			QVERIFY(!addPkeyQuery.next());
		}

		{
			// add again, with force -> will fail
			config.setPrimaryKey(QStringLiteral("Value"));
			config.setForceCreate(true);
			_watcher->dropTable(table);  // no test needed is part of remove
			QVERIFY_EXCEPTION_THROWN(_watcher->addTable(config), TableException);

			// keep force, but use same primary key -> will work
			config.setPrimaryKey(QStringLiteral("TestKey"));
			_watcher->addTable(config);

			// verify correct pkey
			Query pKeyQuery{_watcher};
			pKeyQuery.prepare(QStringLiteral("SELECT pkeyName, pkeyType, state "
											 "FROM __qtdatasync_meta_data "
											 "WHERE tableName == ?;"));
			pKeyQuery.addBindValue(table);
			pKeyQuery.exec();
			QVERIFY(pKeyQuery.next());
			QCOMPARE(pKeyQuery.value(0).toString(), config.primaryKey());
			QCOMPARE(pKeyQuery.value(1).toInt(), static_cast<int>(QMetaType::Int));
			QCOMPARE(pKeyQuery.value(2).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
			QVERIFY(!pKeyQuery.next());
		}

		const DatasetId key{table, QString::number(42)};
		{
			// store data and verify insert trigger works
			Query insertQuery{_watcher};
			insertQuery.prepare(QStringLiteral("INSERT INTO PKeyTest "
											   "(TestKey, Value) "
											   "VALUES(?, ?);"));
			insertQuery.addBindValue(42);
			insertQuery.addBindValue(4.2);
			const auto before = QDateTime::currentDateTimeUtc();
			insertQuery.exec();
			const auto after = QDateTime::currentDateTimeUtc();
			VERIFY_CHANGED_STATE_KEY(key, before, after, DatabaseWatcher::ChangeState::Changed);

			// test load data
			auto lData = _watcher->loadData(table);
			QVERIFY(errorSpy.isEmpty());
			QVERIFY(lData);
			QCOMPARE(lData->key(), key);
			QVERIFY(lData->data());
			QCOMPARE(lData->data()->value(QStringLiteral("AutoKey")).toInt(), 1);  // first AINC value for a clear database
			QCOMPARE(lData->data()->value(QStringLiteral("TestKey")).toInt(), 42);
			QCOMPARE(lData->data()->value(QStringLiteral("Value")).toDouble(), 4.2);
			QVERIFY(lData->modified() >= before);
			QVERIFY(lData->modified() <= after);

			// mark corrupted, then unchanged
			_watcher->markCorrupted(key, QDateTime::currentDateTimeUtc());
			QVERIFY(errorSpy.isEmpty());
			VERIFY_CHANGED_STATE_KEY(key, lData->modified(), lData->modified(), DatabaseWatcher::ChangeState::Corrupted);
			_watcher->markUnchanged(key, lData->modified());
			QVERIFY(errorSpy.isEmpty());
			VERIFY_CHANGED_STATE_KEY(key, lData->modified(), lData->modified(), DatabaseWatcher::ChangeState::Unchanged);
			QVERIFY(!_watcher->loadData(table));
		}

		auto newMod = QDateTime::currentDateTimeUtc().addSecs(1);
		{
			// store update
			_watcher->storeData({
				key,
				QVariantHash {
					{QStringLiteral("AutoKey"), 1},
					{QStringLiteral("TestKey"), 42},
					{QStringLiteral("Value"), 4.2}
				},
				newMod,
				std::nullopt
			});
			QVERIFY(errorSpy.isEmpty());
			VERIFY_CHANGED_STATE_KEY(key, newMod, newMod, DatabaseWatcher::ChangeState::Unchanged);
			Query getUpdateQuery {_watcher};
			getUpdateQuery.prepare(QStringLiteral("SELECT AutoKey, TestKey, Value "
												  "FROM PKeyTest "
												  "WHERE AutoKey == ? AND TestKey == ?"));
			getUpdateQuery.addBindValue(1);
			getUpdateQuery.addBindValue(42);
			getUpdateQuery.exec();
			QVERIFY(getUpdateQuery.next());
			QCOMPARE(getUpdateQuery.value(0).toInt(), 1);
			QCOMPARE(getUpdateQuery.value(1).toInt(), 42);
			QCOMPARE(getUpdateQuery.value(2).toDouble(), 4.2);
			QVERIFY(!getUpdateQuery.next());
		}

		{
			// store delete
			newMod = newMod.addSecs(1);
			_watcher->storeData({
				key,
				std::nullopt,
				newMod,
				std::nullopt
			});
			QVERIFY(errorSpy.isEmpty());
			VERIFY_CHANGED_STATE_KEY(key, newMod, newMod, DatabaseWatcher::ChangeState::Unchanged);
			Query getDeleteQuery {_watcher};
			getDeleteQuery.prepare(QStringLiteral("SELECT AutoKey, TestKey, Value "
												  "FROM PKeyTest "
												  "WHERE AutoKey == ? AND TestKey == ?"));
			getDeleteQuery.addBindValue(1);
			getDeleteQuery.addBindValue(42);
			getDeleteQuery.exec();
			QVERIFY(!getDeleteQuery.next());
		}

		{
			// store insert
			const DatasetId key2{table, QString::number(24)};
			newMod = newMod.addSecs(1);
			_watcher->storeData({
				key2,
				QVariantHash {
					{QStringLiteral("AutoKey"), 5},
					{QStringLiteral("TestKey"), 24},
					{QStringLiteral("Value"), 2.4}
				},
				newMod,
				std::nullopt
			});
			QVERIFY(errorSpy.isEmpty());
			VERIFY_CHANGED_STATE_KEY(key2, newMod, newMod, DatabaseWatcher::ChangeState::Unchanged);
			Query getInsertQuery {_watcher};
			getInsertQuery.prepare(QStringLiteral("SELECT AutoKey, TestKey, Value "
												  "FROM PKeyTest "
												  "WHERE AutoKey == ? AND TestKey == ?"));
			getInsertQuery.addBindValue(5);
			getInsertQuery.addBindValue(24);
			getInsertQuery.exec();
			QVERIFY(getInsertQuery.next());
			QCOMPARE(getInsertQuery.value(0).toInt(), 5);
			QCOMPARE(getInsertQuery.value(1).toInt(), 24);
			QCOMPARE(getInsertQuery.value(2).toDouble(), 2.4);
			QVERIFY(!getInsertQuery.next());
		}

		{
			// unsync table
			_watcher->unsyncTable(table);

			// test pkey entry gone now
			Query rmPKeyQuery{_watcher};
			rmPKeyQuery.prepare(QStringLiteral("SELECT pkeyName, pkeyType, state "
											   "FROM __qtdatasync_meta_data "
											   "WHERE tableName == ?;"));
			rmPKeyQuery.addBindValue(table);
			rmPKeyQuery.exec();
			QVERIFY(!rmPKeyQuery.first());
		}
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
		localDataQuery.addBindValue(storeData.key().key);
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

void DbWatcherTest::testVersionedTable()
{
	try {
		QSignalSpy errorSpy{_watcher, &DatabaseWatcher::databaseError};
		QVERIFY(errorSpy.isValid());

		// create table
		Query createQuery{_watcher};
		createQuery.exec(QStringLiteral("CREATE TABLE VersionedTable ("
										"	Key		INTEGER NOT NULL PRIMARY KEY, "
										"	Value	REAL NOT NULL "
										");"));

		const auto table = QStringLiteral("VersionedTable");
		TableConfig config{table, _watcher->database()};

		{
			config.setVersion({1});
			_watcher->addTable(config);

			// verify version in metadata
			Query getMetaQuery{_watcher};
			getMetaQuery.prepare(QStringLiteral("SELECT version, pkeyName, pkeyType, state "
												"FROM __qtdatasync_meta_data "
												"WHERE tableName == ?;"));
			getMetaQuery.addBindValue(table);
			getMetaQuery.exec();
			QVERIFY(getMetaQuery.next());
			QCOMPARE(getMetaQuery.value(0).toString(), config.version().toString());
			QCOMPARE(getMetaQuery.value(1).toString(), QStringLiteral("Key"));
			QCOMPARE(getMetaQuery.value(2).toInt(), static_cast<int>(QMetaType::Int));
			QCOMPARE(getMetaQuery.value(3).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
			QVERIFY(!getMetaQuery.next());

			// verify fields
			Query fieldsQuery{_watcher};
			fieldsQuery.prepare(QStringLiteral("SELECT syncField "
											   "FROM __qtdatasync_sync_fields "
											   "WHERE tableName == ?;"));
			fieldsQuery.addBindValue(table);
			fieldsQuery.exec();
			QStringList xFields = {QStringLiteral("Key"), QStringLiteral("Value")};
			while (!xFields.isEmpty()) {
				QVERIFY(fieldsQuery.next());
				QCOMPARE(xFields.removeAll(fieldsQuery.value(0).toString()), 1);
			}
			QVERIFY(!fieldsQuery.next());
		}

		{
			// add a field, but don't change version
			Query alterQuery{_watcher};
			alterQuery.exec(QStringLiteral("ALTER TABLE VersionedTable "
										   "ADD NewValue REAL NOT NULL DEFAULT 0.1;"));

			_watcher->dropTable(table);
			_watcher->addTable(config);

			// verify version in metadata
			Query getMetaQuery{_watcher};
			getMetaQuery.prepare(QStringLiteral("SELECT version, pkeyName, pkeyType, state "
												"FROM __qtdatasync_meta_data "
												"WHERE tableName == ?;"));
			getMetaQuery.addBindValue(table);
			getMetaQuery.exec();
			QVERIFY(getMetaQuery.next());
			QCOMPARE(getMetaQuery.value(0).toString(), config.version().toString());
			QCOMPARE(getMetaQuery.value(1).toString(), QStringLiteral("Key"));
			QCOMPARE(getMetaQuery.value(2).toInt(), static_cast<int>(QMetaType::Int));
			QCOMPARE(getMetaQuery.value(3).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
			QVERIFY(!getMetaQuery.next());

			// verify fields
			Query fieldsQuery{_watcher};
			fieldsQuery.prepare(QStringLiteral("SELECT syncField "
											   "FROM __qtdatasync_sync_fields "
											   "WHERE tableName == ?;"));
			fieldsQuery.addBindValue(table);
			fieldsQuery.exec();
			QStringList xFields = {QStringLiteral("Key"), QStringLiteral("Value")};
			while (!xFields.isEmpty()) {
				QVERIFY(fieldsQuery.next());
				QCOMPARE(xFields.removeAll(fieldsQuery.value(0).toString()), 1);
			}
			QVERIFY(!fieldsQuery.next());
		}

		{
			// change version
			config.setVersion({2});
			_watcher->dropTable(table);
			_watcher->addTable(config);

			// verify version in metadata
			Query getMetaQuery{_watcher};
			getMetaQuery.prepare(QStringLiteral("SELECT version, pkeyName, pkeyType, state "
												"FROM __qtdatasync_meta_data "
												"WHERE tableName == ?;"));
			getMetaQuery.addBindValue(table);
			getMetaQuery.exec();
			QVERIFY(getMetaQuery.next());
			QCOMPARE(getMetaQuery.value(0).toString(), config.version().toString());
			QCOMPARE(getMetaQuery.value(1).toString(), QStringLiteral("Key"));
			QCOMPARE(getMetaQuery.value(2).toInt(), static_cast<int>(QMetaType::Int));
			QCOMPARE(getMetaQuery.value(3).toInt(), static_cast<int>(DatabaseWatcher::TableState::Active));
			QVERIFY(!getMetaQuery.next());

			// verify fields
			Query fieldsQuery{_watcher};
			fieldsQuery.prepare(QStringLiteral("SELECT syncField "
											   "FROM __qtdatasync_sync_fields "
											   "WHERE tableName == ?;"));
			fieldsQuery.addBindValue(table);
			fieldsQuery.exec();
			QStringList xFields = {QStringLiteral("Key"), QStringLiteral("Value"), QStringLiteral("NewValue")};
			while (!xFields.isEmpty()) {
				QVERIFY(fieldsQuery.next());
				QCOMPARE(xFields.removeAll(fieldsQuery.value(0).toString()), 1);
			}
			QVERIFY(!fieldsQuery.next());
		}

		{
			// test load data has version
			const DatasetId key{table, QString::number(0)};
			Query insertQuery{_watcher};
			insertQuery.prepare(QStringLiteral("INSERT INTO VersionedTable "
											   "(Key, Value, NewValue) "
											   "VALUES(?, ?, ?);"));
			insertQuery.addBindValue(0);
			insertQuery.addBindValue(0.1);
			insertQuery.addBindValue(0.2);
			insertQuery.exec();

			auto lData = _watcher->loadData(table);
			QVERIFY(errorSpy.isEmpty());
			QVERIFY(lData);
			QCOMPARE(lData->key(), key);
			QCOMPARE(lData->version(), config.version());
		}

		{
			// test should store same version
			_watcher->shouldStore({table, QString::number(1)}, {
				table, QString::number(1),
				std::nullopt,
				QDateTime::currentDateTimeUtc(),
				config.version()
			});
			QVERIFY(errorSpy.isEmpty());

			// test store older version
			_watcher->shouldStore({table, QString::number(2)}, {
				table, QString::number(2),
				std::nullopt,
				QDateTime::currentDateTimeUtc(),
				QVersionNumber{1,5}
			});
			QVERIFY(errorSpy.isEmpty());

			// test store newer version -> fails
			_watcher->shouldStore({table, QString::number(3)}, {
				table, QString::number(3),
				std::nullopt,
				QDateTime::currentDateTimeUtc(),
				QVersionNumber{2,0,1}
			});
			QTRY_COMPARE(errorSpy.size(), 1);
			errorSpy.clear();

			// test should store unversioned
			_watcher->shouldStore({table, QString::number(1)}, {
				table, QString::number(4),
				std::nullopt,
				QDateTime::currentDateTimeUtc(),
				std::nullopt
			});
			QVERIFY(errorSpy.isEmpty());
		}

		{
			// test store same version
			_watcher->storeData({
				table, QString::number(1),
				std::nullopt,
				QDateTime::currentDateTimeUtc(),
				config.version()
			});
			QVERIFY(errorSpy.isEmpty());

			// test store older version
			_watcher->storeData({
				table, QString::number(2),
				std::nullopt,
				QDateTime::currentDateTimeUtc(),
				QVersionNumber{1,5}
			});
			QVERIFY(errorSpy.isEmpty());

			// test store newer version -> fails
			_watcher->storeData({
				table, QString::number(3),
				std::nullopt,
				QDateTime::currentDateTimeUtc(),
				QVersionNumber{2,0,1}
			});
			QTRY_COMPARE(errorSpy.size(), 1);
			errorSpy.clear();

			// test store unversioned
			_watcher->storeData({
				table, QString::number(4),
				std::nullopt,
				QDateTime::currentDateTimeUtc(),
				std::nullopt
			});
			QVERIFY(errorSpy.isEmpty());
		}

		_watcher->unsyncTable(table);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void DbWatcherTest::testReferences()
{
	try {
		QSignalSpy errorSpy{_watcher, &DatabaseWatcher::databaseError};
		QVERIFY(errorSpy.isValid());

		// enable referential integrity
		Query pragmaOnQuery{_watcher};
		pragmaOnQuery.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));

		// create and sync tables
		Query create1Query{_watcher};
		create1Query.exec(QStringLiteral("CREATE TABLE RefTestParent1 ("
										 "	Key		INTEGER NOT NULL PRIMARY KEY, "
										 "	Value	REAL NOT NULL DEFAULT 4.2 "
										 ");"));
		Query create2Query{_watcher};
		create2Query.exec(QStringLiteral("CREATE TABLE RefTestParent2 ("
										 "	Key		INTEGER NOT NULL PRIMARY KEY, "
										 "	Value	TEXT "
										 ");"));
		Query create3Query{_watcher};
		create3Query.exec(QStringLiteral("CREATE TABLE RefTestChild ("
										 "	Key1	INTEGER NOT NULL PRIMARY KEY, "
										 "	Key2	INTEGER NOT NULL, "
										 "	Value	TEXT NOT NULL, "
										 "	FOREIGN KEY(Key1) REFERENCES RefTestParent1(Key) ON UPDATE CASCADE ON DELETE CASCADE, "
										 "	FOREIGN KEY(Key2) REFERENCES RefTestParent2(Key) ON UPDATE CASCADE ON DELETE CASCADE "
										 ");"));

		const auto parent1 = QStringLiteral("RefTestParent1");
		const auto parent2 = QStringLiteral("RefTestParent2");
		const auto child = QStringLiteral("RefTestChild");

		// add parent tables
		_watcher->addTable(parent1);
		_watcher->addTable(parent2);

		// add child
		TableConfig config{child, _watcher->database()};
		{
			// only add 1 ref
			config.setReferences({std::make_pair(parent1, QStringLiteral("Key"))});
			_watcher->addTable(config);

			// verify correct refs
			Query refsQuery{_watcher};
			refsQuery.prepare(QStringLiteral("SELECT fKeyTable, fKeyField "
											 "FROM __qtdatasync_sync_references "
											 "WHERE tableName == ?;"));
			refsQuery.addBindValue(child);
			refsQuery.exec();
			auto xRefs = config.references();
			while (!xRefs.isEmpty()) {
				QVERIFY(refsQuery.next());
				QCOMPARE(xRefs.removeAll(std::make_pair(refsQuery.value(0).toString(),
														refsQuery.value(1).toString())),
						 1);
			}
			QVERIFY(!refsQuery.next());
		}

		{
			// remove sync
			config.setReferences({std::make_pair(parent2, QStringLiteral("Key"))});
			_watcher->removeTable(child);

			// test refs still there
			Query rmRefsQuery{_watcher};
			rmRefsQuery.prepare(QStringLiteral("SELECT fKeyTable, fKeyField "
												 "FROM __qtdatasync_sync_references "
												 "WHERE tableName == ?;"));
			rmRefsQuery.addBindValue(child);
			rmRefsQuery.exec();
			QList<TableConfig::Reference> xRefs {std::make_pair(parent1, QStringLiteral("Key"))};
			while (!xRefs.isEmpty()) {
				QVERIFY(rmRefsQuery.next());
				QCOMPARE(xRefs.removeAll(std::make_pair(rmRefsQuery.value(0).toString(),
														rmRefsQuery.value(1).toString())),
						 1);
			}
			QVERIFY(!rmRefsQuery.next());

			// add again, no force
			_watcher->addTable(config);

			// verify correct (old) refs
			Query addRefsQuery{_watcher};
			addRefsQuery.prepare(QStringLiteral("SELECT fKeyTable, fKeyField "
												"FROM __qtdatasync_sync_references "
												"WHERE tableName == ?;"));
			addRefsQuery.addBindValue(child);
			addRefsQuery.exec();
			xRefs = QList<TableConfig::Reference>{std::make_pair(parent1, QStringLiteral("Key"))};
			while (!xRefs.isEmpty()) {
				QVERIFY(addRefsQuery.next());
				QCOMPARE(xRefs.removeAll(std::make_pair(addRefsQuery.value(0).toString(),
														addRefsQuery.value(1).toString())),
						 1);
			}
			QVERIFY(!addRefsQuery.next());
		}

		{
			// add again, with force
			config.setReferences({
				std::make_pair(parent1, QStringLiteral("Key")),
				std::make_pair(parent2, QStringLiteral("Key"))
			});
			config.setForceCreate(true);
			_watcher->dropTable(child);  // no test needed is part of remove
			_watcher->addTable(config);

			// verify correct fields
			Query refsQuery{_watcher};
			refsQuery.prepare(QStringLiteral("SELECT fKeyTable, fKeyField "
											 "FROM __qtdatasync_sync_references "
											 "WHERE tableName == ?;"));
			refsQuery.addBindValue(child);
			refsQuery.exec();
			auto xRefs = config.references();
			while (!xRefs.isEmpty()) {
				QVERIFY(refsQuery.next());
				QCOMPARE(xRefs.removeAll(std::make_pair(refsQuery.value(0).toString(),
														refsQuery.value(1).toString())),
						 1);
			}
			QVERIFY(!refsQuery.next());
		}

		{
			const DatasetId pKey1{parent1, QString::number(0)};
			const DatasetId pKey2{parent2, QString::number(0)};
			const DatasetId cKey{child, QString::number(0)};

			// add data to parent
			Query addParent1Query{_watcher};
			addParent1Query.prepare(QStringLiteral("INSERT INTO RefTestParent1 "
												   "(Key, Value) "
												   "VALUES(?, ?);"));
			addParent1Query.addBindValue(0);
			addParent1Query.addBindValue(0.5);
			addParent1Query.exec();
			markChanged(DatabaseWatcher::ChangeState::Unchanged, pKey1);

			// store dataset
			auto mod = QDateTime::currentDateTimeUtc().addSecs(5);
			_watcher->storeData(LocalData {
				child, QString::number(0),
				QVariantHash {
					{QStringLiteral("Key1"), 0},
					{QStringLiteral("Key2"), 0},
					{QStringLiteral("Value"), QStringLiteral("baum")}
				},
				mod,
				std::nullopt
			});
			QVERIFY(errorSpy.isEmpty());

			// parent 1 has data -> unchanged
			VERIFY_CHANGED_STATE_KEY(pKey1, QDateTime{}, mod.addSecs(-4), DatabaseWatcher::ChangeState::Unchanged);
			Query getParent1Query{_watcher};
			getParent1Query.prepare(QStringLiteral("SELECT Value "
												   "FROM RefTestParent1 "
												   "WHERE Key == ?;"));
			getParent1Query.addBindValue(0);
			getParent1Query.exec();
			QVERIFY(getParent1Query.next());
			QCOMPARE(getParent1Query.value(0).toDouble(), 0.5);
			QVERIFY(!getParent1Query.next());

			// parent 2 was added as default
			Query getParent2Query{_watcher};
			getParent2Query.prepare(QStringLiteral("SELECT Value "
												   "FROM RefTestParent2 "
												   "WHERE Key == ?;"));
			getParent2Query.addBindValue(0);
			getParent2Query.exec();
			QVERIFY(getParent2Query.next());
			QVERIFY(getParent2Query.value(0).isNull());
			QVERIFY(!getParent2Query.next());
			// should not have a changed entry
			Query testChangedQuery{_watcher};
			testChangedQuery.prepare(QStringLiteral("SELECT tstamp, changed "
													"FROM __qtdatasync_sync_data_RefTestParent2 "
													"WHERE pkey == ?;"));
			testChangedQuery.addBindValue(0);
			testChangedQuery.exec();
			QVERIFY(!testChangedQuery.next());

			// child was added
			Query getChildQuery{_watcher};
			getChildQuery.prepare(QStringLiteral("SELECT Key1, Key2, Value "
												 "FROM RefTestChild "
												 "WHERE Key1 == ? AND Key2 == ?"));
			getChildQuery.addBindValue(0);
			getChildQuery.addBindValue(0);
			getChildQuery.exec();
			QVERIFY(getChildQuery.next());
			QCOMPARE(getChildQuery.value(0).toInt(), 0);
			QCOMPARE(getChildQuery.value(1).toInt(), 0);
			QCOMPARE(getChildQuery.value(2).toString(), QStringLiteral("baum"));
			QVERIFY(!getChildQuery.next());
			VERIFY_CHANGED_STATE_KEY(cKey, mod, mod, DatabaseWatcher::ChangeState::Unchanged);
		}

		{
			// unsync table
			_watcher->unsyncTable(child);

			// test refs gone now
			Query rmRefsQuery{_watcher};
			rmRefsQuery.prepare(QStringLiteral("SELECT fKeyTable, fKeyField "
											   "FROM __qtdatasync_sync_references "
											   "WHERE tableName == ?;"));
			rmRefsQuery.addBindValue(child);
			rmRefsQuery.exec();
			QVERIFY(!rmRefsQuery.first());
		}

		// disable referential integrity
		Query pragmaOffQuery{_watcher};
		pragmaOffQuery.exec(QStringLiteral("PRAGMA foreign_keys = OFF;"));
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
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

void DbWatcherTest::markChanged(DatabaseWatcher::ChangeState state, const DatasetId &key)
{
	if (key.key.isEmpty()) {
		Query markChangedQuery{_watcher};
		markChangedQuery.prepare(QStringLiteral("UPDATE %1 "
												"SET changed = ?;")
									 .arg(DatabaseWatcher::TablePrefix + key.tableName));
		markChangedQuery.addBindValue(static_cast<int>(state));
		markChangedQuery.exec();
	} else {
		Query markChangedQuery{_watcher};
		markChangedQuery.prepare(QStringLiteral("UPDATE %1 "
												"SET changed = ? "
												"WHERE pkey == ?;")
									 .arg(DatabaseWatcher::TablePrefix + key.tableName));
		markChangedQuery.addBindValue(static_cast<int>(state));
		markChangedQuery.addBindValue(key.key);
		markChangedQuery.exec();
	}
}

QTEST_MAIN(DbWatcherTest)

#include "tst_dbwatcher.moc"

