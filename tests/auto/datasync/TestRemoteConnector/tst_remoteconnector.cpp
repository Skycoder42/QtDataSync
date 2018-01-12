#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <QLoggingCategory>
#include <cryptopp/osrng.h>
#include <QtDataSync/private/remoteconnector_p.h>
#include <QtDataSync/private/defaults_p.h>
#include <QtDataSync/private/setup_p.h>

#include <QtDataSync/private/loginmessage_p.h>
#include <QtDataSync/private/syncmessage_p.h>

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
	void testDownloading();
	void testResync();

	void testAddDeviceUntrusted();
	void testAddDeviceTrusted();
	void testListAndRemoveDevices();

	//void testKeyUpdate();
	//void testResetAccount();
	//void testFinalize();

	//TODO test error message and unexpected messages

private:
	MockServer *server;
	MockConnection *currentConnection;
	CryptoPP::AutoSeededRandomPool rng;
	RemoteConnector *remote	;
	QUuid devId;

	QString setup1;
	RemoteConnector *partner1;
	QUuid devId1;

	std::tuple<RemoteConnector *, QUuid> createSetup(const QString &setupName);
};

void TestRemoteConnector::initTestCase()
{
	QLoggingCategory::setFilterRules(QStringLiteral("qtdatasync.*.debug=true"));

	qRegisterMetaType<RemoteConnector::RemoteEvent>("RemoteEvent");
	qRegisterMetaType<DeviceInfo>("DeviceInfo");
	qRegisterMetaType<QList<DeviceInfo>>("QList<DeviceInfo>");

	remote = nullptr;
	partner1 = nullptr;

	try {
		TestLib::init();

		currentConnection = nullptr;
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
	if(partner1) {
		delete partner1;
		partner1 = nullptr;
	}
	Setup::removeSetup(DefaultSetup, true);
	if(!setup1.isEmpty())
		Setup::removeSetup(setup1, true);
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
	if(eventSpy.isEmpty())
		QVERIFY(eventSpy.wait());
	QCOMPARE(eventSpy.size(), 1);
	QCOMPARE(eventSpy.takeFirst()[0].toInt(), RemoteConnector::RemoteDisconnected);
	QCOMPARE(errorSpy.size(), 1);
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
			QCOMPARE(message.deviceName, remote->deviceName());
			AsymmetricCryptoInfo cInfo(crypto->rng(),
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

void TestRemoteConnector::testLogin(bool hasChanges)
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
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
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestRemoteConnector::testLoginWithChanges()
{
	testLogin(true);
}

void TestRemoteConnector::testUploading()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy uploadSpy(remote, &RemoteConnector::uploadDone);

	try {
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
		MockConnection *connection = currentConnection;
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

void TestRemoteConnector::testResync()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy eventSpy(remote, &RemoteConnector::remoteEvent);

	try {
		//assume already logged in
		MockConnection *connection = currentConnection;
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

void TestRemoteConnector::testAddDeviceUntrusted()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy loginSpy(remote, &RemoteConnector::loginRequested);
	QSignalSpy grantedSpy(remote, &RemoteConnector::accountAccessGranted);

	try {
		//assume already logged in
		MockConnection *connection = currentConnection;
		QVERIFY(connection);
		auto crypto = remote->cryptoController()->crypto();

		//create a second setup...
		setup1 = QStringLiteral("partner1");
		std::tie(partner1, devId1) = createSetup(setup1);
		QSignalSpy partnerErrorSpy(partner1, &RemoteConnector::controllerError);
		QSignalSpy partnerImportSpy(partner1, &RemoteConnector::importCompleted);

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
		partner1->prepareImport(exportData, CryptoPP::SecByteBlock());
		partner1->initialize({});
		auto partnerCrypto = partner1->cryptoController()->crypto();
		MockConnection *partnerCon = nullptr;
		QVERIFY(server->waitForConnected(&partnerCon));
		auto iMsg = IdentifyMessage::createRandom(10, rng);
		partnerCon->send(iMsg);

		//wait for the access message
		QVERIFY(partnerCon->waitForSignedReply<AccessMessage>(partnerCrypto, [&](AccessMessage message, bool &ok) {
			QCOMPARE(message.nonce, iMsg.nonce);
			QCOMPARE(message.deviceName, partner1->deviceName());
			AsymmetricCryptoInfo cInfo(partnerCrypto->rng(),
				message.signAlgorithm,
				message.signKey,
				message.cryptAlgorithm,
				message.cryptKey);
			QCOMPARE(cInfo.ownFingerprint(), partner1->cryptoController()->fingerprint());

			QCOMPARE(message.pNonce, exportData.pNonce);
			QCOMPARE(message.partnerId, exportData.partnerId);
			QCOMPARE(message.macscheme, exportData.scheme);
			QCOMPARE(message.cmac, exportData.cmac);
			QVERIFY(message.trustmac.isEmpty());
			ok = true;

			//Send from here because message needed
			connection->send(ProofMessage(message, devId1));
		}));

		//Send the proof the main con and wait for login request
		QVERIFY(loginSpy.wait());
		QCOMPARE(loginSpy.size(), 1);
		auto devInfo = loginSpy.takeFirst()[0].value<DeviceInfo>();
		QCOMPARE(devInfo.deviceId(), devId1);
		QCOMPARE(devInfo.name(), partner1->deviceName());
		QCOMPARE(devInfo.fingerprint(), partner1->cryptoController()->fingerprint());

		//accept the request
		remote->loginReply(devId1, true);
		QVERIFY(connection->waitForSignedReply<AcceptMessage>(crypto, [&](AcceptMessage message, bool &ok) {
			QCOMPARE(message.deviceId, devId1);
			QCOMPARE(message.index, remote->cryptoController()->keyIndex());
			partnerCrypto->decrypt(message.secret); //make shure works without exception
			ok = true;

			//Send from here because message needed
			partnerCon->send(GrantMessage(devId1, message));
		}));

		//verify preperations are done
		if(grantedSpy.isEmpty())
			QVERIFY(grantedSpy.wait());
		QCOMPARE(grantedSpy.size(), 1);
		QCOMPARE(grantedSpy.takeFirst()[0].toUuid(), devId1);

		//verify grant message received successfully
		QVERIFY(partnerImportSpy.wait());
		QCOMPARE(partnerImportSpy.size(), 1);

		//wait for mac update message
		QVERIFY(partnerCon->waitForReply<MacUpdateMessage>([&](MacUpdateMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, partner1->cryptoController()->keyIndex());
			QCOMPARE(message.cmac, partner1->cryptoController()->generateEncryptionKeyCmac());
			ok = true;
		}));

		//send macack
		partnerCon->send(MacUpdateAckMessage());
		//cannot really check if received... but simple enough to assume worked
		//TODO test mac resending

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
		MockConnection *connection = currentConnection;
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
			AsymmetricCryptoInfo cInfo(partnerCrypto->rng(),
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
			partnerCon->send(GrantMessage(devId2, message));
		}));

		//verify preperations are done
		if(grantedSpy.isEmpty())
			QVERIFY(grantedSpy.wait());
		QCOMPARE(grantedSpy.size(), 1);
		QCOMPARE(grantedSpy.takeFirst()[0].toUuid(), devId2);

		//verify grant message received successfully
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
		//TODO test mac resending

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

void TestRemoteConnector::testListAndRemoveDevices()
{
	QSignalSpy errorSpy(remote, &RemoteConnector::controllerError);
	QSignalSpy devicesSpy(remote, &RemoteConnector::devicesListed);

	try {
		//assume already logged in
		MockConnection *connection = currentConnection;
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
								   devId1,
								   partner1->deviceName(),
								   partner1->cryptoController()->fingerprint()
							   ));
		connection->send(message);
		QVERIFY(devicesSpy.wait());
		QCOMPARE(devicesSpy.size(), 1);
		auto devList = devicesSpy.takeFirst()[0].value<QList<DeviceInfo>>();
		QCOMPARE(devList.size(), 1);
		QCOMPARE(devList[0].deviceId(), devId1);
		QCOMPARE(devList[0].name(), partner1->deviceName());
		QCOMPARE(devList[0].fingerprint(), partner1->cryptoController()->fingerprint());

		//remote partner from devices
		remote->removeDevice(devId1);
		QVERIFY(connection->waitForReply<RemoveMessage>([&](RemoveMessage message, bool &ok) {
			QCOMPARE(message.deviceId, devId1);
			ok = true;
		}));

		//send the reply
		connection->send(RemoveAckMessage(devId1));
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

QTEST_MAIN(TestRemoteConnector)

#include "tst_remoteconnector.moc"
