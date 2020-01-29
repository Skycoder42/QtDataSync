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
	void testOutdated();
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
	RemoteConnector *_rmc1 = nullptr;
	RemoteConnector *_rmc2 = nullptr;
};

const QString RemoteConnectorTest::Table = QStringLiteral("RmcTestData");

void RemoteConnectorTest::initTestCase()
{
	qRegisterMetaType<DatasetId>();  // TODO move to datasync

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

		// create rmc1
		auto settings1 = TestLib::createSettings(this);
		settings1->beginGroup(QStringLiteral("rmc1"));
		_rmc1 = new RemoteConnector{
			config,
			_nam,
			settings1,
			this
		};
		_rmc1->setUser(authRes->first);
		_rmc1->setIdToken(authRes->second);
		QVERIFY(_rmc1->isActive());

		// create rmc2
		auto settings2 = TestLib::createSettings(this);
		settings2->beginGroup(QStringLiteral("rmc2"));
		_rmc2 = new RemoteConnector{
			config,
			_nam,
			settings2,
			this
		};
		_rmc2->setUser(authRes->first);
		_rmc2->setIdToken(authRes->second);
		QVERIFY(_rmc2->isActive());
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void RemoteConnectorTest::cleanupTestCase()
{
	if (_rmc2) {
		QSignalSpy delSpy{_rmc2, &RemoteConnector::removedUser};
		_rmc2->removeUser();
		qDebug() << "deleteDb" << delSpy.wait();
	}

	if (_authenticator)
		TestLib::rmAcc(_authenticator);
}

void RemoteConnectorTest::testUploadData()
{
	QSignalSpy uploadSpy1{_rmc1, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy1.isValid());
	QSignalSpy downloadSpy1{_rmc1, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy1.isValid());
	QSignalSpy syncSpy1{_rmc1, &RemoteConnector::triggerSync};
	QVERIFY(syncSpy1.isValid());
	QSignalSpy errorSpy1{_rmc1, &RemoteConnector::networkError};
	QVERIFY(errorSpy1.isValid());

	// upload unchanged data
	for (auto i = 0; i < 10; ++i) {
		const DatasetId key {Table, QStringLiteral("_%1").arg(i)};
		const auto modified = QDateTime::currentDateTimeUtc();
		CloudData data;
		data.setKey(key);
		data.setData(QJsonObject {
			{QStringLiteral("Key"), i},
			{QStringLiteral("Value"), QStringLiteral("data_%1").arg(i)}
		});
		data.setModified(modified);
		const auto token = _rmc1->uploadChange(data);
		QVERIFY(token != InvalidToken);

		VERIFY_SPY(uploadSpy1, errorSpy1);
		QCOMPARE(uploadSpy1.size(), 1);
		QCOMPARE(uploadSpy1[0][0].value<DatasetId>(), key);
		QCOMPARE(uploadSpy1[0][1].toDateTime(), modified);
		QCOMPARE(uploadSpy1[0][2].toBool(), false);
		QVERIFY(downloadSpy1.isEmpty());
		QVERIFY(syncSpy1.isEmpty());
		uploadSpy1.clear();
	}

	// replace data
	for (auto i = 0; i < 10; i += 2) {
		const DatasetId key {Table, QStringLiteral("_%1").arg(i)};
		const auto modified = QDateTime::currentDateTimeUtc();
		CloudData data;
		data.setKey(key);
		data.setData(QJsonObject {
			{QStringLiteral("Key"), i},
			{QStringLiteral("Value"), QStringLiteral("data_%1").arg(i*i + 2)}
		});
		data.setModified(modified);
		const auto token = _rmc1->uploadChange(data);
		QVERIFY(token != InvalidToken);

		VERIFY_SPY(uploadSpy1, errorSpy1);
		QCOMPARE(uploadSpy1.size(), 1);
		QCOMPARE(uploadSpy1[0][0].value<DatasetId>(), key);
		QCOMPARE(uploadSpy1[0][1].toDateTime(), modified);
		QCOMPARE(uploadSpy1[0][2].toBool(), false);
		QVERIFY(downloadSpy1.isEmpty());
		QVERIFY(syncSpy1.isEmpty());
		uploadSpy1.clear();
	}

	// delete data
	for (auto i = 1; i < 10; i += 2) {
		const DatasetId key {Table, QStringLiteral("_%1").arg(i)};
		const auto modified = QDateTime::currentDateTimeUtc();
		CloudData data;
		data.setKey(key);
		data.setData(std::nullopt);
		data.setModified(modified);
		const auto token = _rmc1->uploadChange(data);
		QVERIFY(token != InvalidToken);

		VERIFY_SPY(uploadSpy1, errorSpy1);
		QCOMPARE(uploadSpy1.size(), 1);
		QCOMPARE(uploadSpy1[0][0].value<DatasetId>(), key);
		QCOMPARE(uploadSpy1[0][1].toDateTime(), modified);
		QCOMPARE(uploadSpy1[0][2].toBool(), true);
		QVERIFY(downloadSpy1.isEmpty());
		QVERIFY(syncSpy1.isEmpty());
		uploadSpy1.clear();
	}
}

void RemoteConnectorTest::testGetChanges()
{
	QSignalSpy doneSpy1{_rmc1, &RemoteConnector::syncDone};
	QVERIFY(doneSpy1.isValid());
	QSignalSpy downloadSpy1{_rmc1, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy1.isValid());
	QSignalSpy errorSpy1{_rmc1, &RemoteConnector::networkError};
	QVERIFY(errorSpy1.isValid());

	QSignalSpy doneSpy2{_rmc2, &RemoteConnector::syncDone};
	QVERIFY(doneSpy2.isValid());
	QSignalSpy downloadSpy2{_rmc2, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy2.isValid());
	QSignalSpy errorSpy2{_rmc2, &RemoteConnector::networkError};
	QVERIFY(errorSpy2.isValid());

	// get changes for rmc1 -> should be empty, because skipped
	auto token = _rmc1->getChanges(Table, {});  // get any changes
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(doneSpy1, errorSpy1);
	QCOMPARE(doneSpy1.size(), 1);
	QCOMPARE(downloadSpy1.size(), 0);

	// get changes for rmc2
	token = _rmc2->getChanges(Table, {});  // get any changes
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(doneSpy2, errorSpy2);
	QCOMPARE(doneSpy2.size(), 1);
	QCOMPARE(downloadSpy2.size(), 2);
	QCOMPARE(downloadSpy2[0][0].toString(), Table);
	QCOMPARE(downloadSpy2[0][1].value<QList<CloudData>>().size(), 7);
	QCOMPARE(downloadSpy2[0][0].toString(), Table);
	QCOMPARE(downloadSpy2[1][1].value<QList<CloudData>>().size(), 4);
	QCOMPARE(downloadSpy2[0][1].value<QList<CloudData>>()[6], downloadSpy2[1][1].value<QList<CloudData>>()[0]);

	// combine list, skipping the first
	QList<CloudData> allData;
	allData.append(downloadSpy2[0][1].value<QList<CloudData>>());
	allData.append(downloadSpy2[1][1].value<QList<CloudData>>().mid(1));

	// verify downloaded data and order
	QDateTime nextSync;
	const std::array<int, 10> keys{0, 2, 4, 6, 8, 1, 3, 5, 7, 9};
	for (auto i = 0; i < allData.size(); ++i) {
		const auto key = keys[i];
		qDebug() << "index" << i << "key" << key;
		const auto &data = allData[i];
		if (i > 0)
			QVERIFY(data.uploaded() >= allData[i -1].uploaded());
		QCOMPARE(data.key().tableName, Table);
		QCOMPARE(data.key().key, QStringLiteral("_%1").arg(key));
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
	doneSpy2.clear();
	downloadSpy2.clear();
	token = _rmc2->getChanges(Table, nextSync);  // get any changes
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(doneSpy2, errorSpy2);
	QCOMPARE(doneSpy2.size(), 1);
	QCOMPARE(downloadSpy2.size(), 1);
	QCOMPARE(downloadSpy2[0][0].toString(), Table);
	QCOMPARE(downloadSpy2[0][1].value<QList<CloudData>>().size(), 5);

	allData = downloadSpy2[0][1].value<QList<CloudData>>();
	for (auto i = 0; i < allData.size(); ++i) {
		const auto key = keys[i + 5];
		qDebug() << "index" << i << "key" << key;
		const auto &data = allData[i];
		QCOMPARE(data.key().tableName, Table);
		QCOMPARE(data.key().key, QStringLiteral("_%1").arg(key));
	}
}

void RemoteConnectorTest::testOutdated()
{
	QSignalSpy uploadSpy1{_rmc1, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy1.isValid());
	QSignalSpy downloadSpy1{_rmc1, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy1.isValid());
	QSignalSpy syncSpy1{_rmc1, &RemoteConnector::triggerSync};
	QVERIFY(syncSpy1.isValid());
	QSignalSpy errorSpy1{_rmc1, &RemoteConnector::networkError};
	QVERIFY(errorSpy1.isValid());

	QSignalSpy uploadSpy2{_rmc2, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy2.isValid());
	QSignalSpy downloadSpy2{_rmc2, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy2.isValid());
	QSignalSpy syncSpy2{_rmc2, &RemoteConnector::triggerSync};
	QVERIFY(syncSpy2.isValid());
	QSignalSpy errorSpy2{_rmc2, &RemoteConnector::networkError};
	QVERIFY(errorSpy2.isValid());

	// upload new data
	CloudData data {
		{Table, QStringLiteral("_key_outdated")},
		std::nullopt,
		QDateTime::currentDateTimeUtc().addSecs(1),
		std::nullopt
	};
	auto token = _rmc1->uploadChange(data);
	QVERIFY(token != InvalidToken);

	VERIFY_SPY(uploadSpy1, errorSpy1);
	QCOMPARE(uploadSpy1.size(), 1);
	QCOMPARE(uploadSpy1[0][0].value<DatasetId>(), data.key());
	QCOMPARE(uploadSpy1[0][1].toDateTime(), data.modified());
	QCOMPARE(uploadSpy1[0][2].toBool(), true);
	QVERIFY(downloadSpy1.isEmpty());
	QVERIFY(syncSpy1.isEmpty());
	uploadSpy1.clear();

	// upload same data from same client again
	data.setData(QJsonObject {
		{QStringLiteral("Key"), 1},
		{QStringLiteral("Value"), QJsonValue::Null}
	});
	token = _rmc1->uploadChange(data);
	QVERIFY(token != InvalidToken);

	VERIFY_SPY(uploadSpy1, errorSpy1);
	QCOMPARE(uploadSpy1.size(), 1);
	QCOMPARE(uploadSpy1[0][0].value<DatasetId>(), data.key());
	QCOMPARE(uploadSpy1[0][1].toDateTime(), data.modified());
	QCOMPARE(uploadSpy1[0][2].toBool(), false);
	QVERIFY(downloadSpy1.isEmpty());
	QVERIFY(syncSpy1.isEmpty());
	uploadSpy1.clear();

	// upload outdated data from different client
	data.setModified(data.modified().addSecs(-1));
	token = _rmc2->uploadChange(data);
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(syncSpy2, errorSpy2);
	QCOMPARE(syncSpy2.size(), 1);
	QCOMPARE(syncSpy2[0][0].toString(), Table);
	QCOMPARE(downloadSpy2.size(), 1);
	QCOMPARE(downloadSpy2[0][0].toString(), Table);
	QCOMPARE(downloadSpy2[0][1].value<QList<CloudData>>().size(), 1);
	const auto cData = downloadSpy2[0][1].value<QList<CloudData>>()[0];
	QCOMPARE(cData.key(), data.key());
	QVERIFY(!cData.data());
	QCOMPARE(cData.modified(), data.modified().addSecs(1));
	QVERIFY(!uploadSpy2.wait());
}

void RemoteConnectorTest::testConflict()
{
	QSignalSpy uploadSpy{_rmc1, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy.isValid());
	QSignalSpy syncSpy{_rmc1, &RemoteConnector::triggerSync};
	QVERIFY(syncSpy.isValid());
	QSignalSpy errorSpy{_rmc1, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	CloudData ulData {
		Table, QStringLiteral("_key_conflict"),
		std::nullopt,
		QDateTime::currentDateTimeUtc(),
		std::nullopt
	};
	const auto token1 = _rmc1->uploadChange(ulData);
	QVERIFY(token1 != InvalidToken);
	const auto token2 = _rmc1->uploadChange(ulData);
	QVERIFY(token2 != InvalidToken);
	VERIFY_SPY(uploadSpy, errorSpy);
	if (syncSpy.isEmpty())
		VERIFY_SPY(syncSpy, errorSpy);
	else
		QVERIFY(errorSpy.isEmpty());
}

void RemoteConnectorTest::testCancel()
{
	QSignalSpy uploadSpy1{_rmc1, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy1.isValid());
	QSignalSpy syncSpy1{_rmc1, &RemoteConnector::triggerSync};
	QVERIFY(syncSpy1.isValid());
	QSignalSpy downloadSpy1{_rmc1, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy1.isValid());
	QSignalSpy doneSpy1{_rmc1, &RemoteConnector::syncDone};
	QVERIFY(doneSpy1.isValid());
	QSignalSpy errorSpy1{_rmc1, &RemoteConnector::networkError};
	QVERIFY(errorSpy1.isValid());

	QSignalSpy uploadSpy2{_rmc2, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy2.isValid());
	QSignalSpy syncSpy2{_rmc2, &RemoteConnector::triggerSync};
	QVERIFY(syncSpy2.isValid());
	QSignalSpy downloadSpy2{_rmc2, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy2.isValid());
	QSignalSpy doneSpy2{_rmc2, &RemoteConnector::syncDone};
	QVERIFY(doneSpy2.isValid());
	QSignalSpy errorSpy2{_rmc2, &RemoteConnector::networkError};
	QVERIFY(errorSpy2.isValid());

	// cancel download
	auto token = _rmc2->getChanges(Table, QDateTime{});
	QVERIFY(token != InvalidToken);
	_rmc2->cancel(token);
	QVERIFY(!downloadSpy2.wait());

	// cancel upload
	token = _rmc1->uploadChange({
		Table, QStringLiteral("_key_cancel"),
		std::nullopt,
		QDateTime::currentDateTimeUtc(),
		std::nullopt
	});
	QVERIFY(token != InvalidToken);
	_rmc1->cancel(token);
	QVERIFY(!uploadSpy1.wait());

	QVERIFY(downloadSpy1.isEmpty());
	QVERIFY(uploadSpy1.isEmpty());
	QVERIFY(syncSpy1.isEmpty());
	QVERIFY(doneSpy1.isEmpty());
	QVERIFY(errorSpy1.isEmpty());

	QVERIFY(downloadSpy2.isEmpty());
	QVERIFY(uploadSpy2.isEmpty());
	QVERIFY(syncSpy2.isEmpty());
	QVERIFY(doneSpy2.isEmpty());
	QVERIFY(errorSpy2.isEmpty());
}

void RemoteConnectorTest::testLiveSync()
{
	QSignalSpy downloadSpy1{_rmc1, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy1.isValid());
	QSignalSpy doneSpy1{_rmc1, &RemoteConnector::syncDone};
	QVERIFY(doneSpy1.isValid());
	QSignalSpy uploadSpy1{_rmc1, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy1.isValid());
	QSignalSpy errorSpy1{_rmc1, &RemoteConnector::networkError};
	QVERIFY(errorSpy1.isValid());

	QSignalSpy downloadSpy2{_rmc2, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy2.isValid());
	QSignalSpy doneSpy2{_rmc2, &RemoteConnector::syncDone};
	QVERIFY(doneSpy2.isValid());
	QSignalSpy uploadSpy2{_rmc2, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy2.isValid());
	QSignalSpy errorSpy2{_rmc2, &RemoteConnector::networkError};
	QVERIFY(errorSpy2.isValid());

	// upload changes since tstamp to test as the sync
	const auto tStamp = QDateTime::currentDateTimeUtc();
	for (auto i = 0; i < 10; ++i) {
		const DatasetId key {Table, QStringLiteral("_%1").arg(i)};
		const auto modified = QDateTime::currentDateTimeUtc();
		CloudData data;
		data.setKey(key);
		data.setData(QJsonObject {
			{QStringLiteral("Key"), i},
			{QStringLiteral("Value"), QStringLiteral("data_%1").arg(i)}
		});
		data.setModified(modified);
		const auto token = _rmc1->uploadChange(data);
		QVERIFY(token != InvalidToken);

		VERIFY_SPY(uploadSpy1, errorSpy1);
		QCOMPARE(uploadSpy1.size(), 1);
		QCOMPARE(uploadSpy1[0][0].value<DatasetId>(), key);
		QCOMPARE(uploadSpy1[0][1].toDateTime(), modified);
		QCOMPARE(uploadSpy1[0][2].toBool(), false);
		QVERIFY(downloadSpy1.isEmpty());
		uploadSpy1.clear();
	}

	// start live sync for rmc1 since tstamp
	const auto liveToken1 = _rmc1->startLiveSync(Table, tStamp);
	QVERIFY(liveToken1 != InvalidToken);
	VERIFY_SPY(doneSpy1, errorSpy1);
	QCOMPARE(doneSpy1[0][0], Table);
	QCOMPARE(doneSpy1.size(), 1);  // ignores paging limit
	QCOMPARE(downloadSpy1.size(), 0);  // no downloads here...

	// start live sync for rmc2 since tstamp and verify data
	const auto liveToken2 = _rmc2->startLiveSync(Table, tStamp);
	QVERIFY(liveToken2 != InvalidToken);
	VERIFY_SPY(doneSpy2, errorSpy2);
	QCOMPARE(doneSpy2.size(), 1);  // ignores paging limit
	QCOMPARE(doneSpy2[0][0], Table);
	QCOMPARE(downloadSpy2.size(), 1);
	QCOMPARE(downloadSpy2[0][0].toString(), Table);
	const auto allData = downloadSpy2[0][1].value<QList<CloudData>>();
	QCOMPARE(allData.size(), 10);
	for (auto i = 0; i < allData.size(); ++i) {
		const auto &data = allData[i];
		if (i > 0)
			QVERIFY(data.uploaded() >= allData[i - 1].uploaded());
		QCOMPARE(data.key().tableName, Table);
		QCOMPARE(data.key().key, QStringLiteral("_%1").arg(i));
		const QJsonObject resData {
			{QStringLiteral("Key"), i},
			{QStringLiteral("Value"), QStringLiteral("data_%1").arg(i)}
		};
		QCOMPARE(data.data(), resData);
		if (i > 0)
			QVERIFY(data.modified() >= allData[i - 1].modified());
	}
	downloadSpy2.clear();
	doneSpy2.clear();

	// do an upload -> should trigger a live sync event
	CloudData data {
		Table, QStringLiteral("_key"),
		std::nullopt,
		QDateTime::currentDateTimeUtc(),
		std::nullopt
	};
	const auto token = _rmc1->uploadChange(data);
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(uploadSpy1, errorSpy1);

	// verify received by rmc2
	if (downloadSpy2.isEmpty())
		VERIFY_SPY(downloadSpy2, errorSpy2);
	else
		QVERIFY(errorSpy2.isEmpty());
	QCOMPARE(downloadSpy2.size(), 1);
	QCOMPARE(downloadSpy2[0][0].toString(), Table);
	QCOMPARE(downloadSpy2[0][1].value<QList<CloudData>>().size(), 1);
	const auto dlData = downloadSpy2[0][1].value<QList<CloudData>>()[0];
	QCOMPARE(dlData.key(), data.key());
	QCOMPARE(dlData.data(), data.data());
	QCOMPARE(dlData.modified(), data.modified());
	QVERIFY(doneSpy2.isEmpty());

	// verify nothing for rmc1
	QCOMPARE(downloadSpy1.size(), 0);
	QVERIFY(!downloadSpy1.wait());
	QCOMPARE(downloadSpy1.size(), 0);

	// stop and do again -> nothing
	data.setModified(QDateTime::currentDateTimeUtc());
	_rmc1->cancel(liveToken1);
	_rmc2->cancel(liveToken2);
	_rmc1->uploadChange(data);
	VERIFY_SPY(uploadSpy1, errorSpy1);
	QVERIFY(!downloadSpy2.wait());
	QVERIFY(errorSpy2.isEmpty());
}

void RemoteConnectorTest::testOfflineDetection()
{
	QSignalSpy onlineSpy{_rmc1, &RemoteConnector::onlineChanged};
	QVERIFY(onlineSpy.isValid());

	QCOMPARE(_rmc1->isOnline(), true);

	_nam->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
	QTRY_COMPARE(onlineSpy.size(), 1);
	QCOMPARE(onlineSpy[0][0].toBool(), false);
	QCOMPARE(_rmc1->isOnline(), false);

	_nam->setNetworkAccessible(QNetworkAccessManager::Accessible);
	QTRY_COMPARE(onlineSpy.size(), 2);
	QCOMPARE(onlineSpy[1][0].toBool(), true);
	QCOMPARE(_rmc1->isOnline(), true);
}

void RemoteConnectorTest::testRemoveTable()
{
	QSignalSpy removedSpy{_rmc2, &RemoteConnector::removedTable};
	QVERIFY(removedSpy.isValid());
	QSignalSpy errorSpy{_rmc2, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	auto token = _rmc2->removeTable(Table);
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(removedSpy, errorSpy);
	QCOMPARE(removedSpy.size(), 1);
	QCOMPARE(removedSpy[0][0].toString(), Table);

	// get changes must be empty now
	QSignalSpy doneSpy{_rmc2, &RemoteConnector::syncDone};
	QVERIFY(doneSpy.isValid());
	QSignalSpy downloadSpy{_rmc2, &RemoteConnector::downloadedData};
	QVERIFY(downloadSpy.isValid());

	token = _rmc2->getChanges(Table, {});
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(doneSpy, errorSpy);
	QCOMPARE(doneSpy.size(), 1);
	QCOMPARE(downloadSpy.size(), 0);
}

void RemoteConnectorTest::testRemoveUser()
{
	// add data again, to have something to remove
	QSignalSpy uploadSpy{_rmc1, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy.isValid());
	QSignalSpy errorSpy{_rmc1, &RemoteConnector::networkError};
	QVERIFY(errorSpy.isValid());

	auto token = _rmc1->uploadChange({
		Table, QStringLiteral("_key_rmuser"),
		std::nullopt,
		QDateTime::currentDateTimeUtc(),
		std::nullopt
	});
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(uploadSpy, errorSpy);
	QCOMPARE(uploadSpy.size(), 1);

	QSignalSpy removedSpy{_rmc1, &RemoteConnector::removedUser};
	QVERIFY(removedSpy.isValid());

	token = _rmc1->removeUser();
	QVERIFY(token != InvalidToken);
	VERIFY_SPY(removedSpy, errorSpy);
	QCOMPARE(removedSpy.size(), 1);

	// get changes must throw an error
	token = _rmc1->getChanges(Table, {});
	QCOMPARE(token, InvalidToken);
	if (errorSpy.isEmpty())
		QVERIFY(errorSpy.wait());
	QCOMPARE(errorSpy.size(), 1);
}

QTEST_MAIN(RemoteConnectorTest)

#include "tst_remoteconnector.moc"


