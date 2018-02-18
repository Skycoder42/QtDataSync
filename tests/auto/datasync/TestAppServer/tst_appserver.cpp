#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QProcess>
#include <testlib.h>
#include <mockclient.h>

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <signal.h>
#elif Q_OS_WIN
#include <qt_windows.h>
#endif

#include <QtDataSync/private/identifymessage_p.h>
#include <QtDataSync/private/registermessage_p.h>
#include <QtDataSync/private/accountmessage_p.h>
#include <QtDataSync/private/loginmessage_p.h>
#include <QtDataSync/private/welcomemessage_p.h>
#include <QtDataSync/private/accessmessage_p.h>
#include <QtDataSync/private/proofmessage_p.h>
#include <QtDataSync/private/grantmessage_p.h>
#include <QtDataSync/private/macupdatemessage_p.h>
#include <QtDataSync/private/changemessage_p.h>
#include <QtDataSync/private/changedmessage_p.h>
#include <QtDataSync/private/syncmessage_p.h>
#include <QtDataSync/private/devicechangemessage_p.h>
#include <QtDataSync/private/keychangemessage_p.h>
#include <QtDataSync/private/devicekeysmessage_p.h>
#include <QtDataSync/private/newkeymessage_p.h>
#include <QtDataSync/private/devicesmessage_p.h>
#include <QtDataSync/private/removemessage_p.h>

using namespace QtDataSync;

Q_DECLARE_METATYPE(QSharedPointer<Message>)

class TestAppServer : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testStart();

	void testInvalidVersion();
	void testInvalidRegisterNonce();
	void testInvalidRegisterSignature();
	void testRegister();
	void testInvalidLoginNonce();
	void testInvalidLoginSignature();
	void testInvalidLoginDevId();
	void testLogin();

	void testAddDevice();
	void testInvalidAccessNonce();
	void testInvalidAccessSignature();
	void testDenyAddDevice();
	void testOfflineAddDevice();
	void testInvalidAcceptSignature();
	void testAddDeviceInvalidKeyIndex();
	void testSendDoubleAccept();

	void testChangeUpload();
	void testChangeDownloadOnLogin();
	void testLiveChanges();
	void testSyncCommand();
	void testDeviceUploading();

	void testChangeKey();
	void testChangeKeyInvalidIndex();
	void testChangeKeyDuplicate();
	void testChangeKeyPendingChanges();
	void testAddNewKeyInvalidIndex();
	void testAddNewKeyInvalidSignature();
	void testAddNewKeyPendingChanges();
	void testKeyChangeNoAck();

	void testListAndRemoveDevices();

	void testUnexpectedMessage_data();
	void testUnexpectedMessage();
	void testUnknownMessage();
	void testBrokenMessage();

#ifdef TEST_PING_MSG
	void testPingMessages();
#endif

	void testRemoveSelf();
	void testStop();

private:
	QProcess *server;

	MockClient *client;
	QString devName;
	QUuid devId;
	ClientCrypto *crypto;

	MockClient *partner;
	QString partnerName;
	QUuid partnerDevId;
	ClientCrypto *partnerCrypto;

	void testAddDevice(MockClient *&partner, QUuid &partnerDevId, bool keepPartner = false);

	void clean(bool disconnect = true);
	void clean(MockClient *&client, bool disconnect = true);

	template <typename TMessage, typename... Args>
	inline QSharedPointer<Message> create(Args... args);
	template <typename TMessage, typename... Args>
	inline QSharedPointer<Message> createNonced(Args... args);
};

void TestAppServer::initTestCase()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
	QByteArray confPath { SETUP_FILE };
	QVERIFY(QFile::exists(QString::fromUtf8(confPath)));
	qputenv("QDSAPP_CONFIG_FILE", confPath);

#ifdef Q_OS_UNIX
	QString binPath { QStringLiteral(BUILD_BIN_DIR "qdsappd") };
#elif Q_OS_WIN
	QString binPath { QStringLiteral(BUILD_BIN_DIR "qdsappsvc") };
#else
	QString binPath { QStringLiteral(BUILD_BIN_DIR "qdsapp") };
