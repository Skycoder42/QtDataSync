#include <QtTest>
#include <QtDataSync>

#include <QtDataSync/private/engine_p.h>
#include <QtDataSync/private/remoteconnector_p.h>

#include "anonauth.h"
using namespace QtDataSync;

#define VERIFY_SPY(sigSpy, errorSpy) QVERIFY2(sigSpy.wait(), qUtf8Printable(errorSpy.value(0).value(0).toString()))

class Extractor : public Engine
{
public:
	static EnginePrivate *extract(Engine *engine) {
		return static_cast<Extractor*>(engine)->d_func();
	}

	EnginePrivate *d_func() {
		Q_CAST_IGNORE_ALIGN(return reinterpret_cast<EnginePrivate*>(qGetPtrHelper(d_ptr)););
	}
};

class RemoteConnectorTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testUploadData();
	void testGetChanges();
	void testConflict();

	void testLiveSync();

	void testRemoveTable();
	void testRemoveUser();

private:
	static const QString Table;

	QTemporaryDir _tDir;
	Engine *_engine = nullptr;
	QPointer<RemoteConnector> _connector;

	void doAuth();
};

const QString RemoteConnectorTest::Table = QStringLiteral("RmcTestData");

void RemoteConnectorTest::initTestCase()
{
	qRegisterMetaType<ObjectKey>();  // TODO move to datasync

	try {
		_engine = Setup::fromConfig(QStringLiteral(SRCDIR "../../../ci/web-config.json"), Setup::ConfigType::WebConfig)
					  .setAuthenticator(new AnonAuth{})
					  .setSettings(new QSettings{_tDir.filePath(QStringLiteral("config.ini")), QSettings::IniFormat})
					  .setRemotePageLimit(7)  // force small pages to test paging
					  .createEngine(this);
		_connector = Extractor::extract(_engine)->connector;
		_connector->disconnect(_engine);
		doAuth();
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void RemoteConnectorTest::cleanupTestCase()
{
	if (_connector) {
		QSignalSpy delSpy{_connector, &RemoteConnector::removedUser};
		_connector->removeUser();
		qDebug() << "deleteTable" << delSpy.wait();

		QSignalSpy accDelSpy{_engine->authenticator(), &IAuthenticator::accountDeleted};
		_engine->authenticator()->deleteUser();
		if (accDelSpy.wait())
			qDebug() << "deleteAcc" << accDelSpy[0][0].toBool();
		else
			qDebug() << "deleteAcc did not fire";
	}

	_connector.clear();
	if (_engine)
		_engine->deleteLater();
}

void RemoteConnectorTest::testUploadData()
{
	QSignalSpy uploadSpy{_connector, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy.isValid());
	QSignalSpy downloadSpy{_connector, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy.isValid());
	QSignalSpy syncSpy{_connector, &RemoteConnector::triggerSync};
	QVERIFY(syncSpy.isValid());
	QSignalSpy errorSpy{_connector, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	const auto earlyTime = QDateTime::currentDateTimeUtc().addSecs(-5);
	// upload unchanged data
	for (auto i = 0; i < 10; ++i) {
		const ObjectKey key {Table, QStringLiteral("_%1").arg(i)};
		const auto modified = QDateTime::currentDateTimeUtc();
		CloudData data;
		data.setKey(key);
		data.setData(QJsonObject {
			{QStringLiteral("Key"), i},
			{QStringLiteral("Value"), QStringLiteral("data_%1").arg(i)}
		});
		data.setModified(modified);
		_connector->uploadChange(data);

		VERIFY_SPY(uploadSpy, errorSpy);
		QCOMPARE(uploadSpy.size(), 1);
		QCOMPARE(uploadSpy[0][0].value<ObjectKey>(), key);
		QCOMPARE(uploadSpy[0][1].value<QDateTime>(), modified);
		QVERIFY(downloadSpy.isEmpty());
		QVERIFY(syncSpy.isEmpty());
		uploadSpy.clear();
	}

	// replace data
	for (auto i = 0; i < 10; i += 2) {
		const ObjectKey key {Table, QStringLiteral("_%1").arg(i)};
		const auto modified = QDateTime::currentDateTimeUtc();
		CloudData data;
		data.setKey(key);
		data.setData(QJsonObject {
			{QStringLiteral("Key"), i},
			{QStringLiteral("Value"), QStringLiteral("data_%1").arg(i*i + 2)}
		});
		data.setModified(modified);
		_connector->uploadChange(data);

		VERIFY_SPY(uploadSpy, errorSpy);
		QCOMPARE(uploadSpy.size(), 1);
		QCOMPARE(uploadSpy[0][0].value<ObjectKey>(), key);
		QCOMPARE(uploadSpy[0][1].value<QDateTime>(), modified);
		QVERIFY(downloadSpy.isEmpty());
		QVERIFY(syncSpy.isEmpty());
		uploadSpy.clear();
	}

	// delete data
	for (auto i = 1; i < 10; i += 2) {
		const ObjectKey key {Table, QStringLiteral("_%1").arg(i)};
		const auto modified = QDateTime::currentDateTimeUtc();
		CloudData data;
		data.setKey(key);
		data.setData(std::nullopt);
		data.setModified(modified);
		_connector->uploadChange(data);

		VERIFY_SPY(uploadSpy, errorSpy);
		QCOMPARE(uploadSpy.size(), 1);
		QCOMPARE(uploadSpy[0][0].value<ObjectKey>(), key);
		QCOMPARE(uploadSpy[0][1].value<QDateTime>(), modified);
		QVERIFY(downloadSpy.isEmpty());
		QVERIFY(syncSpy.isEmpty());
		uploadSpy.clear();
	}

	// upload outdated data
	_connector->uploadChange({
		Table, QStringLiteral("_0"),
		std::nullopt,
		earlyTime
	});
	VERIFY_SPY(syncSpy, errorSpy);
	QCOMPARE(syncSpy.size(), 1);
	QCOMPARE(syncSpy[0][0].toString(), Table);
	QCOMPARE(downloadSpy.size(), 1);
	QCOMPARE(downloadSpy[0][0].value<QList<CloudData>>().size(), 1);
	QCOMPARE(downloadSpy[0][1].toBool(), false);
	const auto cData = downloadSpy[0][0].value<QList<CloudData>>()[0];
	QCOMPARE(cData.key().typeName, Table);
	QCOMPARE(cData.key().id, QStringLiteral("_0"));
	const QJsonObject oldData {
		{QStringLiteral("Key"), 0},
		{QStringLiteral("Value"), QStringLiteral("data_2")}
	};
	QCOMPARE(cData.data(), oldData);
	QVERIFY(cData.modified() > earlyTime);
	QVERIFY(!uploadSpy.wait());
}

void RemoteConnectorTest::testGetChanges()
{
	QSignalSpy doneSpy{_connector, &RemoteConnector::syncDone};
	QVERIFY(doneSpy.isValid());
	QSignalSpy downloadSpy{_connector, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy.isValid());
	QSignalSpy errorSpy{_connector, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	_connector->getChanges(Table, {});  // get any changes
	VERIFY_SPY(doneSpy, errorSpy);
	QCOMPARE(doneSpy.size(), 1);
	QCOMPARE(downloadSpy.size(), 2);
	QCOMPARE(downloadSpy[0][0].value<QList<CloudData>>().size(), 7);
	QCOMPARE(downloadSpy[0][1].toBool(), false);
	QCOMPARE(downloadSpy[1][0].value<QList<CloudData>>().size(), 4);
	QCOMPARE(downloadSpy[1][1].toBool(), false);
	QCOMPARE(downloadSpy[0][0].value<QList<CloudData>>()[6], downloadSpy[1][0].value<QList<CloudData>>()[0]);

	// combine list, skipping the first
	QList<CloudData> allData;
	allData.append(downloadSpy[0][0].value<QList<CloudData>>());
	allData.append(downloadSpy[1][0].value<QList<CloudData>>().mid(1));

	// verify downloaded data and order
	QDateTime nextSync;
	const std::array<int, 10> keys{0, 2, 4, 6, 8, 1, 3, 5, 7, 9};
	for (auto i = 0; i < allData.size(); ++i) {
		const auto key = keys[i];
		qDebug() << "index" << i << "key" << key;
		const auto &data = allData[i];
		if (i > 0)
			QVERIFY(data.uploaded() >= allData[i -1].uploaded());
		QCOMPARE(data.key().typeName, Table);
		QCOMPARE(data.key().id, QStringLiteral("_%1").arg(key));
		if (key % 2 == 0) {
			const QJsonObject resData {
				{QStringLiteral("Key"), key},
				{QStringLiteral("Value"), QStringLiteral("data_%1").arg(key*key + 2)}
			};
			QCOMPARE(data.data(), resData);
		} else
			QVERIFY(!data.data());
		if (i % 5 != 0)
			QVERIFY(data.modified() >= allData[i - 1].modified());
		if (i == 5)
			nextSync = data.uploaded();
	}

	// test again, with timestamp
	doneSpy.clear();
	downloadSpy.clear();
	_connector->getChanges(Table, nextSync);  // get any changes
	VERIFY_SPY(doneSpy, errorSpy);
	QCOMPARE(doneSpy.size(), 1);
	QCOMPARE(downloadSpy.size(), 1);
	QCOMPARE(downloadSpy[0][0].value<QList<CloudData>>().size(), 5);
	QCOMPARE(downloadSpy[0][1].toBool(), false);

	allData = downloadSpy[0][0].value<QList<CloudData>>();
	for (auto i = 0; i < allData.size(); ++i) {
		const auto key = keys[i + 5];
		qDebug() << "index" << i << "key" << key;
		const auto &data = allData[i];
		QCOMPARE(data.key().typeName, Table);
		QCOMPARE(data.key().id, QStringLiteral("_%1").arg(key));
	}
}

void RemoteConnectorTest::testConflict()
{
	QSignalSpy uploadSpy{_connector, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy.isValid());
	QSignalSpy syncSpy{_connector, &RemoteConnector::triggerSync};
	QVERIFY(syncSpy.isValid());
	QSignalSpy errorSpy{_connector, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	CloudData ulData {
		Table, QStringLiteral("_key"),
		std::nullopt,
		QDateTime::currentDateTimeUtc()
	};
	_connector->uploadChange(ulData);
	_connector->uploadChange(ulData);
	VERIFY_SPY(uploadSpy, errorSpy);
	if (syncSpy.isEmpty())
		VERIFY_SPY(syncSpy, errorSpy);
	else
		QVERIFY2(errorSpy.isEmpty(), qUtf8Printable(errorSpy.value(0).value(0).toString()));
}

void RemoteConnectorTest::testLiveSync()
{
	QSignalSpy downloadSpy{_connector, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy.isValid());
	QSignalSpy uploadSpy{_connector, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy.isValid());
	QSignalSpy errorSpy{_connector, &RemoteConnector::liveSyncError};
	QVERIFY(errorSpy.isValid());

	_connector->startLiveSync();
	QVERIFY(!errorSpy.wait());

	// do an upload -> should trigger a live sync event
	CloudData data {
		Table, QStringLiteral("_key"),
		std::nullopt,
		QDateTime::currentDateTimeUtc()
	};
	_connector->uploadChange(data);
	VERIFY_SPY(uploadSpy, errorSpy);
	if (downloadSpy.isEmpty())
		VERIFY_SPY(downloadSpy, errorSpy);
	else
		QVERIFY2(errorSpy.isEmpty(), qUtf8Printable(errorSpy.value(0).value(0).toString()));
	QCOMPARE(downloadSpy.size(), 1);
	QCOMPARE(downloadSpy[0][0].value<QList<CloudData>>().size(), 1);
	const auto dlData = downloadSpy[0][0].value<QList<CloudData>>()[0];
	QCOMPARE(dlData.key(), data.key());
	QCOMPARE(dlData.data(), data.data());
	QCOMPARE(dlData.modified(), data.modified());
	QCOMPARE(downloadSpy[0][1].toBool(), true);

	// stop and do again -> nothing
	data.setModified(QDateTime::currentDateTimeUtc());
	_connector->stopLiveSync();
	_connector->uploadChange(data);
	VERIFY_SPY(uploadSpy, errorSpy);
	QVERIFY(!downloadSpy.wait());
}

void RemoteConnectorTest::testRemoveTable()
{
	QSignalSpy removedSpy{_connector, &RemoteConnector::removedTable};
	QVERIFY(removedSpy.isValid());
	QSignalSpy errorSpy{_connector, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	_connector->removeTable(Table);
	VERIFY_SPY(removedSpy, errorSpy);
	QCOMPARE(removedSpy.size(), 1);
	QCOMPARE(removedSpy[0][0].toString(), Table);

	// get changes must be empty now
	QSignalSpy doneSpy{_connector, &RemoteConnector::syncDone};
	QVERIFY(doneSpy.isValid());
	QSignalSpy downloadSpy{_connector, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy.isValid());

	_connector->getChanges(Table, {});
	VERIFY_SPY(doneSpy, errorSpy);
	QCOMPARE(doneSpy.size(), 1);
	QCOMPARE(downloadSpy.size(), 0);
}

void RemoteConnectorTest::testRemoveUser()
{
	// add data again, to have something to remove
	QSignalSpy uploadSpy{_connector, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy.isValid());
	QSignalSpy errorSpy{_connector, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	_connector->uploadChange({
		Table, QStringLiteral("_key"),
		std::nullopt,
		QDateTime::currentDateTimeUtc(),
	});
	VERIFY_SPY(uploadSpy, errorSpy);
	QCOMPARE(uploadSpy.size(), 1);

	QSignalSpy removedSpy{_connector, &RemoteConnector::removedUser};
	QVERIFY(removedSpy.isValid());

	_connector->removeUser();
	VERIFY_SPY(removedSpy, errorSpy);
	QCOMPARE(removedSpy.size(), 1);

	// get changes must throw an error
	_connector->getChanges(Table, {});
	if (errorSpy.isEmpty())
		QVERIFY(errorSpy.wait());
	QCOMPARE(errorSpy.size(), 1);
}

void RemoteConnectorTest::doAuth()
{
	auto auth = _engine->authenticator();
	QSignalSpy signInSpy{auth, &IAuthenticator::signInSuccessful};
	QVERIFY(signInSpy.isValid());
	QSignalSpy errorSpy{auth, &IAuthenticator::signInFailed};
	QVERIFY(errorSpy.isValid());

	auth->signIn();
	QVERIFY(signInSpy.wait());
	QCOMPARE(signInSpy.size(), 1);
	QCOMPARE(errorSpy.size(), 0);

	_connector->setUser(signInSpy[0][0].toString());
	_connector->setIdToken(signInSpy[0][1].toString());
	QVERIFY(_connector->isActive());
}

QTEST_MAIN(RemoteConnectorTest)

#include "tst_remoteconnector.moc"


