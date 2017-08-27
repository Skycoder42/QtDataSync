#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QUdpSocket>
#include "tst.h"
#include "QtDataSync/private/userdatanetworkexchange_p.h"
using namespace QtDataSync;

class NetworkExchangeTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testExchangeSetup();
#ifdef Q_OS_UNIX
	void testMessageBroadcast();
#endif
	void testFullExchange_data();
	void testFullExchange();

private:
	QJsonSerializer *ser;
	MockRemoteConnector *con;
	MockAuthenticator *auth;
};

void NetworkExchangeTest::initTestCase()
{
#ifdef Q_OS_LINUX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	Setup setup;
	mockSetup(setup);
	con = static_cast<MockRemoteConnector*>(setup.remoteConnector());
	setup.create();

	ser = new QJsonSerializer(this);
	ser->setEnumAsString(true);

	auth = Setup::authenticatorForSetup<MockAuthenticator>(this);
}

void NetworkExchangeTest::cleanupTestCase()
{
	auth->deleteLater();
	Setup::removeSetup(Setup::DefaultSetup);
	ser->deleteLater();
}

void NetworkExchangeTest::testExchangeSetup()
{
	auto exchange = new UserDataNetworkExchange(this);
	QVERIFY(!exchange->isActive());
	QCOMPARE(exchange->deviceName(), QSysInfo::machineHostName());
	QVERIFY(exchange->users().isEmpty());

	//change deviceName
	QSignalSpy nameSpy(exchange, &UserDataNetworkExchange::deviceNameChanged);
	auto name = QStringLiteral("super-name");
	exchange->setDeviceName(name);
	QCOMPARE(nameSpy.size(), 1);
	QCOMPARE(nameSpy[0][0].toString(), name);
	QCOMPARE(exchange->deviceName(), name);

	name = QSysInfo::machineHostName();
	exchange->resetDeviceName();
	QCOMPARE(nameSpy.size(), 2);
	QCOMPARE(nameSpy[1][0].toString(), name);
	QCOMPARE(exchange->deviceName(), name);

	//start, then stop the exchange
	QSignalSpy activeSpy(exchange, &UserDataNetworkExchange::activeChanged);

	QVERIFY(exchange->startExchange());
	QVERIFY(exchange->isActive());
	QCOMPARE(activeSpy.size(), 1);
	QVERIFY(activeSpy[0][0].toBool());
	QCoreApplication::processEvents();

	exchange->stopExchange();
	QVERIFY(!exchange->isActive());
	QCOMPARE(activeSpy.size(), 2);
	QVERIFY(!activeSpy[1][0].toBool());

	exchange->deleteLater();
}

#ifdef Q_OS_UNIX
void NetworkExchangeTest::testMessageBroadcast()
{
	//create and start the exchange
	auto exchange = new UserDataNetworkExchange(this);
	QVERIFY(exchange->startExchange(0, true));//random port, ALLOW REUSE
	QVERIFY(exchange->isActive());
	auto port = exchange->port();
	QVERIFY(port != 0);

	//create 2nd socket on the same port
	auto testSocket = new QUdpSocket(this);
	QSignalSpy socketSpy(testSocket, &QUdpSocket::readyRead);
	QVERIFY(testSocket->bind(port, QUdpSocket::ShareAddress));

	//wait for the broadcast
	QVERIFY(socketSpy.wait());
	auto datagram = testSocket->receiveDatagram();
	QVERIFY(datagram.isValid());
	try {
		auto message = ser->deserializeFrom<ExchangeDatagram>(datagram.data());
		QCOMPARE(message.type, ExchangeDatagram::UserInfo);
		QCOMPARE(message.data, exchange->deviceName());
		QVERIFY(message.salt.isEmpty());
	} catch(QException &e) {
		QFAIL(e.what());
	}

	testSocket->close();
	testSocket->deleteLater();
	exchange->stopExchange();
	QVERIFY(!exchange->isActive());
	exchange->deleteLater();
}
#endif

void NetworkExchangeTest::testFullExchange_data()
{
	QTest::addColumn<QString>("name");
	QTest::addColumn<QByteArray>("userData");
	QTest::addColumn<bool>("encrypted");

	QTest::newRow("unencrypted") << "test1"
								 << QByteArray("pseudo user data")
								 << false;

	QTest::newRow("encrypted") << "test2"
							   << QByteArray("different data of a user")
							   << true;
}

