#include <QtTest>
#include <QtDataSync/private/remoteconnector_p.h>
#include <array>

#include "anonauth.h"
#include "testlib.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

#define VERIFY_SPY(sigSpy, errorSpy) QVERIFY2(sigSpy.wait(), qUtf8Printable(QStringLiteral("Error on table: ") + errorSpy.value(0).value(0).toString()))

class RemoteConnectorTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testUploadData();
	void testGetChanges();
	void testConflict();
	void testCancel();

	void testLiveSync();
	void testOfflineDetection();

	void testRemoveTable();
	void testRemoveUser();

private:
	static constexpr RemoteConnector::CancallationToken InvalidToken = std::numeric_limits<RemoteConnector::CancallationToken>::max();
	static const QString Table;

	QTemporaryDir _tDir;
	QNetworkAccessManager *_nam = nullptr;
	FirebaseAuthenticator *_authenticator = nullptr;
	RemoteConnector *_connector = nullptr;
};

const QString RemoteConnectorTest::Table = QStringLiteral("RmcTestData");

void RemoteConnectorTest::initTestCase()
{
	qRegisterMetaType<ObjectKey>();  // TODO move to datasync

	try {
		const __private::SetupPrivate::FirebaseConfig config {
			QStringLiteral(FIREBASE_PROJECT_ID),
			QStringLiteral(FIREBASE_API_KEY),
			1min,
			7,
			false
		};

		_nam = new QNetworkAccessManager{this};

		// authenticate
		_authenticator = TestLib::createAuth(config.apiKey, this, _nam);
		QVERIFY(_authenticator);
		auto authRes = TestLib::doAuth(_authenticator);
		QVERIFY(authRes);

		// create rmc
		_connector = new RemoteConnector{
			config,
			_nam,
			this
		};
		_connector->setUser(authRes->first);
		_connector->setIdToken(authRes->second);
		QVERIFY(_connector->isActive());
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void RemoteConnectorTest::cleanupTestCase()
{
	if (_connector) {
		QSignalSpy delSpy{_connector, &RemoteConnector::removedUser};
		_connector->removeUser();
		qDebug() << "deleteDb" << delSpy.wait();
	}

	if (_authenticator)
		TestLib::rmAcc(_authenticator);
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
		const auto token = _connector->uploadChange(data);
		QVERIFY(token != InvalidToken);

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
		const auto token = _connector->uploadChange(data);
		QVERIFY(token != InvalidToken);

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
		const auto token = _connector->uploadChange(data);
		QVERIFY(token != InvalidToken);

		VERIFY_SPY(uploadSpy, errorSpy);
		QCOMPARE(uploadSpy.size(), 1);
		QCOMPARE(uploadSpy[0][0].value<ObjectKey>(), key);
		QCOMPARE(uploadSpy[0][1].value<QDateTime>(), modified);
		QVERIFY(downloadSpy.isEmpty());
		QVERIFY(syncSpy.isEmpty());
		uploadSpy.clear();
	}

	// upload outdated data
	const auto token = _connector->uploadChange({
		Table, QStringLiteral("_0"),
		std::nullopt,
		earlyTime,
		std::nullopt
	});
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(syncSpy, errorSpy);
	QCOMPARE(syncSpy.size(), 1);
	QCOMPARE(syncSpy[0][0].toString(), Table);
	QCOMPARE(downloadSpy.size(), 1);
	QCOMPARE(downloadSpy[0][0].toString(), Table);
	QCOMPARE(downloadSpy[0][1].value<QList<CloudData>>().size(), 1);
	const auto cData = downloadSpy[0][1].value<QList<CloudData>>()[0];
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

	auto token = _connector->getChanges(Table, {});  // get any changes
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(doneSpy, errorSpy);
	QCOMPARE(doneSpy.size(), 1);
	QCOMPARE(downloadSpy.size(), 2);
	QCOMPARE(downloadSpy[0][0].toString(), Table);
	QCOMPARE(downloadSpy[0][1].value<QList<CloudData>>().size(), 7);
	QCOMPARE(downloadSpy[0][0].toString(), Table);
	QCOMPARE(downloadSpy[1][1].value<QList<CloudData>>().size(), 4);
	QCOMPARE(downloadSpy[0][1].value<QList<CloudData>>()[6], downloadSpy[1][1].value<QList<CloudData>>()[0]);

	// combine list, skipping the first
	QList<CloudData> allData;
	allData.append(downloadSpy[0][1].value<QList<CloudData>>());
	allData.append(downloadSpy[1][1].value<QList<CloudData>>().mid(1));

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
	token = _connector->getChanges(Table, nextSync);  // get any changes
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(doneSpy, errorSpy);
	QCOMPARE(doneSpy.size(), 1);
	QCOMPARE(downloadSpy.size(), 1);
	QCOMPARE(downloadSpy[0][0].toString(), Table);
	QCOMPARE(downloadSpy[0][1].value<QList<CloudData>>().size(), 5);

	allData = downloadSpy[0][1].value<QList<CloudData>>();
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
		QDateTime::currentDateTimeUtc(),
		std::nullopt
	};
	const auto token1 = _connector->uploadChange(ulData);
	QVERIFY(token1 != InvalidToken);
	const auto token2 = _connector->uploadChange(ulData);
	QVERIFY(token2 != InvalidToken);
	VERIFY_SPY(uploadSpy, errorSpy);
	if (syncSpy.isEmpty())
		VERIFY_SPY(syncSpy, errorSpy);
	else
		QVERIFY(errorSpy.isEmpty());
}

