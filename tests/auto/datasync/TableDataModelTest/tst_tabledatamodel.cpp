#include <QtTest>
#include <QtDataSync>
#include "testlib.h"

#include "tablestatemachine_p.h"
#include "tabledatamodel_p.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

#define VERIFY_STATE(machine, state) QCOMPARE(machine->activeStateNames(true), QStringList{QStringLiteral(state)})
#define VERIFY_SPY(sigSpy, errorSpy) QVERIFY2(sigSpy.wait(), qUtf8Printable(QStringLiteral("Error: ") + errorSpy.value(0).value(0).toString()))

class TableDataModelTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testPassivSyncFlow();

private:
	using SyncState = TableDataModel::SyncState;

	static const QString TestTable;

	class Query : public ExQuery {
	public:
		inline Query(QSqlDatabase db) :
			ExQuery{db, DatabaseWatcher::ErrorScope::Database, {}}
		{}
	};

	QSqlDatabase _db;
	FirebaseAuthenticator *_auth = nullptr;
	RemoteConnector *_testCon = nullptr;
	TableStateMachine *_machine = nullptr;
	TableDataModel *_model = nullptr;

	static inline SyncState getState(QSignalSpy &spy) {
		return spy.takeFirst()[0].value<SyncState>();
	}
};

const QString TableDataModelTest::TestTable = QStringLiteral("TableDataModelTest");

