#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <mockserver.h>
#include <QLoggingCategory>
#include <cryptopp/osrng.h>

//DIRTY HACK: allow access for test
#define private public
#include <QtDataSync/private/defaults_p.h>
#undef private

#include <QtDataSync/private/remoteconnector_p.h>
#include <QtDataSync/private/setup_p.h>

#include <QtDataSync/private/loginmessage_p.h>
#include <QtDataSync/private/syncmessage_p.h>
#include <QtDataSync/private/keychangemessage_p.h>

using namespace QtDataSync;
#if CRYPTOPP_VERSION >= 600
using byte = CryptoPP::byte;
#endif

Q_DECLARE_METATYPE(QSharedPointer<Message>)

class TestRemoteConnector : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testMissingKeystore();

	void testInvalidIdentify();
	void testInvalidVersion();
	void testRegistering();
	void testLogin(bool hasChanges = false, bool withDisconnect = true);
	void testInvalidKeystoreData();
	void testLoginWithChanges();

	void testUploading();
	void testDeviceUploading();
	void testDownloading();
	void testDownloadingInvalid();
	void testResync();
	void testErrorMessage();

	void testAddDeviceUntrusted();
	void testAddDeviceTrusted();
	void testAddDeviceDenied();
	void testAddDeviceInvalid();
	void testAddDeviceNonExistant();
	void testListAndRemoveDevices();

	void testKeyUpdate(bool sendAck = true);
	void testKeyUpdateReject();
	void testKeyUpdateLost();
	void testKeyUpdateFakeDevice();
	void testMacLost();
	void testResetAccount();

	void testUnexpectedMessage_data();
	void testUnexpectedMessage();
	void testUnknownMessage();
	void testBrokenMessage();

#ifdef TEST_PING_MSG
	void testPingMessages();
#endif

	void testRetry();
	void testFinalize();

private:
	MockServer *server;
	CryptoPP::AutoSeededRandomPool rng;

	RemoteConnector *remote;
	QUuid devId;
	MockConnection *connection;

	QString partnerSetup;
	RemoteConnector *partner;
	QUuid partnerDevId;
	MockConnection *partnerConnection;

	std::tuple<RemoteConnector *, QUuid> createSetup(const QString &setupName);
	template <typename TMessage, typename... Args>
	inline QSharedPointer<Message> create(Args... args);
	template <typename TMessage>
	inline QSharedPointer<Message> createFn(const std::function<TMessage()> &fn);
};

