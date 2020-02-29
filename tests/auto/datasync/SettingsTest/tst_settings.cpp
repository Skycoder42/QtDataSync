#include <QtTest>
#include <QtDataSync/Engine>
#include <QtDataSync/Setup>
#include <QtDataSync/private/settingsadaptor_p.h>
#include <QtDataSync/private/databasewatcher_p.h>
#include "anonauth.h"
#include "testlib.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

struct Data
{
	int value;

	inline operator QVariant() const {
		return QVariant::fromValue(*this);
	}

	inline friend QDataStream &operator<<(QDataStream &stream, const Data &data) {
		stream << data.value;
		return stream;
	}

	inline friend QDataStream &operator>>(QDataStream &stream, Data &data) {
		stream >> data.value;
		return stream;
	}
};

Q_DECLARE_METATYPE(Data);

class SettingsTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testCreateSettings();
	void testWriteSettings();
	void testReadSettings();

private:
	class Query : public ExQuery {
	public:
		inline Query(QSqlDatabase db) :
			ExQuery{db, DatabaseWatcher::ErrorScope::Database, {}}
		{}
	};

	Engine *_engine = nullptr;
	QSettings *_settings = nullptr;

	QTemporaryDir _dir;
};

void SettingsTest::initTestCase()
{
	qRegisterMetaTypeStreamOperators<Data>();

	try {
		_engine = Setup<AnonAuth>()
					  .setSettings(TestLib::createSettings(this))
					  .createEngine(this);
		QVERIFY(_engine);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void SettingsTest::cleanupTestCase()
{
	try {
		if (_settings)
			_settings->deleteLater();
		if (_engine)
			_engine->deleteLater();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void SettingsTest::testCreateSettings()
{
	try {
		// create db
		auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
		QVERIFY(db.isValid());
		db.setDatabaseName(_dir.filePath(QStringLiteral("test.db")));
		QVERIFY2(db.open(), qUtf8Printable(db.lastError().text()));

		// register settings
		_settings = _engine->syncSettings(false, db, this);
		QVERIFY(_settings);
		QCOMPARE(_settings->status(), QSettings::NoError);
		QVERIFY(QFile::exists(_settings->fileName()));
		QVERIFY(_engine->tables().contains(Engine::DefaultSettingsTableName));
		QVERIFY(_settings->contains(SettingsAdaptor::SettingsInfoConnectionKey));
		QVERIFY(_settings->contains(SettingsAdaptor::SettingsInfoTableKey));
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void SettingsTest::testWriteSettings()
{
	try {
		// recreate settings
		delete _settings;
		_settings = _engine->syncSettings(this);

		// write
		_settings->setValue(QStringLiteral("key0"), true);
		_settings->setValue(QStringLiteral("key1"), 1);
		_settings->beginGroup(QStringLiteral("group0"));
		_settings->setValue(QStringLiteral("key2"), 2.2);
		_settings->beginGroup(QStringLiteral("group1"));
		_settings->setValue(QStringLiteral("key3"), QStringLiteral("3"));
		_settings->endGroup();
		_settings->setValue(QStringLiteral("key4"), QDateTime{{2020, 2, 2}, QTime{22, 22, 22}}.toUTC());
		_settings->endGroup();
		_settings->setValue(QStringLiteral("key5"), Data{42});
		_settings->sync();

		// test all keys are present
		QStringList keys {
			QStringLiteral("key0"),
			QStringLiteral("key1"),
			QStringLiteral("group0/key2"),
			QStringLiteral("group0/group1/key3"),
			QStringLiteral("group0/key4"),
			QStringLiteral("key5"),
		};
		Query testQuery{_engine->database(Engine::DefaultSettingsTableName)};
		testQuery.exec(QStringLiteral("SELECT Key FROM qtdatasync_settings;"));
		while (testQuery.next()) {
			QVERIFY(!keys.isEmpty());
			QVERIFY(keys.removeOne(testQuery.value(0).toString()));
		}
		QVERIFY(keys.isEmpty());


	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void SettingsTest::testReadSettings()
{

}

QTEST_MAIN(SettingsTest)

#include "tst_settings.moc"