#endif
	QVERIFY(QFile::exists(binPath));

	server = new QProcess(this);
	server->setProgram(binPath);
	server->setProcessChannelMode(QProcess::ForwardedErrorChannel);

	try {
		client = nullptr;
		devName = QStringLiteral("client");
		crypto = new ClientCrypto(this);
		crypto->generate(Setup::RSA_PSS_SHA3_512, 2048,
						 Setup::RSA_OAEP_SHA3_512, 2048);

		partner = nullptr;
		partnerName = QStringLiteral("partner");
		partnerCrypto = new ClientCrypto(this);
		partnerCrypto->generate(Setup::RSA_PSS_SHA3_512, 2048,
								Setup::RSA_OAEP_SHA3_512, 2048);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::cleanupTestCase()
{
	clean();
	clean(partner);
	if(server->isOpen()) {
		server->kill();
		QVERIFY(server->waitForFinished(5000));
	}
	server->deleteLater();
}

void TestAppServer::testStart()
{
	server->start();
	QVERIFY(server->waitForStarted(5000));
	QVERIFY(!server->waitForFinished(5000));
}

void TestAppServer::testInvalidVersion()
{
	try {
		//establish connection
		client = new MockClient(this);
		QVERIFY(client->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a register message with invalid version
		RegisterMessage reply {
			devName,
			mNonce,
			crypto->signKey(),
			crypto->cryptKey(),
			crypto,
			"cmac"
		};
		reply.protocolVersion = QVersionNumber(0,5);
		client->send(reply);

		//wait for the error
		QVERIFY(client->waitForError(ErrorMessage::IncompatibleVersionError));
		clean();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testInvalidRegisterNonce()
{
	try {
		//establish connection
		client = new MockClient(this);
		QVERIFY(client->waitForConnected());

		//wait for identify message
		QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			ok = true;
		}));

		//send back a register message with invalid nonce
		client->send(RegisterMessage {
						 devName,
						 "not the original nonce, but still long enough to be considered one",
						 crypto->signKey(),
						 crypto->cryptKey(),
						 crypto,
						 "cmac"
					 });

		//wait for the error
		QVERIFY(client->waitForError(ErrorMessage::ClientError));
		clean();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testInvalidRegisterSignature()
{
	try {
		//establish connection
		client = new MockClient(this);
		QVERIFY(client->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a register message with invalid signature
		QByteArray sData = RegisterMessage {
					 devName,
					 mNonce,
					 crypto->signKey(),
					 crypto->cryptKey(),
					 crypto,
					 "cmac"
				 }.serializeSigned(crypto->privateSignKey(), crypto->rng(), crypto);
		sData[sData.size() - 1] = sData[sData.size() - 1] + 'x';
		client->sendBytes(sData); //message + fake signature

		//wait for the error
		QVERIFY(client->waitForError(ErrorMessage::AuthenticationError));
		clean();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testRegister()
{
	try {
		//establish connection
		client = new MockClient(this);
		QVERIFY(client->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid register message
		client->sendSigned(RegisterMessage {
							   devName,
							   mNonce,
							   crypto->signKey(),
							   crypto->cryptKey(),
							   crypto,
							   devId.toByteArray() //dummy cmac
						   }, crypto);

		//wait for the account message
		QVERIFY(client->waitForReply<AccountMessage>([&](AccountMessage message, bool &ok) {
			devId = message.deviceId;
			ok = true;
		}));

		clean();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testInvalidLoginNonce()
{
	try {
		//establish connection
		client = new MockClient(this);
		QVERIFY(client->waitForConnected());

		//wait for identify message
		QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			ok = true;
		}));

		//send back a valid login message
		client->sendSigned(LoginMessage {
							   devId,
							   devName,
							   "not the original nonce, but still long enough to be considered one",
						   }, crypto);

		//wait for the error
		QVERIFY(client->waitForError(ErrorMessage::ClientError));
		clean();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testInvalidLoginSignature()
{
	try {
		//establish connection
		client = new MockClient(this);
		QVERIFY(client->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message
		QByteArray sData = LoginMessage {
			devId,
			devName,
			mNonce
		}.serializeSigned(crypto->privateSignKey(), crypto->rng(), crypto);
		sData[sData.size() - 1] = sData[sData.size() - 1] + 'x';
		client->sendBytes(sData); //message + fake signature

		//wait for the error
		QVERIFY(client->waitForError(ErrorMessage::AuthenticationError));
		clean();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testInvalidLoginDevId()
{
	try {
		//establish connection
		client = new MockClient(this);
		QVERIFY(client->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message
		client->sendSigned(LoginMessage {
							   QUuid::createUuid(), //wrong devId
							   devName,
							   mNonce
						   }, crypto);

		//wait for the error
		QVERIFY(client->waitForError(ErrorMessage::AuthenticationError));
		clean();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testLogin()
{
	try {
		//establish connection
		client = new MockClient(this);
		QVERIFY(client->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message
		client->sendSigned(LoginMessage {
							   devId,
							   devName,
							   mNonce
						   }, crypto);

		//wait for the account message
		QVERIFY(client->waitForReply<WelcomeMessage>([&](WelcomeMessage message, bool &ok) {
			QVERIFY(!message.hasChanges);
			QCOMPARE(message.keyIndex, 0u);
			QVERIFY(message.scheme.isNull());
			QVERIFY(message.key.isNull());
			QVERIFY(message.cmac.isNull());
			ok = true;
		}));

		//keep session active
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testAddDevice()
{
	testAddDevice(partner, partnerDevId);
}

void TestAppServer::testAddDevice(MockClient *&partner, QUuid &partnerDevId, bool keepPartner) //not executed as test
{
	QByteArray pNonce = "partner_nonce";
	QByteArray macscheme = "macscheme";
	QByteArray cmac = "cmac";
	QByteArray trustmac = "trustmac";
	quint32 keyIndex = 0;
	QByteArray keyScheme = "keyScheme";
	QByteArray keySecret = "keySecret";

	try {
		QVERIFY(client);
		//establish connection
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//Send access message
		partner->sendSigned(AccessMessage {
								partnerName,
								mNonce,
								partnerCrypto->signKey(),
								partnerCrypto->cryptKey(),
								partnerCrypto,
								pNonce,
								devId,
								macscheme,
								cmac,
								trustmac
							}, partnerCrypto);

		//wait for proof message on client
		QVERIFY(client->waitForReply<ProofMessage>([&](ProofMessage message, bool &ok) {
			QCOMPARE(message.pNonce, pNonce);
			QVERIFY(!message.deviceId.isNull());
			partnerDevId = message.deviceId;
			QCOMPARE(message.deviceName, partnerName);
			QCOMPARE(message.macscheme, macscheme);
			QCOMPARE(message.cmac, cmac);
			QCOMPARE(message.trustmac, trustmac);
			AsymmetricCryptoInfo cInfo {crypto->rng(),
				message.signAlgorithm,
				message.signKey,
				message.cryptAlgorithm,
				message.cryptKey
			};
			QCOMPARE(cInfo.ownFingerprint(), partnerCrypto->ownFingerprint());
			ok = true;
		}));

		//accept the proof
		AcceptMessage accMsg { partnerDevId };
		accMsg.index = keyIndex;
		accMsg.scheme = keyScheme;
		accMsg.secret = keySecret;
		client->sendSigned(accMsg, crypto);
		QVERIFY(client->waitForReply<AcceptAckMessage>([&](AcceptAckMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			ok = true;
		}));

		//wait for granted
		QVERIFY(partner->waitForReply<GrantMessage>([&](GrantMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			QCOMPARE(message.index, keyIndex);
			QCOMPARE(message.scheme, keyScheme);
			QCOMPARE(message.secret, keySecret);
			ok = true;
		}));

		//send mac update
		partner->send(MacUpdateMessage {
						  keyIndex,
						  partnerDevId.toByteArray() //dummy cmac
					  });

		//wait for mac ack
		QVERIFY(partner->waitForReply<MacUpdateAckMessage>([&](MacUpdateAckMessage message, bool &ok) {
			Q_UNUSED(message);
			ok = true;
		}));

		if(!keepPartner)
			clean(partner);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testInvalidAccessNonce()
{
	try {
		QVERIFY(client);

		MockClient *partner = nullptr;
		//use crypto and name from real partner

		//establish connection
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			ok = true;
		}));

		//Send access message
		partner->sendSigned(AccessMessage {
								partnerName,
								"not the original nonce, but still long enough to be considered one",
								partnerCrypto->signKey(),
								partnerCrypto->cryptKey(),
								partnerCrypto,
								"pNonce",
								devId,
								"macscheme",
								"cmac",
								"trustmac"
							}, partnerCrypto);

		QVERIFY(partner->waitForError(ErrorMessage::ClientError));
		clean(partner);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testInvalidAccessSignature()
{
	try {
		QVERIFY(client);

		MockClient *partner = nullptr;
		//use crypto and name from real partner

		//establish connection
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//Send access message
		QByteArray sData = AccessMessage {
			partnerName,
			mNonce,
			partnerCrypto->signKey(),
			partnerCrypto->cryptKey(),
			partnerCrypto,
			"pNonce",
			devId,
			"macscheme",
			"cmac",
			"trustmac"
		}.serializeSigned(partnerCrypto->privateSignKey(), partnerCrypto->rng(), partnerCrypto);
		sData[sData.size() - 1] = sData[sData.size() - 1] + 'x';
		partner->sendBytes(sData); //message + fake signature

		QVERIFY(partner->waitForError(ErrorMessage::AuthenticationError));
		clean(partner);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testDenyAddDevice()
{
	QByteArray pNonce = "partner_nonce";
	QByteArray macscheme = "macscheme";
	QByteArray cmac = "cmac";
	QByteArray trustmac = "trustmac";

	try {
		QVERIFY(client);

		MockClient *partner = nullptr;
		QUuid partnerDevId;
		//use crypto and name from real partner

		//establish connection
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//Send access message
		partner->sendSigned(AccessMessage {
								partnerName,
								mNonce,
								partnerCrypto->signKey(),
								partnerCrypto->cryptKey(),
								partnerCrypto,
								pNonce,
								devId,
								macscheme,
								cmac,
								trustmac
							}, partnerCrypto);

		//wait for proof message on client
		QVERIFY(client->waitForReply<ProofMessage>([&](ProofMessage message, bool &ok) {
			QCOMPARE(message.pNonce, pNonce);
			QVERIFY(!message.deviceId.isNull());
			partnerDevId = message.deviceId;
			QCOMPARE(message.deviceName, partnerName);
			QCOMPARE(message.macscheme, macscheme);
			QCOMPARE(message.cmac, cmac);
			QCOMPARE(message.trustmac, trustmac);
			AsymmetricCryptoInfo cInfo {crypto->rng(),
				message.signAlgorithm,
				message.signKey,
				message.cryptAlgorithm,
				message.cryptKey
			};
			QCOMPARE(cInfo.ownFingerprint(), partnerCrypto->ownFingerprint());
			ok = true;
		}));

		//accept the proof
		client->send(DenyMessage { partnerDevId });

		QVERIFY(partner->waitForError(ErrorMessage::AccessError));
		clean(partner);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testOfflineAddDevice()
{
	try {
		QVERIFY(client);
		clean(client);

		MockClient *partner = nullptr;
		//use crypto and name from real partner

		//establish connection
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//Send access message for offline device
		partner->sendSigned(AccessMessage {
								partnerName,
								mNonce,
								partnerCrypto->signKey(),
								partnerCrypto->cryptKey(),
								partnerCrypto,
								"pNonce",
								devId,
								"macscheme",
								"cmac",
								"trustmac"
							}, partnerCrypto);

		QVERIFY(partner->waitForError(ErrorMessage::AccessError));
		clean(partner);

		//reconnect main device
		testLogin();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testInvalidAcceptSignature()
{
	try {
		QVERIFY(client);

		//invalid accept
		AcceptMessage accMsg { QUuid::createUuid() };
		accMsg.index = 42;
		accMsg.scheme = "keyScheme";
		accMsg.secret = "keySecret";
		auto sData = accMsg.serializeSigned(crypto->privateSignKey(), crypto->rng(), crypto);
		sData[sData.size() - 1] = sData[sData.size() - 1] + 'x';
		client->sendBytes(sData); //message + fake signature

		QVERIFY(client->waitForError(ErrorMessage::AuthenticationError));
		clean(client);

		testLogin();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testAddDeviceInvalidKeyIndex()
{
	QByteArray pNonce = "partner_nonce";
	QByteArray macscheme = "macscheme";
	QByteArray cmac = "cmac";
	QByteArray trustmac = "trustmac";
	quint32 keyIndex = 5; //fake key index
	QByteArray keyScheme = "keyScheme";
	QByteArray keySecret = "keySecret";

	try {
		QVERIFY(client);
		MockClient *partner = nullptr;
		QUuid partnerDevId;

		//establish connection
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//Send access message
		partner->sendSigned(AccessMessage {
								partnerName,
								mNonce,
								partnerCrypto->signKey(),
								partnerCrypto->cryptKey(),
								partnerCrypto,
								pNonce,
								devId,
								macscheme,
								cmac,
								trustmac
							}, partnerCrypto);

		//wait for proof message on client
		QVERIFY(client->waitForReply<ProofMessage>([&](ProofMessage message, bool &ok) {
			QCOMPARE(message.pNonce, pNonce);
			QVERIFY(!message.deviceId.isNull());
			partnerDevId = message.deviceId;
			QCOMPARE(message.deviceName, partnerName);
			QCOMPARE(message.macscheme, macscheme);
			QCOMPARE(message.cmac, cmac);
			QCOMPARE(message.trustmac, trustmac);
			AsymmetricCryptoInfo cInfo {crypto->rng(),
				message.signAlgorithm,
				message.signKey,
				message.cryptAlgorithm,
				message.cryptKey
			};
			QCOMPARE(cInfo.ownFingerprint(), partnerCrypto->ownFingerprint());
			ok = true;
		}));

		//accept the proof
		AcceptMessage accMsg { partnerDevId };
		accMsg.index = keyIndex;
		accMsg.scheme = keyScheme;
		accMsg.secret = keySecret;
		client->sendSigned(accMsg, crypto);
		QVERIFY(client->waitForReply<AcceptAckMessage>([&](AcceptAckMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			ok = true;
		}));

		//wait for granted
		QVERIFY(partner->waitForReply<GrantMessage>([&](GrantMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			QCOMPARE(message.index, keyIndex);
			QCOMPARE(message.scheme, keyScheme);
			QCOMPARE(message.secret, keySecret);
			ok = true;
		}));

		//send mac update
		partner->send(MacUpdateMessage {
						  keyIndex,
						  partnerDevId.toByteArray() //dummy cmac
					  });

		QVERIFY(partner->waitForError(ErrorMessage::KeyIndexError));
		clean(partner);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testSendDoubleAccept()
{
	QByteArray pNonce = "partner_nonce";
	QByteArray macscheme = "macscheme";
	QByteArray cmac = "cmac";
	QByteArray trustmac = "trustmac";
	quint32 keyIndex = 0;
	QByteArray keyScheme = "keyScheme";
	QByteArray keySecret = "keySecret";

	try {
		QVERIFY(client);
		MockClient *partner = nullptr;
		QUuid partnerDevId;

		//establish connection
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//Send access message
		partner->sendSigned(AccessMessage {
								partnerName,
								mNonce,
								partnerCrypto->signKey(),
								partnerCrypto->cryptKey(),
								partnerCrypto,
								pNonce,
								devId,
								macscheme,
								cmac,
								trustmac
							}, partnerCrypto);

		//wait for proof message on client
		QVERIFY(client->waitForReply<ProofMessage>([&](ProofMessage message, bool &ok) {
			QCOMPARE(message.pNonce, pNonce);
			QVERIFY(!message.deviceId.isNull());
			partnerDevId = message.deviceId;
			QCOMPARE(message.deviceName, partnerName);
			QCOMPARE(message.macscheme, macscheme);
			QCOMPARE(message.cmac, cmac);
			QCOMPARE(message.trustmac, trustmac);
			AsymmetricCryptoInfo cInfo {crypto->rng(),
				message.signAlgorithm,
				message.signKey,
				message.cryptAlgorithm,
				message.cryptKey
			};
			QCOMPARE(cInfo.ownFingerprint(), partnerCrypto->ownFingerprint());
			ok = true;
		}));

		//accept the proof
		AcceptMessage accMsg { partnerDevId };
		accMsg.index = keyIndex;
		accMsg.scheme = keyScheme;
		accMsg.secret = keySecret;
		client->sendSigned(accMsg, crypto);
		QVERIFY(client->waitForReply<AcceptAckMessage>([&](AcceptAckMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			ok = true;
		}));

		//wait for granted
		QVERIFY(partner->waitForReply<GrantMessage>([&](GrantMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			QCOMPARE(message.index, keyIndex);
			QCOMPARE(message.scheme, keyScheme);
			QCOMPARE(message.secret, keySecret);
			ok = true;
		}));

		//send mac update
		partner->send(MacUpdateMessage {
						  keyIndex,
						  partnerDevId.toByteArray() //dummy cmac
					  });

		//wait for mac ack
		QVERIFY(partner->waitForReply<MacUpdateAckMessage>([&](MacUpdateAckMessage message, bool &ok) {
			Q_UNUSED(message);
			ok = true;
		}));

		//send another proof accept
		client->sendSigned(accMsg, crypto);
		QVERIFY(client->waitForNothing()); //no accept ack
		QVERIFY(partner->waitForNothing()); //no grant

		//clean partner
		clean(partner);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testChangeUpload()
{
	QByteArray dataId1 = "dataId1";
	QByteArray dataId2 = "dataId2";
	quint32 keyIndex = 0;
	QByteArray salt = "salt";
	QByteArray data = "data";

	try {
		QVERIFY(client);
		QVERIFY(!partner);

		//send an upload 1 and 2
		ChangeMessage changeMsg { dataId1 };
		changeMsg.keyIndex = keyIndex;
		changeMsg.salt = salt;
		changeMsg.data = data;
		client->send(changeMsg);
		changeMsg.dataId = dataId2;
		client->send(changeMsg);

		//wait for ack
		QVERIFY(client->waitForReply<ChangeAckMessage>([&](ChangeAckMessage message, bool &ok) {
			QCOMPARE(message.dataId, dataId1);
			ok = true;
		}));
		QVERIFY(client->waitForReply<ChangeAckMessage>([&](ChangeAckMessage message, bool &ok) {
			QCOMPARE(message.dataId, dataId2);
			ok = true;
		}));

		//send 1 again
		changeMsg.dataId = dataId1;
		client->send(changeMsg);

		//wait for ack
		QVERIFY(client->waitForReply<ChangeAckMessage>([&](ChangeAckMessage message, bool &ok) {
			QCOMPARE(message.dataId, dataId1);
			ok = true;
		}));
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testChangeDownloadOnLogin()
{
	quint32 keyIndex = 0;
	QByteArray salt = "salt";
	QByteArray data = "data";

	try {
		QVERIFY(client);
		QVERIFY(!partner);

		//establish connection
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message
		partner->sendSigned(LoginMessage {
							   partnerDevId,
							   partnerName,
							   mNonce
						   }, partnerCrypto);

		//wait for the account message
		QVERIFY(partner->waitForReply<WelcomeMessage>([&](WelcomeMessage message, bool &ok) {
			QVERIFY(message.hasChanges);//Must have changes now
			QCOMPARE(message.keyIndex, 0u);
			QVERIFY(message.scheme.isNull());
			QVERIFY(message.key.isNull());
			QVERIFY(message.cmac.isNull());
			ok = true;
		}));

		//wait for change info message
		quint64 dataId1 = 0;
		QVERIFY(partner->waitForReply<ChangedInfoMessage>([&](ChangedInfoMessage message, bool &ok) {
			QCOMPARE(message.changeEstimate, 2u);
			QCOMPARE(message.keyIndex, keyIndex);
			QCOMPARE(message.salt, salt);
			QCOMPARE(message.data, data);
			dataId1 = message.dataIndex;
			ok = true;
		}));

		//wait for the second message
		quint64 dataId2 = 0;
		QVERIFY(partner->waitForReply<ChangedMessage>([&](ChangedMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, keyIndex);
			QCOMPARE(message.salt, salt);
			QCOMPARE(message.data, data);
			dataId2 = message.dataIndex;
			ok = true;
		}));

		//send the first ack
		partner->send(ChangedAckMessage { dataId1 });
		QVERIFY(partner->waitForNothing());

		//send second ack
		partner->send(ChangedAckMessage { dataId2 });
		QVERIFY(partner->waitForReply<LastChangedMessage>([&](LastChangedMessage message, bool &ok) {
			Q_UNUSED(message)
			ok = true;
		}));
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testLiveChanges()
{
	QByteArray dataId1 = "dataId3";
	quint32 keyIndex = 0;
	QByteArray salt = "salt";
	QByteArray data = "data";

	try {
		QVERIFY(client);
		QVERIFY(partner);

		//send an upload
		ChangeMessage changeMsg { dataId1 };
		changeMsg.keyIndex = keyIndex;
		changeMsg.salt = salt;
		changeMsg.data = data;
		client->send(changeMsg);

		//wait for ack
		QVERIFY(client->waitForReply<ChangeAckMessage>([&](ChangeAckMessage message, bool &ok) {
			QCOMPARE(message.dataId, dataId1);
			ok = true;
		}));

		//wait for change info message
		quint64 dataId2 = 0;
		QVERIFY(partner->waitForReply<ChangedInfoMessage>([&](ChangedInfoMessage message, bool &ok) {
			QCOMPARE(message.changeEstimate, 1u);
			QCOMPARE(message.keyIndex, keyIndex);
			QCOMPARE(message.salt, salt);
			QCOMPARE(message.data, data);
			dataId2 = message.dataIndex;
			ok = true;
		}));

		//send the ack
		partner->send(ChangedAckMessage { dataId2 });
		QVERIFY(partner->waitForReply<LastChangedMessage>([&](LastChangedMessage message, bool &ok) {
			Q_UNUSED(message)
			ok = true;
		}));
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testSyncCommand()
{
	try {
		QVERIFY(client);

		//send sync
		client->send(SyncMessage{});

		//wait for last changed (as no changes are present)
		QVERIFY(client->waitForReply<LastChangedMessage>([&](LastChangedMessage message, bool &ok) {
			Q_UNUSED(message)
			ok = true;
		}));
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testDeviceUploading()
{
	QByteArray dataId1 = "dataId4";
	quint32 keyIndex = 0;
	QByteArray salt = "salt";
	QByteArray data = "data";

	try {
		QVERIFY(client);
		QVERIFY(partner);

		MockClient *partner3 = nullptr;
		QUuid partner3DevId;
		testAddDevice(partner3, partner3DevId, true);
		QVERIFY(partner3);

		//send an upload for partner only
		DeviceChangeMessage changeMsg { dataId1, partnerDevId };
		changeMsg.keyIndex = keyIndex;
		changeMsg.salt = salt;
		changeMsg.data = data;
		client->send(changeMsg);

		//wait for ack
		QVERIFY(client->waitForReply<DeviceChangeAckMessage>([&](DeviceChangeAckMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			QCOMPARE(message.dataId, dataId1);
			ok = true;
		}));

		//wait for change info message
		quint64 dataId2 = 0;
		QVERIFY(partner->waitForReply<ChangedInfoMessage>([&](ChangedInfoMessage message, bool &ok) {
			QCOMPARE(message.changeEstimate, 1u);
			QCOMPARE(message.keyIndex, keyIndex);
			QCOMPARE(message.salt, salt);
			QCOMPARE(message.data, data);
			dataId2 = message.dataIndex;
			ok = true;
		}));

		//send the ack
		partner->send(ChangedAckMessage { dataId2 });
		QVERIFY(partner->waitForReply<LastChangedMessage>([&](LastChangedMessage message, bool &ok) {
			Q_UNUSED(message)
			ok = true;
		}));

		//patner3: nothing
		QVERIFY(partner3->waitForNothing());

		clean(partner3);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testChangeKey()
{
	quint32 nextIndex = 1;
	QByteArray scheme = "scheme";
	QByteArray key = "key";
	QByteArray cmac = "cmac";

	try {
		QVERIFY(client);
		QVERIFY(partner);

		//Send the key change proposal
		client->send(KeyChangeMessage { nextIndex });
		QList<DeviceKeysMessage::DeviceKey> keys;
		QVERIFY(client->waitForReply<DeviceKeysMessage>([&](DeviceKeysMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, nextIndex);
			QVERIFY(!message.duplicated);
			QCOMPARE(message.devices.size(), 4); //testAddDevice, testAddDeviceInvalidKeyIndex, testSendDoubleAccept, testDeviceUploading
			keys = message.devices;
			ok = true;
		}));

		//verify the keys, extract partner 1 key
		DeviceKeysMessage::DeviceKey mKey;
		do {
			mKey = keys.takeFirst();
			if(partnerDevId == std::get<0>(mKey))
				break;
		} while(!keys.isEmpty());
		QCOMPARE(std::get<0>(mKey), partnerDevId);
		QCOMPARE(std::get<3>(mKey), partnerDevId.toByteArray()); //dummy cmac

		//send the actual key update message (ignoring the other 3)
		NewKeyMessage keyMsg;
		keyMsg.keyIndex = nextIndex;
		keyMsg.cmac = devName.toUtf8();
		keyMsg.scheme = scheme;
		keyMsg.deviceKeys.append(std::make_tuple(partnerDevId, key, cmac));
		client->sendSigned(keyMsg, crypto);

		//wait for ack
		QVERIFY(client->waitForReply<NewKeyAckMessage>([&](NewKeyAckMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, nextIndex);
			ok = true;
		}));

		//wait for partner to be disconnected
		QVERIFY(partner->waitForDisconnect());
		partner->deleteLater();
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message
		partner->sendSigned(LoginMessage {
							   partnerDevId,
							   partnerName,
							   mNonce
						   }, partnerCrypto);

		//wait for the account message
		QVERIFY(partner->waitForReply<WelcomeMessage>([&](WelcomeMessage message, bool &ok) {
			QVERIFY(!message.hasChanges);//Must have changes now
			QCOMPARE(message.keyIndex, nextIndex);
			QCOMPARE(message.scheme, scheme);
			QCOMPARE(message.key, key);
			QCOMPARE(message.cmac, cmac);
			ok = true;
		}));

		//update the mac accordingly
		partner->send(MacUpdateMessage { nextIndex, partnerName.toUtf8() });
		QVERIFY(partner->waitForReply<MacUpdateAckMessage>([&](MacUpdateAckMessage message, bool &ok) {
			Q_UNUSED(message)
			ok = true;
		}));

		//connect again, to make shure the ack was stored
		clean(partner);
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message
		partner->sendSigned(LoginMessage {
							   partnerDevId,
							   partnerName,
							   mNonce
						   }, partnerCrypto);

		//wait for the account message
		QVERIFY(partner->waitForReply<WelcomeMessage>([&](WelcomeMessage message, bool &ok) {
			QVERIFY(!message.hasChanges);//Must have changes now
			QCOMPARE(message.keyIndex, 0u);
			QVERIFY(message.scheme.isNull());
			QVERIFY(message.key.isNull());
			QVERIFY(message.cmac.isNull());
			ok = true;
		}));
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testChangeKeyInvalidIndex()
{
	try {
		QVERIFY(partner);

		//Send the key change proposal
		partner->send(KeyChangeMessage { 42 });

		QVERIFY(partner->waitForError(ErrorMessage::KeyIndexError));
		clean(partner);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testChangeKeyDuplicate()
{
	quint32 nextIndex = 1;//duplicate to current

	try {
		QVERIFY(client);

		//Send the key change proposal
		client->send(KeyChangeMessage { nextIndex });
		QVERIFY(client->waitForReply<DeviceKeysMessage>([&](DeviceKeysMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, nextIndex);
			QVERIFY(message.duplicated);
			QVERIFY(message.devices.isEmpty());
			ok = true;
		}));

		//make shure no disconnect
		QVERIFY(client->waitForNothing());
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testChangeKeyPendingChanges()
{
	quint32 nextIndex = 2;//valid
	QByteArray scheme = "scheme2";
	QByteArray key = "key2";
	QByteArray cmac = "cmac2";

	try {
		QVERIFY(client);
		QVERIFY(!partner);

		//Send the key change proposal
		client->send(KeyChangeMessage { nextIndex });
		QVERIFY(client->waitForReply<DeviceKeysMessage>([&](DeviceKeysMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, nextIndex);
			QVERIFY(!message.duplicated);
			//skip key checks
			ok = true;
		}));

		//send the actual key update message (ignoring the other 3)
		NewKeyMessage keyMsg;
		keyMsg.keyIndex = nextIndex;
		keyMsg.cmac = devName.toUtf8();
		keyMsg.scheme = scheme;
		keyMsg.deviceKeys.append(std::make_tuple(partnerDevId, key, cmac));
		client->sendSigned(keyMsg, crypto);

		//wait for ack
		QVERIFY(client->waitForReply<NewKeyAckMessage>([&](NewKeyAckMessage message, bool &ok) {
			QCOMPARE(message.keyIndex, nextIndex);
			ok = true;
		}));

		//try to send another keychange
		client->send(KeyChangeMessage { ++nextIndex });
		QVERIFY(client->waitForError(ErrorMessage::KeyPendingError));//should have it's own error message
		clean(client);

		testLogin();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testAddNewKeyInvalidIndex()
{
	quint32 nextIndex = 42;//invalid index

	try {
		QVERIFY(client);

		NewKeyMessage keyMsg;
		keyMsg.keyIndex = nextIndex;
		keyMsg.cmac = "cmac";
		keyMsg.scheme = "scheme";
		keyMsg.deviceKeys.append(std::make_tuple(partnerDevId, "key", "cmac"));
		client->sendSigned(keyMsg, crypto);

		//make shure no disconnect
		QVERIFY(client->waitForError(ErrorMessage::KeyIndexError));
		clean(client);

		testLogin();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testAddNewKeyInvalidSignature()
{
	quint32 nextIndex = 3;//valid index

	try {
		QVERIFY(client);

		NewKeyMessage keyMsg;
		keyMsg.keyIndex = nextIndex;
		keyMsg.cmac = "cmac";
		keyMsg.scheme = "scheme";
		keyMsg.deviceKeys.append(std::make_tuple(partnerDevId, "key", "cmac"));
		auto sData = keyMsg.serializeSigned(crypto->privateSignKey(), crypto->rng(), crypto);
		sData[sData.size() - 1] = sData[sData.size() - 1] + 'x';
		client->sendBytes(sData); //message + fake signature

		//make shure no disconnect
		QVERIFY(client->waitForError(ErrorMessage::AuthenticationError));
		clean(client);

		testLogin();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testAddNewKeyPendingChanges()
{
	//theoretical situtation, that cannot occur normally
	quint32 nextIndex = 3;//valid index

	try {
		QVERIFY(client);

		NewKeyMessage keyMsg;
		keyMsg.keyIndex = nextIndex;
		keyMsg.cmac = "cmac";
		keyMsg.scheme = "scheme";
		keyMsg.deviceKeys.append(std::make_tuple(partnerDevId, "key", "cmac"));
		client->sendSigned(keyMsg, crypto);

		//make shure no disconnect
		QVERIFY(client->waitForError(ErrorMessage::ServerError, true));//is ok here, as this case is typically detected by the KeyChangeMessage.
		clean(client);

		testLogin();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testKeyChangeNoAck()
{
	quint32 nextIndex = 2;
	QByteArray scheme = "scheme2";
	QByteArray key = "key2";
	QByteArray cmac = "cmac2";

	try {
		QVERIFY(!partner);

		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message
		partner->sendSigned(LoginMessage {
							   partnerDevId,
							   partnerName,
							   mNonce
						   }, partnerCrypto);

		//wait for the account message
		QVERIFY(partner->waitForReply<WelcomeMessage>([&](WelcomeMessage message, bool &ok) {
			QVERIFY(!message.hasChanges);//Must have changes now
			QCOMPARE(message.keyIndex, nextIndex);
			QCOMPARE(message.scheme, scheme);
			QCOMPARE(message.key, key);
			QCOMPARE(message.cmac, cmac);
			ok = true;
		}));

		//do NOT send the mac, but reconnect
		clean(partner);
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message
		partner->sendSigned(LoginMessage {
							   partnerDevId,
							   partnerName,
							   mNonce
						   }, partnerCrypto);

		//wait for the account message
		QVERIFY(partner->waitForReply<WelcomeMessage>([&](WelcomeMessage message, bool &ok) {
			QVERIFY(!message.hasChanges);//Must have changes now
			QCOMPARE(message.keyIndex, nextIndex);
			QCOMPARE(message.scheme, scheme);
			QCOMPARE(message.key, key);
			QCOMPARE(message.cmac, cmac);
			ok = true;
		}));

		//update the mac accordingly
		partner->send(MacUpdateMessage { nextIndex, partnerName.toUtf8() });
		QVERIFY(partner->waitForReply<MacUpdateAckMessage>([&](MacUpdateAckMessage message, bool &ok) {
			Q_UNUSED(message)
			ok = true;
		}));
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testListAndRemoveDevices()
{
	try {
		QVERIFY(client);
		QVERIFY(partner);

		//list devices
		client->send(ListDevicesMessage {});
		QList<DevicesMessage::DeviceInfo> infos;
		QVERIFY(client->waitForReply<DevicesMessage>([&](DevicesMessage message, bool &ok) {
			QCOMPARE(message.devices.size(), 4); //testAddDevice, testAddDeviceInvalidKeyIndex, testSendDoubleAccept, testDeviceUploading
			infos = message.devices;
			ok = true;
		}));

		//verify the keys, verify partner, remove others
		auto ok = false;
		for(auto info : infos) {
			QVERIFY(std::get<0>(info) != devId);
			if(std::get<0>(info) == partnerDevId) {
				QCOMPARE(std::get<1>(info), partnerName);
				QCOMPARE(std::get<2>(info), partnerCrypto->ownFingerprint());
				ok = true;
			} else {
				client->send(RemoveMessage {std::get<0>(info)});
				QVERIFY(client->waitForReply<RemoveAckMessage>([&](RemoveAckMessage message, bool &ok) {
					QCOMPARE(message.deviceId, std::get<0>(info));
					ok = true;
				}));
			}
		}
		QVERIFY(ok);

		//list from partners side
		partner->send(ListDevicesMessage {});
		QVERIFY(partner->waitForReply<DevicesMessage>([&](DevicesMessage message, bool &ok) {
			QCOMPARE(message.devices.size(), 1);
			auto info = message.devices.first();
			QCOMPARE(std::get<0>(info), devId);
			QCOMPARE(std::get<1>(info), devName);
			QCOMPARE(std::get<2>(info), crypto->ownFingerprint());
			ok = true;
		}));

		//now delete partner
		client->send(RemoveMessage {partnerDevId});
		QVERIFY(client->waitForReply<RemoveAckMessage>([&](RemoveAckMessage message, bool &ok) {
			QCOMPARE(message.deviceId, partnerDevId);
			ok = true;
		}));

		//wait for partner disconnect and reconnect
		QVERIFY(partner->waitForDisconnect());
		partner->deleteLater();
		partner = new MockClient(this);
		QVERIFY(partner->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(partner->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message (but partner does not exist anymore
		partner->sendSigned(LoginMessage {
							   partnerDevId,
							   partnerName,
							   mNonce
						   }, partnerCrypto);

		QVERIFY(partner->waitForError(ErrorMessage::AuthenticationError));
		clean(partner);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testUnexpectedMessage_data()
{
	QTest::addColumn<QSharedPointer<Message>>("message");
	QTest::addColumn<bool>("whenIdle");
	QTest::addColumn<bool>("isSigned");

	QTest::newRow("RegisterMessage") << createNonced<RegisterMessage>()
									 << true
									 << true;
	QTest::newRow("LoginMessage") << createNonced<LoginMessage>(devId, devName, "nonce")
								  << true
								  << true;
	QTest::newRow("AccessMessage") << createNonced<AccessMessage>()
								   << true
								   << true;
	QTest::newRow("SyncMessage") << create<SyncMessage>()
								 << false
								 << false;
	QTest::newRow("ChangeMessage") << create<ChangeMessage>("data_id")
								   << false
								   << false;
	QTest::newRow("DeviceChangeMessage") << create<DeviceChangeMessage>("data_id", partnerDevId)
										 << false
										 << false;
	QTest::newRow("ChangedAckMessage") << create<ChangedAckMessage>(42ull)
									   << false
									   << false;
	QTest::newRow("ListDevicesMessage") << create<ListDevicesMessage>()
										<< false
										<< false;
	QTest::newRow("RemoveMessage") << create<RemoveMessage>(partnerDevId)
								   << false
								   << false;
	QTest::newRow("AcceptMessage") << create<AcceptMessage>(partnerDevId)
								   << false
								   << true;
	QTest::newRow("DenyMessage") << create<DenyMessage>(partnerDevId)
								 << false
								 << false;
	QTest::newRow("MacUpdateMessage") << create<MacUpdateMessage>(42, "cmac")
									  << false
									  << false;
	QTest::newRow("KeyChangeMessage") << create<KeyChangeMessage>(42)
									  << false
									  << false;
	QTest::newRow("NewKeyMessage") << create<NewKeyMessage>()
								   << false
								   << true;
}

void TestAppServer::testUnexpectedMessage()
{
	QFETCH(QSharedPointer<Message>, message);
	QFETCH(bool, whenIdle);
	QFETCH(bool, isSigned);

	QVERIFY(message);

	try {
		//assume already logged in
		QVERIFY(client);

		if(!whenIdle) {
			clean(client);
			client = new MockClient(this);
			QVERIFY(client->waitForConnected());

			//wait for identify message
			QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
				QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
				QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
				ok = true;
			}));
		}

		if(isSigned)
			client->sendSigned(*message, crypto);
		else
			client->send(*message);
		QVERIFY(client->waitForError(ErrorMessage::UnexpectedMessageError, true));

		clean(client);
		testLogin();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testUnknownMessage()
{
	try {
		//assume already logged in
		QVERIFY(client);

		client->send(LastChangedMessage()); //unknown of the server
		QVERIFY(client->waitForError(ErrorMessage::IncompatibleVersionError));

		clean(client);
		testLogin();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testBrokenMessage()
{
	try {
		//assume already logged in
		QVERIFY(client);

		client->sendBytes("\x00\x00\x00\x06Change\x00followed_by_gibberish");//header looks right, but not the rest
		QVERIFY(client->waitForError(ErrorMessage::ClientError));

		clean(client);
		testLogin();
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testRemoveSelf()
{
	try {
		QVERIFY(client);

		//list devices
		client->send(ListDevicesMessage {});
		QList<DevicesMessage::DeviceInfo> infos;
		QVERIFY(client->waitForReply<DevicesMessage>([&](DevicesMessage message, bool &ok) {
			QVERIFY(message.devices.isEmpty());
			ok = true;
		}));

		//now delete self
		client->send(RemoveMessage {devId});
		QVERIFY(client->waitForReply<RemoveAckMessage>([&](RemoveAckMessage message, bool &ok) {
			QCOMPARE(message.deviceId, devId);
			ok = true;
		}));

		clean(client);
		client = new MockClient(this);
		QVERIFY(client->waitForConnected());

		//wait for identify message
		QByteArray mNonce;
		QVERIFY(client->waitForReply<IdentifyMessage>([&](IdentifyMessage message, bool &ok) {
			QVERIFY(message.nonce.size() >= InitMessage::NonceSize);
			QCOMPARE(message.protocolVersion, InitMessage::CurrentVersion);
			mNonce = message.nonce;
			ok = true;
		}));

		//send back a valid login message (but partner does not exist anymore
		client->sendSigned(LoginMessage {
							   devId,
							   devName,
							   mNonce
						   }, crypto);

		QVERIFY(client->waitForError(ErrorMessage::AuthenticationError));
		clean(client);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
}

void TestAppServer::testStop()
{
	//send a signal to stop
#ifdef Q_OS_UNIX
	server->terminate(); //same as kill(SIGTERM)
#elif Q_OS_WIN
	GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, server->processId());
#endif
	QVERIFY(server->waitForFinished(5000));
	QCOMPARE(server->exitStatus(), QProcess::NormalExit);
	QCOMPARE(server->exitCode(), 0);
	server->close();
}

void TestAppServer::clean(bool disconnect)
{
	clean(client, disconnect);
}

void TestAppServer::clean(MockClient *&client, bool disconnect)
{
	if(!client)
		return;
	if(disconnect)
		client->close();
	QVERIFY(client->waitForDisconnect());
	client->deleteLater();
	client = nullptr;
}

template<typename TMessage, typename... Args>
inline QSharedPointer<Message> TestAppServer::create(Args... args)
{
	return QSharedPointer<TMessage>::create(args...).template staticCast<Message>();
}



template<typename TMessage, typename... Args>
inline QSharedPointer<Message> TestAppServer::createNonced(Args... args)
{
	auto msg = create<TMessage>(args...).template dynamicCast<InitMessage>();
	msg->nonce = QByteArray(InitMessage::NonceSize, 'x');
	return msg;
}

QTEST_MAIN(TestAppServer)

#include "tst_appserver.moc"