void NetworkExchangeTest::testFullExchange()
{
	QFETCH(QString, name);
	QFETCH(QByteArray, userData);
	QFETCH(bool, encrypted);

	con->mutex.lock();
	con->pseudoUserData = userData;
	con->mutex.unlock();

	//create and start the exchange
	auto exchange = new UserDataNetworkExchange(this);
	QSignalSpy userSpy(exchange, &UserDataNetworkExchange::usersChanged);
	QSignalSpy dataSpy(exchange, &UserDataNetworkExchange::userDataReceived);
	QVERIFY(exchange->startExchange(0));//random port
	QVERIFY(exchange->isActive());
	auto port = exchange->port();
	QVERIFY(port != 0);

	//create 2nd socket on a random port
	auto testSocket = new QUdpSocket(this);
	QSignalSpy socketSpy(testSocket, &QUdpSocket::readyRead);
	QVERIFY(testSocket->bind());

	//send a user info to the exchange
	userSpy.clear();
	testSocket->writeDatagram(ser->serializeTo<ExchangeDatagram>(name), QHostAddress::LocalHost, port);
	QVERIFY(userSpy.wait());
	QCOMPARE(userSpy.size(), 1);
	QCOMPARE(exchange->users().size(), 1);
	auto user = exchange->users()[0];
	QCOMPARE(user.name(), name);
	QVERIFY(user.address().isLoopback());
	QCOMPARE(user.port(), testSocket->localPort());

	//send again, should not change
	testSocket->writeDatagram(ser->serializeTo<ExchangeDatagram>(name), QHostAddress::LocalHost, port);
	QVERIFY(!userSpy.wait());
	QCOMPARE(userSpy.size(), 1);

	//start export to test socket
	exchange->exportTo(user, encrypted ? QStringLiteral("baum42") : QString());
	socketSpy.wait();
	auto datagram = testSocket->receiveDatagram();
	QVERIFY(datagram.isValid());
	try {
		auto message = ser->deserializeFrom<ExchangeDatagram>(datagram.data());
		QCOMPARE(message.type, ExchangeDatagram::UserData);
		QCOMPARE(!message.salt.isEmpty(), encrypted);
		if(!encrypted)
			QCOMPARE(message.data, QString::fromUtf8(userData.toBase64()));

		//and export the same message back
		testSocket->writeDatagram(ser->serializeTo<ExchangeDatagram>(message), QHostAddress::LocalHost, port);
		QVERIFY(dataSpy.wait());
		QCOMPARE(dataSpy.size(), 1);
		auto dataSig = dataSpy.takeFirst();
		auto sigInfo = dataSig[0].value<UserInfo>();
		QCOMPARE(sigInfo.name(), name);
		QVERIFY(sigInfo.address().isLoopback());
		QCOMPARE(sigInfo.port(), testSocket->localPort());
		QCOMPARE(dataSig[1].toBool(), encrypted);

		//and as last step import it
		con->mutex.lock();
		con->enabled = true;
		con->mutex.unlock();

		auto res = exchange->importFrom(sigInfo, encrypted ? QStringLiteral("baum42") : QString());
		res.waitForFinished();
		QVERIFY(res.isFinished());
		con->mutex.lock();
		auto pseudoUserData = con->pseudoUserData;
		con->mutex.unlock();
		QCOMPARE(pseudoUserData, userData);

		//import without user data available
		auto res2 = exchange->importFrom(sigInfo, encrypted ? QStringLiteral("baum42") : QString());
		QVERIFY_EXCEPTION_THROWN(res2.waitForFinished(), DataSyncException);

		//import with disabled -> triggers error
		con->mutex.lock();
		con->enabled = false;
		con->mutex.unlock();

		testSocket->writeDatagram(ser->serializeTo<ExchangeDatagram>(message), QHostAddress::LocalHost, port);
		QVERIFY(dataSpy.wait());
		auto res3 = exchange->importFrom(sigInfo, encrypted ? QStringLiteral("baum42") : QString());
		QVERIFY_EXCEPTION_THROWN(res3.waitForFinished(), DataSyncException);
	} catch(QException &e) {
		QFAIL(e.what());
	}

	testSocket->close();
	testSocket->deleteLater();
	exchange->stopExchange();
	QVERIFY(!exchange->isActive());
	exchange->deleteLater();
}

QTEST_MAIN(NetworkExchangeTest)

#include "tst_networkexchange.moc"
