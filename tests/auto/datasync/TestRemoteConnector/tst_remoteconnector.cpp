#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <QLoggingCategory>
#include <cryptopp/osrng.h>
#include <QtDataSync/private/remoteconnector_p.h>
#include <QtDataSync/private/defaults_p.h>

#include <QtDataSync/private/loginmessage_p.h>

#include "mockserver.h"
using namespace QtDataSync;

class TestRemoteConnector : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testInvalidIdentify();
	void testRegistering();
	void testLogin(bool hasChanges = false);
	void testLoginWithChanges();

	void testUploading();
	void testDeviceUploading();
	//void testDownloading();

	//void testAddDevice();
	//void testListAndRemoveDevices();

	//void testKeyUpdate();

	//TODO test error message and unexpected messages

private:
	MockServer *server;
	MockConnection *currentConnection;
	CryptoPP::AutoSeededRandomPool rng;
	RemoteConnector *remote;
	QUuid devId;
};

void TestRemoteConnector::initTestCase()
{
	QLoggingCategory::setFilterRules(QStringLiteral("qtdatasync.*.debug=true"));

	qRegisterMetaType<RemoteConnector::RemoteEvent>("RemoteEvent");

	try {
		currentConnection = nullptr;
		server = new MockServer(this);
		server->init();

		TestLib::init();
		Setup setup;
		TestLib::setup(setup);
		setup.setRemoteConfiguration(server->url())
				.create();

		//disable sync
		auto s = Defaults(DefaultsPrivate::obtainDefaults(DefaultSetup)).createSettings(this);
		s->setValue(QStringLiteral("connector/enabled"), false);
		s->sync();

		//wait for engine to finish
		QCoreApplication::processEvents();
		QThread::msleep(500);
		QCoreApplication::processEvents();
		server->clear();

		//enable sync for internal instance
		s->setValue(QStringLiteral("connector/enabled"), true);
		s->sync();
		s->deleteLater();
		remote = new RemoteConnector(DefaultsPrivate::obtainDefaults(DefaultSetup), this);
		devId = QUuid::createUuid();
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::cleanupTestCase()
{
	delete remote;
	remote = nullptr;
	Setup::removeSetup(DefaultSetup, true);
}

void TestRemoteConnector::testInvalidIdentify()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	//init a connection
	remote->initialize({});
	MockConnection *connection = nullptr;
	QVERIFY(server->waitForConnected(&connection));
	QCOMPARE(eventSpy.size(), 1);
	QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);

	//send the identifiy message
	auto iMsg = IdentifyMessage(20); //without a nonce
	connection->send(iMsg);

	//wait for register message
	QVERIFY(connection->waitForDisconnect());
	QCOMPARE(eventSpy.size(), 1);
	QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
	QCOMPARE(errorSpy.size(), 1);
}

void TestRemoteConnector::testRegistering()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy uploadSpy(remote, &RemoteConnector::updateUploadLimit);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	//init a connection
	remote->reconnect();
	auto crypto = remote->cryptoController()->crypto();
	MockConnection *connection = nullptr;
	QVERIFY(server->waitForConnected(&connection));
	QCOMPARE(eventSpy.size(), 1);
	QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);

	//send the identifiy message
	auto iMsg = IdentifyMessage::createRandom(20, rng);
	connection->send(iMsg);

	QVERIFY(uploadSpy.wait());
	QCOMPARE(uploadSpy.size(), 1);
	QCOMPARE(uploadSpy.takeFirst()[0].toUInt(), iMsg.uploadLimit);

	//wait for register message
	QVERIFY(connection->waitForSignedReply<RegisterMessage>(crypto, [&](RegisterMessage message, bool &ok) {
		QCOMPARE(message.nonce, iMsg.nonce);
		QCOMPARE(message.deviceName, remote->deviceName());
		AsymmetricCryptoInfo cInfo(crypto->rng(),
			message.signAlgorithm,
			message.signKey,
			message.cryptAlgorithm,
			message.cryptKey);
		QCOMPARE(cInfo.ownFingerprint(), crypto->ownFingerprint());
		QCOMPARE(message.cmac, remote->cryptoController()->generateEncryptionKeyCmac());
		ok = true;
	}));

	//send account reply
	connection->send(AccountMessage(devId));
	QVERIFY(eventSpy.wait());
	QCOMPARE(eventSpy.size(), 1);
	QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteReady);

	QVERIFY(errorSpy.isEmpty());
}