void TestRemoteConnector::initTestCase()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
	//QLoggingCategory::setFilterRules(QStringLiteral("qtdatasync.*.debug=true"));

	qRegisterMetaType<RemoteConnector::RemoteEvent>("RemoteEvent");
	qRegisterMetaType<DeviceInfo>("DeviceInfo");
	qRegisterMetaType<QList<DeviceInfo>>("QList<DeviceInfo>");

	remote = nullptr;
	connection = nullptr;
	partner = nullptr;
	partnerConnection = nullptr;

	try {
		TestLib::init();

		connection = nullptr;
		server = new MockServer(this);
		server->init();

		std::tie(remote, devId) = createSetup(DefaultSetup);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::cleanupTestCase()
{
	if(remote) {
		delete remote;
		remote = nullptr;
	}
	if(partner) {
		delete partner;
		partner = nullptr;
	}
	Setup::removeSetup(DefaultSetup, true);
	if(!partnerSetup.isEmpty())
		Setup::removeSetup(partnerSetup, true);
}

void TestRemoteConnector::testMissingKeystore()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		auto p = DefaultsPrivate::obtainDefaults(DefaultSetup);
		p->properties.insert(Defaults::KeyStoreProvider, QStringLiteral("invalid_dummy"));

		remote->initialize({});
		QVERIFY(errorSpy.wait());
		QCOMPARE(errorSpy.size(), 1);
		QCOMPARE(eventSpy.size(), 2);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);

		//restore correct keysstore
		p->properties.insert(Defaults::KeyStoreProvider, QStringLiteral("plain"));
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testInvalidIdentify()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		//init a connection
		remote->reconnect();
		MockConnection *connection = nullptr;
		QVERIFY(server->waitForConnected(&connection));
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);

		//send the identifiy message
		auto iMsg = IdentifyMessage(20); //without a nonce
		connection->send(iMsg);

		//wait for register message
		QVERIFY(connection->waitForDisconnect());
		if(eventSpy.isEmpty())
			QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
		QCOMPARE(errorSpy.size(), 1);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testInvalidVersion()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		//init a connection
		remote->reconnect();
		MockConnection *connection = nullptr;
		QVERIFY(server->waitForConnected(&connection));
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);

		//send the identifiy message
		auto iMsg = IdentifyMessage::createRandom(20, rng);
		iMsg.protocolVersion = QVersionNumber(0,5);//definitly not supported
		connection->send(iMsg);

		//wait for register message
		QVERIFY(connection->waitForDisconnect());
		if(eventSpy.isEmpty())
			QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
		QCOMPARE(errorSpy.size(), 1);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testRegistering()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy uploadSpy(remote, &RemoteConnector::updateUploadLimit);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
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
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			QCOMPARE(message.deviceName, remote->deviceName());
			AsymmetricCryptoInfo cInfo(rng,
				message.signAlgorithm,
				message.signKey,
				message.cryptAlgorithm,
				message.cryptKey);
			QCOMPARE(cInfo.ownFingerprint(), remote->cryptoController()->fingerprint());
			QCOMPARE(message.cmac, remote->cryptoController()->generateEncryptionKeyCmac());
			ok = true;
		}));

		//send account reply
		connection->send(AccountMessage(devId));
		QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteReady);

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testLogin(bool hasChanges, bool withDisconnect)
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		//init a connection
		remote->reconnect();
		auto crypto = remote->cryptoController()->crypto();
		connection = nullptr;
		QVERIFY(server->waitForConnected(&connection));
		if(withDisconnect) {
			QCOMPARE(eventSpy.size(), 2);
			QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
		} else
			QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);

		//send the identifiy message
		auto iMsg = IdentifyMessage::createRandom(20, rng);
		connection->send(iMsg);

		//wait for login message
		QVERIFY(connection->waitForSignedReply<LoginMessage>(crypto, [&](LoginMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
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
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testInvalidKeystoreData()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		auto s = Defaults(DefaultsPrivate::obtainDefaults(DefaultSetup)).createSettings(this);
		s->setValue(QStringLiteral("connector/deviceId"), QUuid::createUuid());
		s->sync();

		//run
		remote->reconnect();
		QVERIFY(errorSpy.wait());
		QCOMPARE(errorSpy.size(), 1);
		QCOMPARE(eventSpy.size(), 3);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);

		//restore correct keysstore
		s->setValue(QStringLiteral("connector/deviceId"), devId);
		s->sync();
		s->deleteLater();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testLoginWithChanges()
{
	testLogin(true, false);
}

void TestRemoteConnector::testUploading()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy uploadSpy(remote, &RemoteConnector::uploadDone);

	try {
		//assume already logged in
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
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testDeviceUploading()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy uploadSpy(remote, &RemoteConnector::deviceUploadDone);

	try {
		//assume already logged in
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
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testDownloading()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);
	QSignalSpy downloadSpy(remote, &RemoteConnector::downloadData);
	QSignalSpy progUpdateSpy(remote, &RemoteConnector::progressAdded);
	QSignalSpy progIncSpy(remote, &RemoteConnector::progressIncrement);

	try {
		//assume already logged in
		QVERIFY(connection);

		//send the change info with 2 changes
		QByteArray data1 = "random_dataset_1";
		ChangedInfoMessage infoMsg(2);
		infoMsg.dataIndex = 10;
		std::tie(infoMsg.keyIndex, infoMsg.salt, infoMsg.data) = remote->cryptoController()->encryptData(data1);
		connection->send(infoMsg);

		//check signals
		QVERIFY(downloadSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteReadyWithChanges);
		QCOMPARE(progUpdateSpy.size(), 1);
		QCOMPARE(progUpdateSpy.takeFirst()[0].toUInt(), infoMsg.changeEstimate);
		QCOMPARE(downloadSpy.size(), 1);
		auto cChange = downloadSpy.takeFirst();
		QCOMPARE(cChange[0].toULongLong(), infoMsg.dataIndex);
		QCOMPARE(cChange[1].toByteArray(), data1);

		//complete the change
		remote->downloadDone(infoMsg.dataIndex);
		QVERIFY(connection->waitForReply<ChangedAckMessage>([&](ChangedAckMessage message, bool &ok) {
			QCOMPARE(message.dataIndex, infoMsg.dataIndex);
			ok = true;
		}));
		QCOMPARE(progIncSpy.size(), 1);

		//send another (normal) change
		QByteArray data2 = "random_dataset_2";
		ChangedMessage changeMsg;
		changeMsg.dataIndex = 20;
		std::tie(changeMsg.keyIndex, changeMsg.salt, changeMsg.data) = remote->cryptoController()->encryptData(data2);
		connection->send(changeMsg);

		//check signals
		QVERIFY(downloadSpy.wait());
		QCOMPARE(downloadSpy.size(), 1);
		cChange = downloadSpy.takeFirst();
		QCOMPARE(cChange[0].toULongLong(), changeMsg.dataIndex);
		QCOMPARE(cChange[1].toByteArray(), data2);

		//complete the change
		remote->downloadDone(changeMsg.dataIndex);
		QVERIFY(connection->waitForReply<ChangedAckMessage>([&](ChangedAckMessage message, bool &ok) {
			QCOMPARE(message.dataIndex, changeMsg.dataIndex);
			ok = true;
		}));
		QCOMPARE(progIncSpy.size(), 2);

		//complete downloading
		connection->send(LastChangedMessage());
		QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteReady);

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testDownloadingInvalid()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);
	QSignalSpy downloadSpy(remote, &RemoteConnector::downloadData);

	try {
		//assume already logged in
		QVERIFY(connection);

		//send the change info with 2 changes
		QByteArray data1 = "random_dataset_1";
		ChangedInfoMessage infoMsg(2);
		infoMsg.dataIndex = 10;
		std::tie(infoMsg.keyIndex, infoMsg.salt, infoMsg.data) = remote->cryptoController()->encryptData(data1);
		infoMsg.salt[0] = infoMsg.salt[0] + 'b';
		connection->send(infoMsg);

		//check signals
		QVERIFY(errorSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteReadyWithChanges);
		QCOMPARE(errorSpy.size(), 1);

		QVERIFY(connection->waitForDisconnect());
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
		QVERIFY(downloadSpy.isEmpty());

		testLogin(false, false);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testResync()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		//assume already logged in
		QVERIFY(connection);

		//send a resync
		remote->resync();
		QVERIFY(connection->waitForReply<SyncMessage>([&](SyncMessage message, bool &ok) {
			Q_UNUSED(message);
			ok = true;
		}));
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteReadyWithChanges);

		//send no changes
		connection->send(LastChangedMessage());
		QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteReady);

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testErrorMessage()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		//assume already logged in
		QVERIFY(connection);
		auto crypto = remote->cryptoController()->crypto();

		//send an error
		connection->send(ErrorMessage(ErrorMessage::ServerError, {}, true));
		QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);

		//wait for reconnect
		QVERIFY(server->waitForConnected(&connection, 10000));//5 extra secs because of retry
		if(eventSpy.isEmpty())
			QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);

		QVERIFY(errorSpy.isEmpty());

		//send the identifiy message
		auto iMsg = IdentifyMessage::createRandom(20, rng);
		connection->send(iMsg);

		//wait for login message
		QVERIFY(connection->waitForSignedReply<LoginMessage>(crypto, [&](LoginMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			//skip checks
			ok = true;
		}));

		//send an error
		connection->send(ErrorMessage(ErrorMessage::AuthenticationError));
		QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
		QCOMPARE(errorSpy.size(), 1);

		//wait for not comming
		QVERIFY(!eventSpy.wait());
		testLogin(false, false);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testAddDeviceUntrusted()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy loginSpy(remote, &RemoteConnector::loginRequested);
	QSignalSpy grantedSpy(remote, &RemoteConnector::accountAccessGranted);

	try {
		//assume already logged in
		QVERIFY(connection);
		auto crypto = remote->cryptoController()->crypto();

		//create a second setup...
		partnerSetup = QStringLiteral("partner1");
		std::tie(partner, partnerDevId) = createSetup(partnerSetup);
		QSignalSpy partnerErrorSpy(partner, &RemoteConnector::controllerError);
		QSignalSpy partnerImportSpy(partner, &RemoteConnector::importCompleted);

		//data export
		ExportData exportData;
		QByteArray salt;
		CryptoPP::SecByteBlock key;
		std::tie(exportData, salt, key) = remote->exportAccount(true, QString());

		QVERIFY(!exportData.trusted);
		QCOMPARE(exportData.partnerId, devId);
		QVERIFY(exportData.config);
		remote->cryptoController()->verifyImportCmac(exportData.scheme, key, exportData.signData(), exportData.cmac);

		//create a second client and pass it the import data, then connect and send identify
		partner->prepareImport(exportData, CryptoPP::SecByteBlock());
		partner->initialize({});
		auto partnerCrypto = partner->cryptoController()->crypto();
		QVERIFY(server->waitForConnected(&partnerConnection));
		auto iMsg = IdentifyMessage::createRandom(10, rng);
		partnerConnection->send(iMsg);

		//wait for the access message
		QVERIFY(partnerConnection->waitForSignedReply<AccessMessage>(partnerCrypto, [&](AccessMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			QCOMPARE(message.deviceName, partner->deviceName());
			AsymmetricCryptoInfo cInfo(rng,
				message.signAlgorithm,
				message.signKey,
				message.cryptAlgorithm,
				message.cryptKey);
			QCOMPARE(cInfo.ownFingerprint(), partner->cryptoController()->fingerprint());

			QCOMPARE(message.pNonce, exportData.pNonce);
			QCOMPARE(message.partnerId, exportData.partnerId);
			QCOMPARE(message.macscheme, exportData.scheme);
			QCOMPARE(message.cmac, exportData.cmac);
			QVERIFY(message.trustmac.isEmpty());
			ok = true;

			//Send from here because message needed
			connection->send(ProofMessage(message, partnerDevId));
		}));

		//Send the proof the main con and wait for login request
		QVERIFY(loginSpy.wait());
		QCOMPARE(loginSpy.size(), 1);
		auto devInfo = loginSpy.takeFirst()[0].value<DeviceInfo>();
		QCOMPARE(devInfo.deviceId(), partnerDevId);
		QCOMPARE(devInfo.name(), partner->deviceName());
		QCOMPARE(devInfo.fingerprint(), partner->cryptoController()->fingerprint());

		//accept the request
		remote->loginReply(partnerDevId, true);
		QVERIFY(connection->waitForSignedReply<AcceptMessage>(crypto, [&](AcceptMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			QCOMPARE(message.index, remote->cryptoController()->keyIndex());
			partnerCrypto->decrypt(message.secret); //make shure works without exception
			ok = true;

			//Send from here because message needed
			partnerConnection->send(GrantMessage(message));
			connection->send(AcceptAckMessage(message.deviceId));
		}));

		//verify preperations are done
		if(grantedSpy.isEmpty())
			QVERIFY(grantedSpy.wait());
		QCOMPARE(grantedSpy.size(), 1);
		QCOMPARE(grantedSpy.takeFirst()[0].toUuid(), partnerDevId);

		//verify grant message received successfully
		if(partnerImportSpy.isEmpty())
			QVERIFY(partnerImportSpy.wait());
		QCOMPARE(partnerImportSpy.size(), 1);

		//wait for mac update message
		QVERIFY(partnerConnection->waitForReply<MacUpdateMessage>([&](MacUpdateMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, partner->cryptoController()->keyIndex());
			QCOMPARE(message.cmac, partner->cryptoController()->generateEncryptionKeyCmac());
			ok = true;
		}));

		//send macack
		partnerConnection->send(MacUpdateAckMessage());
		//cannot really check if received... but simple enough to assume worked

		QVERIFY(errorSpy.isEmpty());
		QVERIFY(partnerErrorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testAddDeviceTrusted()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy loginSpy(remote, &RemoteConnector::loginRequested);
	QSignalSpy grantedSpy(remote, &RemoteConnector::accountAccessGranted);

	try {
		//assume already logged in
		QVERIFY(connection);
		auto crypto = remote->cryptoController()->crypto();

		//create a second setup...
		auto setup2 = QStringLiteral("partner2");
		RemoteConnector *partner2;
		QUuid devId2;
		std::tie(partner2, devId2) = createSetup(setup2);
		QSignalSpy partnerErrorSpy(partner2, &RemoteConnector::controllerError);
		QSignalSpy partnerImportSpy(partner2, &RemoteConnector::importCompleted);

		//data export
		auto password = QStringLiteral("random_secure_password");
		ExportData exportData;
		QByteArray salt;
		CryptoPP::SecByteBlock key;
		std::tie(exportData, salt, key) = remote->exportAccount(false, password);

		QVERIFY(exportData.trusted);
		QCOMPARE(exportData.partnerId, devId);
		QVERIFY(!exportData.config);
		remote->cryptoController()->verifyImportCmac(exportData.scheme, key, exportData.signData(), exportData.cmac);

		//create a second client and pass it the import data, then connect and send identify
		partner2->prepareImport(exportData, key); //assume correctly recovered key
		partner2->initialize({});
		auto partnerCrypto = partner2->cryptoController()->crypto();
		MockConnection *partnerCon = nullptr;
		QVERIFY(server->waitForConnected(&partnerCon));
		auto iMsg = IdentifyMessage::createRandom(10, rng);
		partnerCon->send(iMsg);

		//wait for the access message
		QVERIFY(partnerCon->waitForSignedReply<AccessMessage>(partnerCrypto, [&](AccessMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			QCOMPARE(message.deviceName, partner2->deviceName());
			AsymmetricCryptoInfo cInfo(rng,
				message.signAlgorithm,
				message.signKey,
				message.cryptAlgorithm,
				message.cryptKey);
			QCOMPARE(cInfo.ownFingerprint(), partner2->cryptoController()->fingerprint());

			QCOMPARE(message.pNonce, exportData.pNonce);
			QCOMPARE(message.partnerId, exportData.partnerId);
			QCOMPARE(message.macscheme, exportData.scheme);
			QCOMPARE(message.cmac, exportData.cmac);
			QVERIFY(!message.trustmac.isEmpty());
			remote->cryptoController()->verifyImportCmacForCrypto(exportData.scheme, key, &cInfo, message.trustmac);
			ok = true;

			//Send from here because message needed
			connection->send(ProofMessage(message, devId2));
		}));

		//wait for the automatically accepted request
		QVERIFY(connection->waitForSignedReply<AcceptMessage>(crypto, [&](AcceptMessage message, bool &ok) {
			QCOMPARE(message.deviceId, devId2);
			QCOMPARE(message.index, remote->cryptoController()->keyIndex());
			partnerCrypto->decrypt(message.secret); //make shure works without exception
			ok = true;

			//Send from here because message needed
			partnerCon->send(GrantMessage(message));
			connection->send(AcceptAckMessage(message.deviceId));
		}));

		//verify preperations are done
		if(grantedSpy.isEmpty())
			QVERIFY(grantedSpy.wait());
		QCOMPARE(grantedSpy.size(), 1);
		QCOMPARE(grantedSpy.takeFirst()[0].toUuid(), devId2);

		//verify grant message received successfully
		if(partnerImportSpy.isEmpty())
			QVERIFY(partnerImportSpy.wait());
		QCOMPARE(partnerImportSpy.size(), 1);

		//wait for mac update message
		QVERIFY(partnerCon->waitForReply<MacUpdateMessage>([&](MacUpdateMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, partner2->cryptoController()->keyIndex());
			QCOMPARE(message.cmac, partner2->cryptoController()->generateEncryptionKeyCmac());
			ok = true;
		}));

		//send macack
		partnerCon->send(MacUpdateAckMessage());
		//cannot really check if received... but simple enough to assume worked

		QVERIFY(errorSpy.isEmpty());
		QVERIFY(partnerErrorSpy.isEmpty());
		QVERIFY(loginSpy.isEmpty());

		//cleanup: remove the partner2
		partner2->deleteLater();
		QVERIFY(partnerCon->waitForDisconnect());
		partnerCon->deleteLater();
		Setup::removeSetup(setup2, true);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testAddDeviceDenied()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy loginSpy(remote, &RemoteConnector::loginRequested);

	try {
		//assume already logged in
		QVERIFY(connection);

		//create a second setup...
		auto setup3 = QStringLiteral("partner3");
		RemoteConnector *partner3;
		QUuid devId3;
		std::tie(partner3, devId3) = createSetup(setup3);
		QSignalSpy partnerErrorSpy(partner3, &RemoteConnector::controllerError);

		//data export
		ExportData exportData;
		QByteArray salt;
		CryptoPP::SecByteBlock key;
		std::tie(exportData, salt, key) = remote->exportAccount(false, QString());

		QVERIFY(!exportData.trusted);
		QCOMPARE(exportData.partnerId, devId);
		QVERIFY(!exportData.config);
		remote->cryptoController()->verifyImportCmac(exportData.scheme, key, exportData.signData(), exportData.cmac);

		//create a second client and pass it the import data, then connect and send identify
		partner3->prepareImport(exportData, key); //assume correctly recovered key
		partner3->initialize({});
		auto partnerCrypto = partner3->cryptoController()->crypto();
		MockConnection *partnerCon = nullptr;
		QVERIFY(server->waitForConnected(&partnerCon));
		auto iMsg = IdentifyMessage::createRandom(10, rng);
		partnerCon->send(iMsg);

		//wait for the access message
		QVERIFY(partnerCon->waitForSignedReply<AccessMessage>(partnerCrypto, [&](AccessMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			//skip additional checks
			ok = true;

			//Send from here because message needed
			connection->send(ProofMessage(message, devId3));
		}));

		//Send the proof the main con and wait for login request
		QVERIFY(loginSpy.wait());
		QCOMPARE(loginSpy.size(), 1);
		auto devInfo = loginSpy.takeFirst()[0].value<DeviceInfo>();
		QCOMPARE(devInfo.deviceId(), devId3);
		QCOMPARE(devInfo.name(), partner3->deviceName());
		QCOMPARE(devInfo.fingerprint(), partner3->cryptoController()->fingerprint());

		//deny the login
		remote->loginReply(devId3, false);
		QVERIFY(connection->waitForReply<DenyMessage>([&](DenyMessage message, bool &ok) {
			QCOMPARE(message.deviceId, devId3);
			ok = true;
		}));

		//stop here

		QVERIFY(errorSpy.isEmpty());
		QVERIFY(partnerErrorSpy.isEmpty());

		//cleanup: remove the partner2
		partner3->deleteLater();
		QVERIFY(partnerCon->waitForDisconnect());
		partnerCon->deleteLater();
		Setup::removeSetup(setup3, true);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testAddDeviceInvalid()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);

	try {
		//assume already logged in
		QVERIFY(connection);

		//create a second setup...
		auto setup4 = QStringLiteral("partner4");
		RemoteConnector *partner4;
		QUuid devId4;
		std::tie(partner4, devId4) = createSetup(setup4);
		QSignalSpy partnerErrorSpy(partner4, &RemoteConnector::controllerError);

		//data export
		ExportData exportData;
		QByteArray salt;
		CryptoPP::SecByteBlock key;
		std::tie(exportData, salt, key) = remote->exportAccount(false, QString());

		QVERIFY(!exportData.trusted);
		QCOMPARE(exportData.partnerId, devId);
		QVERIFY(!exportData.config);
		remote->cryptoController()->verifyImportCmac(exportData.scheme, key, exportData.signData(), exportData.cmac);

		//create a second client and pass it the import data, then connect and send identify
		partner4->prepareImport(exportData, key); //assume correctly recovered key
		partner4->initialize({});
		auto partnerCrypto = partner4->cryptoController()->crypto();
		MockConnection *partnerCon = nullptr;
		QVERIFY(server->waitForConnected(&partnerCon));
		auto iMsg = IdentifyMessage::createRandom(10, rng);
		partnerCon->send(iMsg);

		//wait for the access message
		QVERIFY(partnerCon->waitForSignedReply<AccessMessage>(partnerCrypto, [&](AccessMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			//skip additional checks
			ok = true;

			//tamper with the cmac
			message.cmac[0] = message.cmac[0] + 'B';
			//Send from here because message needed
			connection->send(ProofMessage(message, devId4));
		}));

		//wait for the deny message
		QVERIFY(connection->waitForReply<DenyMessage>([&](DenyMessage message, bool &ok) {
			QCOMPARE(message.deviceId, devId4);
			ok = true;
		}));

		//stop here

		QVERIFY(errorSpy.isEmpty());
		QVERIFY(partnerErrorSpy.isEmpty());

		//cleanup: remove the partner2
		partner4->deleteLater();
		QVERIFY(partnerCon->waitForDisconnect());
		partnerCon->deleteLater();
		Setup::removeSetup(setup4, true);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testAddDeviceNonExistant()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);

	try {
		//assume already logged in
		QVERIFY(connection);

		//create a second setup...
		auto setup5 = QStringLiteral("partner5");
		RemoteConnector *partner5;
		QUuid devId5;
		std::tie(partner5, devId5) = createSetup(setup5);
		QSignalSpy partnerErrorSpy(partner5, &RemoteConnector::controllerError);

		//generate theoretically valid export data
		ExportData data;
		data.trusted = false;
		data.pNonce.resize(InitMessage::NonceSize);
		rng.GenerateBlock(reinterpret_cast<byte*>(data.pNonce.data()), data.pNonce.size());
		data.partnerId = devId;

		QByteArray salt;
		CryptoPP::SecByteBlock key;
		std::tie(data.scheme, salt, key) = remote->cryptoController()->generateExportKey(QString());
		data.cmac = remote->cryptoController()->createExportCmac(data.scheme, key, data.signData());

		//create a second client and pass it the import fake data, then connect and send identify
		partner5->prepareImport(data, key); //assume correctly recovered key
		partner5->initialize({});
		auto partnerCrypto = partner5->cryptoController()->crypto();
		MockConnection *partnerCon = nullptr;
		QVERIFY(server->waitForConnected(&partnerCon));
		auto iMsg = IdentifyMessage::createRandom(10, rng);
		partnerCon->send(iMsg);

		//wait for the access message
		QVERIFY(partnerCon->waitForSignedReply<AccessMessage>(partnerCrypto, [&](AccessMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			//skip additional checks
			ok = true;

			//tamper with the cmac
			message.cmac[0] = message.cmac[0] + 'B';
			//Send from here because message needed
			connection->send(ProofMessage(message, devId5));
		}));

		//wait for the deny message
		QVERIFY(connection->waitForReply<DenyMessage>([&](DenyMessage message, bool &ok) {
			QCOMPARE(message.deviceId, devId5);
			ok = true;
		}));

		//stop here

		QVERIFY(errorSpy.isEmpty());
		QVERIFY(partnerErrorSpy.isEmpty());

		//cleanup: remove the partner2
		partner5->deleteLater();
		QVERIFY(partnerCon->waitForDisconnect());
		partnerCon->deleteLater();
		Setup::removeSetup(setup5, true);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testListAndRemoveDevices()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy devicesSpy(remote, &RemoteConnector::devicesListed);

	try {
		//assume already logged in
		QVERIFY(connection);

		//send a list devices
		remote->listDevices();
		QVERIFY(connection->waitForReply<ListDevicesMessage>([&](ListDevicesMessage message, bool &ok) {
			Q_UNUSED(message);
			ok = true;
		}));

		//send the other partner
		DevicesMessage message;
		message.devices.append(std::make_tuple(
								   partnerDevId,
								   partner->deviceName(),
								   partner->cryptoController()->fingerprint()
							   ));
		connection->send(message);
		QVERIFY(devicesSpy.wait());
		QCOMPARE(devicesSpy.size(), 1);
		auto devList = devicesSpy.takeFirst()[0].value<QList<DeviceInfo>>();
		QCOMPARE(devList.size(), 1);
		QCOMPARE(devList[0].deviceId(), partnerDevId);
		QCOMPARE(devList[0].name(), partner->deviceName());
		QCOMPARE(devList[0].fingerprint(), partner->cryptoController()->fingerprint());

		//remote partner from devices
		remote->removeDevice(partnerDevId);
		QVERIFY(connection->waitForReply<RemoveMessage>([&](RemoveMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			ok = true;
		}));

		//send the reply
		connection->send(RemoveAckMessage(partnerDevId));
		QVERIFY(devicesSpy.wait());
		QCOMPARE(devicesSpy.size(), 1);
		devList = devicesSpy.takeFirst()[0].value<QList<DeviceInfo>>();
		QCOMPARE(devList.size(), 0);

		//remove self not allowed
		remote->removeDevice(devId);
		QVERIFY(connection->waitForNothing());

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testKeyUpdate(bool sendAck)
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy partnerErrorSpy(partner, &RemoteConnector::controllerError);

	try {
		//assume already logged in
		QVERIFY(connection);
		QVERIFY(partnerConnection);
		auto pCrypto = partner->cryptoController()->crypto();

		//get the key update
		remote->initKeyUpdate();
		QVERIFY(connection->waitForReply<KeyChangeMessage>([&](KeyChangeMessage message, bool &ok) {
			QCOMPARE(message.nextIndex, remote->cryptoController()->keyIndex() + 1);
			ok = true;
		}));

		//accept it
		QList<DeviceKeysMessage::DeviceKey> devices;
		devices.append(std::make_tuple(
						   partnerDevId,
						   pCrypto->encryptionScheme(),
						   pCrypto->writeCryptKey(),
						   partner->cryptoController()->generateEncryptionKeyCmac()
					   ));
		connection->send(DeviceKeysMessage(remote->cryptoController()->keyIndex() + 1,
										   devices));

		//wait for reply
		WelcomeMessage welcomeMessage(false);
		QVERIFY(connection->waitForReply<NewKeyMessage>([&](NewKeyMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, remote->cryptoController()->keyIndex() + 1);
			QCOMPARE(message.cmac, remote->cryptoController()->generateEncryptionKeyCmac(message.keyIndex));
			QCOMPARE(message.deviceKeys.size(), 1);

			QUuid mId;
			std::tie(mId, welcomeMessage.key, welcomeMessage.cmac) = message.deviceKeys.first();
			QCOMPARE(mId, partnerDevId);
			partner->cryptoController()->verifyCmac(partner->cryptoController()->keyIndex(),
													message.signatureData(message.deviceKeys.first()),
													welcomeMessage.cmac);
			auto pKey = pCrypto->decrypt(welcomeMessage.key);
			QVERIFY(!pKey.isEmpty());
			welcomeMessage.keyIndex = message.keyIndex;
			welcomeMessage.scheme = message.scheme;
			ok = true;

			//send key ack (verify later)
			connection->send(NewKeyAckMessage(message));
		}));

		//reconnect the partner and do a login with key updates
		partner->reconnect();
		QVERIFY(partnerConnection->waitForDisconnect());
		QVERIFY(server->waitForConnected(&partnerConnection));

		//send the identifiy message
		auto iMsg = IdentifyMessage::createRandom(20, rng);
		partnerConnection->send(iMsg);

		//wait for login message
		QVERIFY(partnerConnection->waitForSignedReply<LoginMessage>(pCrypto, [&](LoginMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			QCOMPARE(message.deviceId, partnerDevId);
			QCOMPARE(message.deviceName, partner->deviceName());
			ok = true;
		}));

		//send welcome reply
		partnerConnection->send(welcomeMessage);//no changes, but key update
		QVERIFY(partnerConnection->waitForReply<MacUpdateMessage>([&](MacUpdateMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, welcomeMessage.keyIndex);
			partner->cryptoController()->verifyEncryptionKeyCmac(pCrypto, *(pCrypto->cryptKey()), message.cmac);
			ok = true;
		}));

		//send the mac update
		if(sendAck)
			partnerConnection->send(MacUpdateAckMessage());

		//final: check if both have the new index
		QCOMPARE(remote->cryptoController()->keyIndex(), welcomeMessage.keyIndex);
		QCOMPARE(partner->cryptoController()->keyIndex(), welcomeMessage.keyIndex);

		QVERIFY(errorSpy.isEmpty());
		QVERIFY(partnerErrorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testKeyUpdateReject()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		//assume already logged in
		QVERIFY(connection);
		auto cIndex = remote->cryptoController()->keyIndex();

		//get the key update
		remote->initKeyUpdate();
		QVERIFY(connection->waitForReply<KeyChangeMessage>([&](KeyChangeMessage message, bool &ok) {
			QCOMPARE(message.nextIndex, cIndex + 1);
			ok = true;
		}));

		//reject it
		connection->send(ErrorMessage(ErrorMessage::KeyIndexError));
		QVERIFY(connection->waitForDisconnect());

		if(errorSpy.isEmpty())
			QVERIFY(errorSpy.wait());
		QCOMPARE(errorSpy.size(), 1);
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);

		//verify still old key
		QCOMPARE(remote->cryptoController()->keyIndex(), cIndex);

		//login again...
		testLogin(false, false);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testKeyUpdateLost()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy partnerErrorSpy(partner, &RemoteConnector::controllerError);

	try {
		//assume already logged in
		QVERIFY(connection);
		QVERIFY(partnerConnection);
		auto pCrypto = partner->cryptoController()->crypto();

		//get the key update
		remote->initKeyUpdate();
		QVERIFY(connection->waitForReply<KeyChangeMessage>([&](KeyChangeMessage message, bool &ok) {
			QCOMPARE(message.nextIndex, remote->cryptoController()->keyIndex() + 1);
			ok = true;
		}));

		//accept it
		QList<DeviceKeysMessage::DeviceKey> devices;
		devices.append(std::make_tuple(
						   partnerDevId,
						   pCrypto->encryptionScheme(),
						   pCrypto->writeCryptKey(),
						   partner->cryptoController()->generateEncryptionKeyCmac()
					   ));
		connection->send(DeviceKeysMessage(remote->cryptoController()->keyIndex() + 1,
										   devices));

		//wait for reply
		QVERIFY(connection->waitForReply<NewKeyMessage>([&](NewKeyMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, remote->cryptoController()->keyIndex() + 1);
			//skip other checks
			//ack message is lost
			ok = true;
		}));

		//log in remote again
		testLogin(false);

		//wait for the KeyChangeMessage
		QVERIFY(connection->waitForReply<KeyChangeMessage>([&](KeyChangeMessage message, bool &ok) {
			QCOMPARE(message.nextIndex, remote->cryptoController()->keyIndex() + 1);
			ok = true;
		}));

		//accept it
		connection->send(DeviceKeysMessage(remote->cryptoController()->keyIndex() + 1,
										   devices));

		//wait for reply
		WelcomeMessage welcomeMsg;
		QVERIFY(connection->waitForReply<NewKeyMessage>([&](NewKeyMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, remote->cryptoController()->keyIndex() + 1);
			//skip other checks
			QUuid mId;
			std::tie(mId, welcomeMsg.key, welcomeMsg.cmac) = message.deviceKeys.first();
			welcomeMsg.keyIndex = message.keyIndex;
			welcomeMsg.scheme = message.scheme;
			ok = true;
			//again, don't ack, but this time update partners (i.e. accept it)
		}));

		//reconnect the partner and do a login with key updates
		partner->reconnect();
		QVERIFY(partnerConnection->waitForDisconnect());
		QVERIFY(server->waitForConnected(&partnerConnection));

		//send the identifiy message
		auto iMsg = IdentifyMessage::createRandom(20, rng);
		partnerConnection->send(iMsg);

		//wait for login message
		QVERIFY(partnerConnection->waitForSignedReply<LoginMessage>(pCrypto, [&](LoginMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			QCOMPARE(message.deviceId, partnerDevId);
			QCOMPARE(message.deviceName, partner->deviceName());
			ok = true;
		}));

		//send welcome reply
		partnerConnection->send(welcomeMsg);//no changes, but key update
		QVERIFY(partnerConnection->waitForReply<MacUpdateMessage>([&](MacUpdateMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, welcomeMsg.keyIndex);
			partner->cryptoController()->verifyEncryptionKeyCmac(pCrypto, *(pCrypto->cryptKey()), message.cmac);
			ok = true;
		}));

		//send the mac update
		partnerConnection->send(MacUpdateAckMessage());

		//again, reconnect the remote
		testLogin(false);

		//wait for the KeyChangeMessage
		QVERIFY(connection->waitForReply<KeyChangeMessage>([&](KeyChangeMessage message, bool &ok) {
			QCOMPARE(message.nextIndex, remote->cryptoController()->keyIndex() + 1);
			ok = true;
		}));

		//send back the keychange without any devices
		connection->send(DeviceKeysMessage(remote->cryptoController()->keyIndex() + 1));
		QVERIFY(connection->waitForNothing());

		//final: check if both have the new index
		QCOMPARE(remote->cryptoController()->keyIndex(), welcomeMsg.keyIndex);
		QCOMPARE(partner->cryptoController()->keyIndex(), welcomeMsg.keyIndex);

		QVERIFY(errorSpy.isEmpty());
		QVERIFY(partnerErrorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testKeyUpdateFakeDevice()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy partnerErrorSpy(partner, &RemoteConnector::controllerError);

	try {
		//assume already logged in
		QVERIFY(connection);
		QVERIFY(partnerConnection);
		auto pCrypto = partner->cryptoController()->crypto();

		//get the key update
		remote->initKeyUpdate();
		QVERIFY(connection->waitForReply<KeyChangeMessage>([&](KeyChangeMessage message, bool &ok) {
			QCOMPARE(message.nextIndex, remote->cryptoController()->keyIndex() + 1);
			ok = true;
		}));

		//accept it, but add a second, invalid device
		QList<DeviceKeysMessage::DeviceKey> devices;
		devices.append(std::make_tuple(
						   partnerDevId,
						   pCrypto->encryptionScheme(),
						   pCrypto->writeCryptKey(),
						   partner->cryptoController()->generateEncryptionKeyCmac()
					   ));
		devices.append(std::make_tuple(
						   QUuid::createUuid(),
						   pCrypto->encryptionScheme(),
						   pCrypto->writeCryptKey(),
						   QByteArray("cannot_create_valid_cmac")
					   ));
		connection->send(DeviceKeysMessage(remote->cryptoController()->keyIndex() + 1,
										   devices));

		//wait for reply
		WelcomeMessage welcomeMsg;
		QVERIFY(connection->waitForReply<NewKeyMessage>([&](NewKeyMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, remote->cryptoController()->keyIndex() + 1);
			QCOMPARE(message.cmac, remote->cryptoController()->generateEncryptionKeyCmac(message.keyIndex));
			QCOMPARE(message.deviceKeys.size(), 1);

			QUuid mId;
			std::tie(mId, welcomeMsg.key, welcomeMsg.cmac) = message.deviceKeys.first();
			QCOMPARE(mId, partnerDevId);
			partner->cryptoController()->verifyCmac(partner->cryptoController()->keyIndex(),
													message.signatureData(message.deviceKeys.first()),
													welcomeMsg.cmac);
			auto pKey = pCrypto->decrypt(welcomeMsg.key);
			QVERIFY(!pKey.isEmpty());
			welcomeMsg.keyIndex = message.keyIndex;
			welcomeMsg.scheme = message.scheme;
			ok = true;

			//send key ack (verify later)
			connection->send(NewKeyAckMessage(message));
		}));

		//reconnect the partner and do a login with key updates
		partner->reconnect();
		QVERIFY(partnerConnection->waitForDisconnect());
		QVERIFY(server->waitForConnected(&partnerConnection));

		//send the identifiy message
		auto iMsg = IdentifyMessage::createRandom(20, rng);
		partnerConnection->send(iMsg);

		//wait for login message
		QVERIFY(partnerConnection->waitForSignedReply<LoginMessage>(pCrypto, [&](LoginMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			QCOMPARE(message.deviceId, partnerDevId);
			QCOMPARE(message.deviceName, partner->deviceName());
			ok = true;
		}));

		//send welcome reply
		partnerConnection->send(welcomeMsg);//no changes, but key update
		QVERIFY(partnerConnection->waitForReply<MacUpdateMessage>([&](MacUpdateMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, welcomeMsg.keyIndex);
			partner->cryptoController()->verifyEncryptionKeyCmac(pCrypto, *(pCrypto->cryptKey()), message.cmac);
			ok = true;
		}));

		//send the mac update
		partnerConnection->send(MacUpdateAckMessage());

		//final: check if both have the new index
		QCOMPARE(remote->cryptoController()->keyIndex(), welcomeMsg.keyIndex);
		QCOMPARE(partner->cryptoController()->keyIndex(), welcomeMsg.keyIndex);

		QVERIFY(errorSpy.isEmpty());
		QVERIFY(partnerErrorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testMacLost()
{
	testKeyUpdate(false);

	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);

	try {
		//init a connection
		partner->reconnect();
		auto pCrypto = partner->cryptoController()->crypto();
		QVERIFY(server->waitForConnected(&partnerConnection));

		//send the identifiy message
		auto iMsg = IdentifyMessage::createRandom(20, rng);
		partnerConnection->send(iMsg);

		//wait for login message
		QVERIFY(partnerConnection->waitForSignedReply<LoginMessage>(pCrypto, [&](LoginMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			//skip rest
			ok = true;
		}));

		//send welcome reply
		partnerConnection->send(WelcomeMessage(false));//no changes etc.

		//wait for the macupdate
		QVERIFY(partnerConnection->waitForReply<MacUpdateMessage>([&](MacUpdateMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, partner->cryptoController()->keyIndex());
			partner->cryptoController()->verifyEncryptionKeyCmac(pCrypto, *(pCrypto->cryptKey()), message.cmac);
			//skip rest
			ok = true;
		}));

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testResetAccount()
{
	QSignalSpy errorSpy(partner, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(partner, &RemoteConnector::remoteEvent);

	try {
		//assume already logged in
		QVERIFY(partnerConnection);
		auto pCrypto = partner->cryptoController()->crypto();
		auto oldFp = partner->cryptoController()->fingerprint();

		//perform reset and wait for message
		partner->resetAccount(true);
		QVERIFY(partnerConnection->waitForReply<RemoveMessage>([&](RemoveMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			ok = true;
		}));

		//ack remove
		partnerConnection->send(RemoveAckMessage(partnerDevId));
		QVERIFY(partnerConnection->waitForDisconnect());
		QVERIFY(eventSpy.size() > 0);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);

		//wait for reconnected
		QVERIFY(server->waitForConnected(&partnerConnection));

		//send the identifiy message
		auto iMsg = IdentifyMessage::createRandom(20, rng);
		partnerConnection->send(iMsg);

		//wait for register message
		QVERIFY(partnerConnection->waitForSignedReply<RegisterMessage>(pCrypto, [&](RegisterMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			QCOMPARE(message.deviceName, partner->deviceName());
			AsymmetricCryptoInfo cInfo(rng,
				message.signAlgorithm,
				message.signKey,
				message.cryptAlgorithm,
				message.cryptKey);
			QCOMPARE(cInfo.ownFingerprint(), partner->cryptoController()->fingerprint());
			QCOMPARE(message.cmac, partner->cryptoController()->generateEncryptionKeyCmac());
			QVERIFY(partner->cryptoController()->fingerprint() != oldFp);
			ok = true;
		}));

		//send account reply
		partnerDevId = QUuid::createUuid();
		partnerConnection->send(AccountMessage(partnerDevId));
		QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 2);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteReady);

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testUnexpectedMessage_data()
{
	QTest::addColumn<QSharedPointer<Message>>("message");
	QTest::addColumn<bool>("whenIdle");

	QTest::newRow("IdentifyMessage") << createFn<IdentifyMessage>([&](){ return IdentifyMessage::createRandom(10, rng); })
									 << true;
	QTest::newRow("AccountMessage") << create<AccountMessage>(devId)
									<< true;
	QTest::newRow("WelcomeMessage") << create<WelcomeMessage>(false)
									<< true;
	QTest::newRow("GrantMessage") << create<GrantMessage>(AcceptMessage(partnerDevId))
								  << true;
	QTest::newRow("ChangeAckMessage") << create<ChangeAckMessage>(ChangeMessage("test"))
									  << false;
	QTest::newRow("DeviceChangeAckMessage") << create<DeviceChangeAckMessage>(DeviceChangeMessage("test", partnerDevId))
											<< false;
	QTest::newRow("ChangedMessage") << create<ChangedMessage>()
									<< false;
	QTest::newRow("ChangedInfoMessage") << create<ChangedInfoMessage>(42)
										<< false;
	QTest::newRow("LastChangedMessage") << create<LastChangedMessage>()
										<< false;
	QTest::newRow("DevicesMessage") << create<DevicesMessage>()
									<< false;
	QTest::newRow("RemoveAckMessage") << create<RemoveAckMessage>(partnerDevId)
									  << false;
	QTest::newRow("ProofMessage") << create<ProofMessage>(AccessMessage(), partnerDevId)
								  << false;
	QTest::newRow("AcceptAckMessage") << create<AcceptAckMessage>(partnerDevId)
									  << false;
	QTest::newRow("MacUpdateAckMessage") << create<MacUpdateAckMessage>()
										 << false;
	QTest::newRow("DeviceKeysMessage") << create<DeviceKeysMessage>(7)
									   << false;
	QTest::newRow("NewKeyAckMessage") << create<NewKeyAckMessage>()
									  << false;
}

void TestRemoteConnector::testUnexpectedMessage()
{
	QFETCH(QSharedPointer<Message>, message);
	QFETCH(bool, whenIdle);

	QVERIFY(message);

	try {
		//assume already logged in
		QVERIFY(remote);
		auto pCrypto = remote->cryptoController()->crypto();

		if(!whenIdle) {
			remote->reconnect();
			QVERIFY(connection->waitForDisconnect());
			QVERIFY(server->waitForConnected(&connection));

			//send the identifiy message
			auto iMsg = IdentifyMessage::createRandom(20, rng);
			connection->send(iMsg);

			//wait for login message
			QVERIFY(connection->waitForSignedReply<LoginMessage>(pCrypto, [&](LoginMessage message, bool &ok) {
				QCOMPARE(message.nonce, iMsg.nonce);
				//skip checks
				ok = true;
			}));
		}

		QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
		QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

		connection->send(*message);
		QVERIFY(connection->waitForDisconnect());
		if(eventSpy.isEmpty())
			QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);

		QVERIFY(server->waitForConnected(&connection, 10000));//5 extra secs because of retry
		if(eventSpy.isEmpty())
			QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);
		//ignore the connection, simply login as usual

		testLogin(false);

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testUnknownMessage()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		//assume already logged in
		QVERIFY(remote);

		connection->send(ChangedAckMessage(42));//not known by the remcon
		QVERIFY(connection->waitForDisconnect());
		if(eventSpy.isEmpty())
			QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);

		QVERIFY(server->waitForConnected(&connection, 10000));//5 extra secs because of retry
		if(eventSpy.isEmpty())
			QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);
		//ignore the connection, simply login as usual

		testLogin(false);

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testBrokenMessage()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		//assume already logged in
		QVERIFY(remote);

		connection->sendBytes("\x00\x00\x00\x05Proof\x00followed_by_gibberish");//header looks right, but not the rest
		QVERIFY(connection->waitForDisconnect());
		if(eventSpy.isEmpty())
			QVERIFY(eventSpy.wait());
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
		QCOMPARE(errorSpy.size(), 1);

		//will not reconnect, so just login
		QVERIFY(!eventSpy.wait());
		testLogin(false, false);

	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