void RemoteConnectorTest::testCancel()
{
	QSignalSpy uploadSpy{_connector, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy.isValid());
	QSignalSpy downloadSpy{_connector, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy.isValid());
	QSignalSpy syncSpy{_connector, &RemoteConnector::triggerSync};
	QVERIFY(syncSpy.isValid());
	QSignalSpy doneSpy{_connector, &RemoteConnector::syncDone};
	QVERIFY(doneSpy.isValid());
	QSignalSpy errorSpy{_connector, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	// cancel download
	auto token = _connector->getChanges(Table, QDateTime{});
	QVERIFY(token != InvalidToken);
	_connector->cancel(token);
	QVERIFY(!downloadSpy.wait());

	// cancel upload
	token = _connector->uploadChange({
		Table, QStringLiteral("_key"),
		std::nullopt,
		QDateTime::currentDateTimeUtc(),
		std::nullopt
	});
	QVERIFY(token != InvalidToken);
	_connector->cancel(token);
	QVERIFY(!uploadSpy.wait());

	QVERIFY(downloadSpy.isEmpty());
	QVERIFY(uploadSpy.isEmpty());
	QVERIFY(syncSpy.isEmpty());
	QVERIFY(doneSpy.isEmpty());
	QVERIFY(errorSpy.isEmpty());
}

void RemoteConnectorTest::testLiveSync()
{
	QSignalSpy downloadSpy{_connector, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy.isValid());
	QSignalSpy doneSpy{_connector, &RemoteConnector::syncDone};
	QVERIFY(doneSpy.isValid());
	QSignalSpy uploadSpy{_connector, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy.isValid());
	QSignalSpy errorSpy{_connector, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	// upload changes since tstamp to test as the sync
	const auto tStamp = QDateTime::currentDateTimeUtc();
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
		const auto token = _connector->uploadChange(data);
		QVERIFY(token != InvalidToken);

		VERIFY_SPY(uploadSpy, errorSpy);
		QCOMPARE(uploadSpy.size(), 1);
		QCOMPARE(uploadSpy[0][0].value<ObjectKey>(), key);
		QCOMPARE(uploadSpy[0][1].value<QDateTime>(), modified);
		QVERIFY(downloadSpy.isEmpty());
		uploadSpy.clear();
	}

	// start live sync since tstamp and verify data
	const auto liveToken = _connector->startLiveSync(Table, tStamp);
	QVERIFY(liveToken != InvalidToken);
	VERIFY_SPY(doneSpy, errorSpy);
	QCOMPARE(doneSpy.size(), 1);  // ignores paging limit
	QCOMPARE(downloadSpy.size(), 1);
	QCOMPARE(downloadSpy[0][0].toString(), Table);
	const auto allData = downloadSpy[0][1].value<QList<CloudData>>();
	QCOMPARE(allData.size(), 10);
	for (auto i = 0; i < allData.size(); ++i) {
		const auto &data = allData[i];
		if (i > 0)
			QVERIFY(data.uploaded() >= allData[i - 1].uploaded());
		QCOMPARE(data.key().typeName, Table);
		QCOMPARE(data.key().id, QStringLiteral("_%1").arg(i));
		const QJsonObject resData {
			{QStringLiteral("Key"), i},
			{QStringLiteral("Value"), QStringLiteral("data_%1").arg(i)}
		};
		QCOMPARE(data.data(), resData);
		if (i > 0)
			QVERIFY(data.modified() >= allData[i - 1].modified());
	}
	downloadSpy.clear();
	doneSpy.clear();

	// do an upload -> should trigger a live sync event
	CloudData data {
		Table, QStringLiteral("_key"),
		std::nullopt,
		QDateTime::currentDateTimeUtc(),
		std::nullopt
	};
	const auto token = _connector->uploadChange(data);
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(uploadSpy, errorSpy);
	if (downloadSpy.isEmpty())
		VERIFY_SPY(downloadSpy, errorSpy);
	else
		QVERIFY(errorSpy.isEmpty());
	QCOMPARE(downloadSpy.size(), 1);
	QCOMPARE(downloadSpy[0][0].toString(), Table);
	QCOMPARE(downloadSpy[0][1].value<QList<CloudData>>().size(), 1);
	const auto dlData = downloadSpy[0][1].value<QList<CloudData>>()[0];
	QCOMPARE(dlData.key(), data.key());
	QCOMPARE(dlData.data(), data.data());
	QCOMPARE(dlData.modified(), data.modified());
	QVERIFY(doneSpy.isEmpty());

	// stop and do again -> nothing
	data.setModified(QDateTime::currentDateTimeUtc());
	_connector->cancel(liveToken);
	_connector->uploadChange(data);
	VERIFY_SPY(uploadSpy, errorSpy);
	QVERIFY(!downloadSpy.wait());
}

void RemoteConnectorTest::testOfflineDetection()
{
	QSignalSpy onlineSpy{_connector, &RemoteConnector::onlineChanged};
	QVERIFY(onlineSpy.isValid());

	QCOMPARE(_connector->isOnline(), true);

	_nam->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
	QTRY_COMPARE(onlineSpy.size(), 1);
	QCOMPARE(onlineSpy[0][0].toBool(), false);
	QCOMPARE(_connector->isOnline(), false);

	_nam->setNetworkAccessible(QNetworkAccessManager::Accessible);
	QTRY_COMPARE(onlineSpy.size(), 2);
	QCOMPARE(onlineSpy[1][0].toBool(), true);
	QCOMPARE(_connector->isOnline(), true);
}

void RemoteConnectorTest::testRemoveTable()
{
	QSignalSpy removedSpy{_connector, &RemoteConnector::removedTable};
	QVERIFY(removedSpy.isValid());
	QSignalSpy errorSpy{_connector, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	auto token = _connector->removeTable(Table);
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(removedSpy, errorSpy);
	QCOMPARE(removedSpy.size(), 1);
	QCOMPARE(removedSpy[0][0].toString(), Table);

	// get changes must be empty now
	QSignalSpy doneSpy{_connector, &RemoteConnector::syncDone};
	QVERIFY(doneSpy.isValid());
	QSignalSpy downloadSpy{_connector, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy.isValid());

	token = _connector->getChanges(Table, {});
	QVERIFY(token != InvalidToken);
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

	auto token = _connector->uploadChange({
		Table, QStringLiteral("_key"),
		std::nullopt,
		QDateTime::currentDateTimeUtc(),
		std::nullopt
	});
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(uploadSpy, errorSpy);
	QCOMPARE(uploadSpy.size(), 1);

	QSignalSpy removedSpy{_connector, &RemoteConnector::removedUser};
	QVERIFY(removedSpy.isValid());

	token = _connector->removeUser();
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(removedSpy, errorSpy);
	QCOMPARE(removedSpy.size(), 1);

	// get changes must throw an error
	token = _connector->getChanges(Table, {});
	QCOMPARE(token,  InvalidToken);
	if (errorSpy.isEmpty())
		QVERIFY(errorSpy.wait());
	QCOMPARE(errorSpy.size(), 1);
}

QTEST_MAIN(RemoteConnectorTest)

#include "tst_remoteconnector.moc"