void TestRemoteConnector::testLogin(bool hasChanges)
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	//init a connection
	remote->reconnect();
	auto crypto = remote->cryptoController()->crypto();
	MockConnection *connection = nullptr;
	QVERIFY(server->waitForConnected(&connection));
	QCOMPARE(eventSpy.size(), 2);
	QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
	QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);

	//send the identifiy message
	auto iMsg = IdentifyMessage::createRandom(20, rng);
	connection->send(iMsg);

	//wait for login message
	QVERIFY(connection->waitForSignedReply<LoginMessage>(crypto, [&](LoginMessage message, bool &ok) {
		QCOMPARE(message.nonce, iMsg.nonce);
		QCOMPARE(message.deviceId, devId);
		QCOMPARE(message.deviceName, remote->deviceName());
		ok = true;
	}));

	//send welcome reply
	connection->send(WelcomeMessage(hasChanges));//no changes etc.
	QVERIFY(eventSpy.wait());
	QCOMPARE(eventSpy.size(), 1);
	QCOMPARE(eventSpy.takeFirst()[0].toInt(), (hasChanges ? RemoteConnector::RemoteReadyWithChanges : RemoteConnector::RemoteReady));

	QVERIFY(errorSpy.isEmpty());
	currentConnection = connection;
}

void TestRemoteConnector::testLoginWithChanges()
{
	testLogin(true);
}

void TestRemoteConnector::testUploading()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy uploadSpy(remote, &RemoteConnector::uploadDone);

	//assume already logged in
	MockConnection *connection = currentConnection;
	QVERIFY(connection);

	//trigger a data change to be send
	QByteArray key("special_key");
	QByteArray data("very_secret_message_data");
	remote->uploadData(key, data);

	//wait for reply
	QVERIFY(connection->waitForReply<ChangeMessage>([&](ChangeMessage message, bool &ok) {
		QCOMPARE(message.dataId, key);
		auto plain = remote->cryptoController()->decryptData(message.keyIndex, message.salt, message.data);
		QCOMPARE(plain, data);
		ok = true;
	}));

	//send back the ack
	connection->send(ChangeAckMessage(key));
	QVERIFY(uploadSpy.wait());
	QCOMPARE(uploadSpy.size(), 1);
	QCOMPARE(uploadSpy.takeFirst()[0].toByteArray(), key);

	QVERIFY(errorSpy.isEmpty());
}

void TestRemoteConnector::testDeviceUploading()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy uploadSpy(remote, &RemoteConnector::deviceUploadDone);

	//assume already logged in
	MockConnection *connection = currentConnection;
	QVERIFY(connection);

	//trigger a data change to be send
	auto deviceId = QUuid::createUuid();
	QByteArray key("special_key");
	QByteArray data("very_secret_message_data");
	remote->uploadDeviceData(key, deviceId, data);

	//wait for reply
	QVERIFY(connection->waitForReply<DeviceChangeMessage>([&](DeviceChangeMessage message, bool &ok) {
		QCOMPARE(message.dataId, key);
		QCOMPARE(message.deviceId, deviceId);
		auto plain = remote->cryptoController()->decryptData(message.keyIndex, message.salt, message.data);
		QCOMPARE(plain, data);
		//send from here because msg copy
		connection->send(DeviceChangeAckMessage(message));
		ok = true;
	}));

	//send back the ack
	QVERIFY(uploadSpy.wait());
	QCOMPARE(uploadSpy.size(), 1);
	auto sgnl = uploadSpy.takeFirst();
	QCOMPARE(sgnl[0].toByteArray(), key);
	QCOMPARE(sgnl[1].toUuid(), deviceId);

	QVERIFY(errorSpy.isEmpty());
}

QTEST_MAIN(TestRemoteConnector)

#include "tst_remoteconnector.moc"
