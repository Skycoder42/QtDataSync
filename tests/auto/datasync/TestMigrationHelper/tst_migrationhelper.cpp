#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <QtDataSync/private/localstore_p.h>
#include <QtDataSync/private/defaults_p.h>
#include <QtDataSync/private/remoteconnector_p.h>
using namespace QtDataSync;

using std::get;

class TestMigrationHelper : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testMigration_data();
	void testMigration();

private:
	MigrationHelper *helper;
	LocalStore *store;

	QSharedPointer<QTemporaryDir> createTestData();
};

void TestMigrationHelper::initTestCase()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
	try {
		//QLoggingCategory::setFilterRules(QStringLiteral("qtdatasync.default.migration.debug=true"));
		TestLib::init();
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		helper = new MigrationHelper(this);
		store = new LocalStore(DefaultsPrivate::obtainDefaults(DefaultSetup), this);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestMigrationHelper::cleanupTestCase()
{
	delete helper;
	helper = nullptr;
	delete store;
	store = nullptr;
	Setup::removeSetup(DefaultSetup, true);
}

void TestMigrationHelper::testMigration_data()
{
	QTest::addColumn<MigrationHelper::MigrationFlags>("flags");

	//try useful flag combinations
	QTest::newRow("data_only") << MigrationHelper::MigrationFlags(MigrationHelper::MigrateData);
	QTest::newRow("changed_data_only") << (MigrationHelper::MigrateData | MigrationHelper::MigrateChanged);
	QTest::newRow("config_only") << MigrationHelper::MigrationFlags(MigrationHelper::MigrateRemoteConfig);
	QTest::newRow("all") << (MigrationHelper::MigrateData | MigrationHelper::MigrateChanged | MigrationHelper::MigrateRemoteConfig);
	QTest::newRow("all_cleanup") << (MigrationHelper::MigrateData | MigrationHelper::MigrateChanged | MigrationHelper::MigrateRemoteConfig | MigrationHelper::MigrateWithCleanup);

//	QTest::newRow("data_no_overwrite") << MigrationHelper::MigrationFlags(MigrationHelper::MigrateData)
//									   << true;
//	QTest::newRow("data_overwrite") << (MigrationHelper::MigrateData | MigrationHelper::MigrateOverwriteData)
//									<< true;
//	QTest::newRow("config_no_overwrite") << MigrationHelper::MigrationFlags(MigrationHelper::MigrateRemoteConfig)
//										 << true;
//	QTest::newRow("config_overwrite") << (MigrationHelper::MigrateRemoteConfig | MigrationHelper::MigrateOverwriteConfig)
//									  << true;
}

void TestMigrationHelper::testMigration()
{
	QFETCH(MigrationHelper::MigrationFlags, flags);

	try {
		auto config = Defaults(DefaultsPrivate::obtainDefaults(DefaultSetup)).createSettings(this, QStringLiteral("connector"));
		config->clear();
		store->reset(false);

		//prepare data
		auto tDir = createTestData();
		QVERIFY(tDir->isValid());

		QSignalSpy maxSpy(helper, &MigrationHelper::migrationPrepared);
		QSignalSpy progSpy(helper, &MigrationHelper::migrationProgress);
		QSignalSpy doneSpy(helper, &MigrationHelper::migrationDone);

		//perform the migration
		helper->startMigration(tDir->path(), flags);
		QVERIFY(doneSpy.wait(15000));
		QCOMPARE(doneSpy.size(), 1);
		QVERIFY(doneSpy.takeFirst()[0].toBool());

		// check the spies
		QCOMPARE(maxSpy.size(), 1);
		QCOMPARE(progSpy.size(), maxSpy.first()[0].toInt());
		QCOMPARE(progSpy.last()[0].toInt(), maxSpy.first()[0].toInt());

		//verify the config
		if(flags.testFlag(MigrationHelper::MigrateRemoteConfig)) {
			QCOMPARE(config->value(RemoteConnector::keyRemoteEnabled).toBool(), true);
			QCOMPARE(config->value(RemoteConnector::keyRemoteUrl).toUrl(), QUrl(QStringLiteral("wss://random.url/app")));
			QCOMPARE(config->value(RemoteConnector::keyRemoteAccessKey).toString(), QStringLiteral("the-secret"));
			QCOMPARE(config->value(RemoteConnector::keyDeviceName).toString(), QStringLiteral("new-name"));
			config->beginGroup(RemoteConnector::keyRemoteHeaders);
			QCOMPARE(config->childKeys().size(), 2);
			QCOMPARE(config->value(QStringLiteral("h1")).toInt(), 10);
			QCOMPARE(config->value(QStringLiteral("h2")).toBool(), true);
			config->endGroup();
		} else {
			QCOMPARE(config->value(RemoteConnector::keyRemoteEnabled).toBool(), false);
			QCOMPARE(config->value(RemoteConnector::keyRemoteUrl).toUrl(), QUrl());
			QCOMPARE(config->value(RemoteConnector::keyRemoteAccessKey).toString(), QString());
			QCOMPARE(config->value(RemoteConnector::keyDeviceName).toString(), QString());
			config->beginGroup(RemoteConnector::keyRemoteHeaders);
			QCOMPARE(config->childKeys().size(), 0);
			config->endGroup();
		}
		config->clear();
		config->deleteLater();

		//verify the data
		if(flags.testFlag(MigrationHelper::MigrateData)) {
			QCOMPARE(store->count(TestLib::TypeName), (111u / 3u) * 2u);
			QCOMPARE(store->changeCount(), (111u / 3u) * 2u); //no matter if migrate changed or not
			//verify the actual data
			for(auto i = 0; i < 111; i++) {
				auto key = TestLib::generateKey(i);
				if(i % 3 == 0) {
					QVERIFY_EXCEPTION_THROWN(store->load(key), NoDataException);
					auto scope = store->startSync(key);
					auto info = store->loadChangeInfo(scope);
					if(flags.testFlag(MigrationHelper::MigrateChanged)) {
						QCOMPARE(get<0>(info), LocalStore::ExistsDeleted);
						QCOMPARE(get<1>(info), 1u);
						QVERIFY(get<2>(info).isEmpty());
					} else
						QCOMPARE(get<0>(info), LocalStore::NoExists);
					store->commitSync(scope);
				} else {
					auto cData = store->load(key);
					QCOMPARE(cData, TestLib::generateDataJson(i));
					auto scope = store->startSync(key);
					auto info = store->loadChangeInfo(scope);
					QCOMPARE(get<0>(info), LocalStore::Exists);
					QCOMPARE(get<1>(info), 1u);
					QVERIFY(!get<2>(info).isEmpty());
					store->commitSync(scope);
				}
			}
		} else {
			QCOMPARE(store->count(TestLib::TypeName), 0u);
			QCOMPARE(store->changeCount(), 0u);
		}

		// test cleanup
		if(flags.testFlag(MigrationHelper::MigrateWithCleanup)) {
			QDir dir(tDir->path());
			if(dir.exists())
				QVERIFY(dir.entryList().isEmpty());
		}
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

QSharedPointer<QTemporaryDir> TestMigrationHelper::createTestData()
{
	QSharedPointer<QTemporaryDir> mDir;
	[&]() {
		auto tDir = QSharedPointer<QTemporaryDir>::create();
		qDebug() << "migration dir:" << tDir->path();
		QDir dir(tDir->path());

		//create the config
		QSettings config(dir.absoluteFilePath(QStringLiteral("config.ini")), QSettings::IniFormat);
		config.beginGroup(QStringLiteral("RemoteConnector"));
		config.setValue(QStringLiteral("remoteEnabled"), true);
		config.setValue(QStringLiteral("remoteUrl"), QUrl(QStringLiteral("wss://random.url/app")));
		config.setValue(QStringLiteral("sharedSecret"), QStringLiteral("the-secret"));
		config.beginGroup(QStringLiteral("headers"));
		config.setValue(QStringLiteral("h1"), 10);
		config.setValue(QStringLiteral("h2"), true);
		config.endGroup();
		config.endGroup();
		config.setValue(QStringLiteral("NetworkExchange/name"), QStringLiteral("new-name"));
		config.sync();

		//create the data
		auto name = QStringLiteral("create_migrate");
		auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
		db.setDatabaseName(dir.absoluteFilePath(QStringLiteral("store.db")));
		QVERIFY2(db.open(), qUtf8Printable(db.lastError().text()));

		//create tables
		QSqlQuery indexQuery(db);
		indexQuery.prepare(QStringLiteral("CREATE TABLE DataIndex ( "
										  "		Type	TEXT NOT NULL, "
										  "		Key		TEXT NOT NULL, "
										  "		File	TEXT NOT NULL, "
										  "		PRIMARY KEY(Type, Key) "
										  ")"));
		QVERIFY2(indexQuery.exec(), qUtf8Printable(indexQuery.lastError().text()));
		QSqlQuery syncQuery(db);
		syncQuery.prepare(QStringLiteral("CREATE TABLE SyncState ("
										 "	Type	TEXT NOT NULL,"
										 "	Key		TEXT NOT NULL,"
										 "	Changed	INTEGER NOT NULL CHECK(Changed >= 0 AND Changed <= 2),"
										 "	PRIMARY KEY(Type, Key)"
										 ");"));
		QVERIFY2(syncQuery.exec(), qUtf8Printable(syncQuery.lastError().text()));

		auto dirPath = QString::fromUtf8("store/_" + TestLib::TypeName.toHex());
		QVERIFY(dir.mkpath(dirPath));
		QVERIFY(dir.cd(dirPath));
		for(auto i = 0; i < 111; i++) {
			auto key = TestLib::generateKey(i);
			if(i % 3 == 0) {
				// deleted changed
				QSqlQuery query(db);
				query.prepare(QStringLiteral("INSERT INTO SyncState "
											 "(Type, Key, Changed) "
											 "VALUES(?, ?, ?)"));
				query.addBindValue(key.typeName);
				query.addBindValue(key.id);
				query.addBindValue(2);
				QVERIFY2(query.exec(), qUtf8Printable(query.lastError().text()));
			} else {
				// entry
				QFile jFile(dir.absoluteFilePath(QStringLiteral("%1.dat").arg(i)));
				jFile.open(QIODevice::WriteOnly);
				jFile.write(QJsonDocument(TestLib::generateDataJson(i)).toBinaryData());
				jFile.close();

				QSqlQuery query(db);
				query.prepare(QStringLiteral("INSERT INTO DataIndex "
											 "(Type, Key, File) "
											 "VALUES(?, ?, ?)"));
				query.addBindValue(key.typeName);
				query.addBindValue(key.id);
				query.addBindValue(QString::number(i));
				QVERIFY2(query.exec(), qUtf8Printable(query.lastError().text()));

				if(i % 3 == 1) {
					QSqlQuery query(db);
					query.prepare(QStringLiteral("INSERT INTO SyncState "
												 "(Type, Key, Changed) "
												 "VALUES(?, ?, ?)"));
					query.addBindValue(key.typeName);
					query.addBindValue(key.id);
					query.addBindValue(1);
					QVERIFY2(query.exec(), qUtf8Printable(query.lastError().text()));
				}
			}
		}

		db.close();
		db = QSqlDatabase();
		QSqlDatabase::removeDatabase(name);

		mDir = tDir;
	}();

	return mDir;
}

QTEST_MAIN(TestMigrationHelper)

#include "tst_migrationhelper.moc"
