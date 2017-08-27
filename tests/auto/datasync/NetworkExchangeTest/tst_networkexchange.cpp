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
	void testMessageBroadcast();
	void testFullExchange_data();
	void testFullExchange();

private:
	QJsonSerializer *ser;
};

void NetworkExchangeTest::initTestCase()
{
#ifdef Q_OS_LINUX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	Setup setup;
	mockSetup(setup);
	setup.create();

	ser = new QJsonSerializer(this);
	ser->setEnumAsString(true);
}

void NetworkExchangeTest::cleanupTestCase()
{
	Setup::removeSetup(Setup::DefaultSetup);
	ser->deleteLater();
}

void NetworkExchangeTest::testExchangeSetup()
{
	auto exchange = new UserDataNetworkExchange(this);
	QVERIFY(!exchange->isActive());
	QCOMPARE(exchange->deviceName(), QSysInfo::machineHostName());
	QVERIFY(exchange->users().isEmpty());

	//start, then stop the exchange
	QVERIFY(exchange->startExchange());
	QCoreApplication::processEvents();
	exchange->stopExchange();

	exchange->deleteLater();
}

void NetworkExchangeTest::testMessageBroadcast()
{
	//create and start the exchange
	auto exchange = new UserDataNetworkExchange(this);
	QVERIFY(exchange->startExchange(0, true));//random port, ALLOW REUSE
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
	exchange->deleteLater();
}

void NetworkExchangeTest::testFullExchange_data()
{
	QTest::addColumn<QString>("name");
	QTest::addColumn<bool>("encrypted");

	QTest::newRow("unencrypted") << "test1"
								 << false;
}

void NetworkExchangeTest::testFullExchange()
{
	QFETCH(QString, name);
	QFETCH(bool, encrypted);

	//create and start the exchange
	auto exchange = new UserDataNetworkExchange(this);
	QSignalSpy userSpy(exchange, &UserDataNetworkExchange::usersChanged);
	QSignalSpy dataSpy(exchange, &UserDataNetworkExchange::userDataReceived);
	QVERIFY(exchange->startExchange(0));//random port
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

		//and export the same message back
		testSocket->writeDatagram(ser->serializeTo<ExchangeDatagram>(message), QHostAddress::LocalHost, port);
		QVERIFY(dataSpy.wait());
	} catch(QException &e) {
		QFAIL(e.what());
	}

	testSocket->close();
	testSocket->deleteLater();
	exchange->stopExchange();
	exchange->deleteLater();
}

QTEST_MAIN(NetworkExchangeTest)

#include "tst_networkexchange.moc"