#ifdef TEST_PING_MSG
void TestRemoteConnector::testPingMessages()
{
	QSignalSpy errorSpy(partner, &RemoteConnector::controllerError);

	try {
		//assume already logged in
		QVERIFY(connection);
		QVERIFY(partnerConnection);

		QVERIFY(connection->waitForPing());
		connection->sendPing();
		QVERIFY(connection->waitForPing());

		//wait for partner to reconnect
		QVERIFY(partnerConnection->waitForPing());
		QVERIFY(partnerConnection->waitForDisconnect());

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}
#endif

void TestRemoteConnector::testRetry()
{
	QSignalSpy errorSpy(partner, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(partner, &RemoteConnector::remoteEvent);

	try {
		//assume already logged in
		QVERIFY(partnerConnection);

		//reconnect, the wait 3 times to check timouts working
		qDebug() << "starting reconnect";
		partner->reconnect();
		for(auto i = 0; i < 3; i++) {
			qDebug() << "wait for connect" << i;
			QVERIFY(server->waitForConnected(&partnerConnection, 12000*(i+1)));
			if(eventSpy.size() != 2)
				QVERIFY(eventSpy.wait());
			QCOMPARE(eventSpy.size(), 2);
			QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
			QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteConnecting);
			partnerConnection->close();
		}
		partner->disconnectRemote();

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testFinalize()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);
	QSignalSpy finalSpy(remote, &RemoteConnector::finalized);

	try {
		//assume already logged in
		QVERIFY(connection);

		//finalize the connection
		remote->finalize();

		QVERIFY(finalSpy.wait());
		QCOMPARE(finalSpy.size(), 1);
		QCOMPARE(eventSpy.size(), 1);
		QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);

		QVERIFY(connection->waitForDisconnect());

		QVERIFY(errorSpy.isEmpty());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

std::tuple<RemoteConnector*, QUuid> TestRemoteConnector::createSetup(const QString &setupName)
{
	Setup setup;
	TestLib::setup(setup);
	setup.setRemoteConfiguration(server->url())
			.setLocalDir(setup.localDir() + QStringLiteral("/") + setupName)
			.create(setupName);

	MockConnection *tCon = nullptr;
	if(!server->waitForConnected(&tCon))
		throw Exception(setupName, QStringLiteral("Client did not connect to server"));
	SetupPrivate::engine(setupName)->remoteConnector()->disconnectRemote();
	if(!tCon->waitForDisconnect())
		throw Exception(setupName, QStringLiteral("Client did not disconnect from server"));

	auto remote = new RemoteConnector(DefaultsPrivate::obtainDefaults(setupName), this);
	auto devId = QUuid::createUuid();
	return std::make_tuple(remote, devId);
}

template<typename TMessage, typename... Args>
inline QSharedPointer<Message> TestRemoteConnector::create(Args... args)
{
	return QSharedPointer<TMessage>::create(args...).template staticCast<Message>();
}

template<typename TMessage>
inline QSharedPointer<Message> TestRemoteConnector::createFn(const std::function<TMessage()> &fn)
{
	if(fn)
		return QSharedPointer<TMessage>::create(fn()).template staticCast<Message>();
	else
		return QSharedPointer<TMessage>::create().template staticCast<Message>();
}

QTEST_MAIN(TestRemoteConnector)

#include "tst_remoteconnector.moc"