void TableDataModelTest::initTestCase()
{
	qRegisterMetaType<SyncState>("SyncState");
	qRegisterMetaType<EnginePrivate::ErrorInfo>("EnginePrivate::ErrorInfo");

	try {
		// stuff
		auto settings = TestLib::createSettings(this);
		auto nam = new QNetworkAccessManager{this};

		// database
		_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
		QVERIFY(_db.isValid());
		_db.setDatabaseName(TestLib::tmpPath(QStringLiteral("test.db")));
		QVERIFY(_db.open());
		Query createQuery{_db};
		createQuery.exec(QStringLiteral("CREATE TABLE TableDataModelTest ( "
										"	Key		INTEGER NOT NULL PRIMARY KEY, "
										"	Value	REAL NOT NULL"
										");"));

		// watcher
		auto watcher = new DatabaseWatcher{std::move(_db), {}, this};
		watcher->addTable(TableConfig{TestTable, _db});

		// fb config
		const __private::SetupPrivate::FirebaseConfig config {
			QStringLiteral(FIREBASE_PROJECT_ID),
			QStringLiteral(FIREBASE_API_KEY),
			1min,
			100,
			false
		};

		// authenticator
		_auth = TestLib::createAuth(config.apiKey, this, nam, settings);
		QVERIFY(_auth);
		const auto credentials = TestLib::doAuth(_auth);
		QVERIFY(credentials);

		// normal rmc
		auto rmc = new RemoteConnector{
			config,
			settings,
			nam,
			std::nullopt,
			this
		};
		rmc->setUser(credentials->first);
		rmc->setIdToken(credentials->second);
		QVERIFY(rmc->isActive());

		// testCon
		auto s2 = TestLib::createSettings(this);
		s2->beginGroup(QStringLiteral("rmc2"));
		_testCon = new RemoteConnector{
			config,
			s2,
			nam,
			std::nullopt,
			this
		};
		_testCon->setUser(credentials->first);
		_testCon->setIdToken(credentials->second);
		QVERIFY(_testCon->isActive());

		// create machine/model
		_machine = new TableStateMachine{this};
		_model = new TableDataModel{_machine};
		_machine->setDataModel(_model);
		QVERIFY(_machine->init());
		_model->setupModel(TestTable,
						   watcher,
						   rmc,
						   new PlainCloudTransformer{this});
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void TableDataModelTest::cleanupTestCase()
{
	if (_testCon) {
		QSignalSpy delSpy{_testCon, &RemoteConnector::removedUser};
		_testCon->removeUser();
		qDebug() << "deleteDb" << delSpy.wait();
	}

	if (_auth)
		TestLib::rmAcc(_auth);
}

void TableDataModelTest::testPassivSyncFlow()
{
	try {
		QSignalSpy stateSpy{_model, &TableDataModel::stateChanged};
		QVERIFY(stateSpy.isValid());
		QSignalSpy exitSpy{_model, &TableDataModel::exited};
		QVERIFY(exitSpy.isValid());
		QSignalSpy errorSpy{_model, &TableDataModel::errorOccured};
		QVERIFY(errorSpy.isValid());

		QSignalSpy rmcDoneSpy{_testCon, &RemoteConnector::syncDone};
		QVERIFY(rmcDoneSpy.isValid());
		QSignalSpy rmcDownloadSpy{_testCon, &RemoteConnector::downloadedData};
		QVERIFY(rmcDownloadSpy.isValid());
		QSignalSpy rmcErrorSpy{_testCon, &RemoteConnector::networkError};
		QVERIFY(rmcErrorSpy.isValid());
		QSignalSpy rmcUploadSpy{_testCon, &RemoteConnector::uploadedData};
		QVERIFY(rmcUploadSpy.isValid());

		QCOMPARE(_model->isLiveSyncEnabled(), false);
		QCOMPARE(_model->state(), SyncState::Disabled);

		// start the machine and let it initialize
		_machine->start();
		QTRY_COMPARE(_model->state(), SyncState::Stopped);
		QCOMPARE(stateSpy.size(), 1);
		QCOMPARE(getState(stateSpy), SyncState::Stopped);

		// start table sync (no data on both sides)
		_model->start();
		QTRY_COMPARE_WITH_TIMEOUT(_model->state(), SyncState::Synchronized, 10000);
		QCOMPARE(stateSpy.size(), 2);
		QCOMPARE(getState(stateSpy), SyncState::Downloading);
		QCOMPARE(getState(stateSpy), SyncState::Synchronized);

		// sync again (still no data)
		_model->triggerSync(false);
		QTRY_COMPARE_WITH_TIMEOUT(stateSpy.size(), 2, 10000);
		QCOMPARE(_model->state(), SyncState::Synchronized);
		QCOMPARE(getState(stateSpy), SyncState::Downloading);
		QCOMPARE(getState(stateSpy), SyncState::Synchronized);

		// trigger fake uplaod (still no data)
		_model->triggerExtUpload();
		QTRY_COMPARE_WITH_TIMEOUT(stateSpy.size(), 1, 10000);
		QCOMPARE(_model->state(), SyncState::Synchronized);
		QCOMPARE(getState(stateSpy), SyncState::Synchronized);

		// add local data to trigger an upload
		for (auto i = 0; i < 5; ++i) {
			Query insertQuery{_db};
			insertQuery.prepare(QStringLiteral("INSERT INTO TableDataModelTest "
											   "(Key, Value) "
											   "VALUES(?, ?)"));
			insertQuery.addBindValue(i);
			insertQuery.addBindValue(i * 0.1);
			insertQuery.exec();
		}
		QTRY_COMPARE_WITH_TIMEOUT(stateSpy.size(), 10, 10000);
		QCOMPARE(_model->state(), SyncState::Synchronized);
		for (auto i = 0; i < 9; ++i)  // 5 triggers (last is 1 upload) + 4 uploads
			QCOMPARE(getState(stateSpy), SyncState::Uploading);
		QCOMPARE(getState(stateSpy), SyncState::Synchronized);

		// verify data via rmc
		auto token = _testCon->getChanges(TestTable, QDateTime{});
		QVERIFY(token != RemoteConnector::InvalidToken);
		VERIFY_SPY(rmcDoneSpy, rmcErrorSpy);
		QCOMPARE(rmcDoneSpy.size(), 1);
		QCOMPARE(rmcDownloadSpy.size(), 1);
		QCOMPARE(rmcDownloadSpy[0][0].toString(), TestTable);
		QCOMPARE(rmcDownloadSpy[0][1].value<QList<CloudData>>().size(), 5);
		// TODO verify actual data
		QVERIFY(rmcErrorSpy.isEmpty());

		// trigger sync -> nothing downloaded
		_model->triggerSync(false);
		QTRY_COMPARE_WITH_TIMEOUT(stateSpy.size(), 2, 10000);
		QCOMPARE(_model->state(), SyncState::Synchronized);
		QCOMPARE(getState(stateSpy), SyncState::Downloading);
		QCOMPARE(getState(stateSpy), SyncState::Synchronized);

		// create changes via rmc -> delete entries 2, 3, 4
		for (auto i = 2; i < 5; ++i) {
			token = _testCon->uploadChange({
				TestTable, QStringLiteral("_%1").arg(i),
				std::nullopt,
				QDateTime::currentDateTimeUtc(),
				std::nullopt
			});
			QVERIFY(token != RemoteConnector::InvalidToken);
			VERIFY_SPY(rmcUploadSpy, rmcErrorSpy);
			QCOMPARE(rmcUploadSpy.size(), 1);
			QVERIFY(rmcErrorSpy.isEmpty());
			rmcUploadSpy.clear();
		}

		// trigger sync -> should download those changes
		_model->triggerSync(false);
		QTRY_COMPARE_WITH_TIMEOUT(stateSpy.size(), 6, 10000);
		QCOMPARE(_model->state(), SyncState::Synchronized);
		QCOMPARE(getState(stateSpy), SyncState::Downloading);
		for (auto i = 0; i < 5; ++i)
			QCOMPARE(getState(stateSpy), SyncState::Synchronized);
		// TODO verify local data

		// trigger sync again -> should only download the one last change -> no effect
		_model->triggerSync(false);
		QTRY_COMPARE_WITH_TIMEOUT(stateSpy.size(), 3, 10000);
		QCOMPARE(_model->state(), SyncState::Synchronized);
		QCOMPARE(getState(stateSpy), SyncState::Downloading);
		QCOMPARE(getState(stateSpy), SyncState::Synchronized);
		QCOMPARE(getState(stateSpy), SyncState::Synchronized);
		// TODO verify local data
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(TableDataModelTest)

#include "tst_tabledatamodel.moc"
