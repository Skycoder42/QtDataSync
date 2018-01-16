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

using namespace QtDataSync;

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

	void testAddDeviceUntrusted();

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

	void clean(bool disconnect = true);
	void clean(MockClient *client, bool disconnect = true);
};

void TestAppServer::initTestCase()
{
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

void TestAppServer::testAddDeviceUntrusted()
{
	QByteArray pNonce = "partner_nonce";
	QByteArray macscheme = "macscheme";
	QByteArray cmac = "cmac";
	QByteArray trustmac = "trustmac";
	quint32 keyIndex = 0;//TODO test what happens if 5 is send -> should fail?
	QByteArray keyScheme = "keyScheme";
	QByteArray keySecret = "keySecret";

	try {
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
						  keyIndex, //TODO test what happens if different
						  partnerDevId.toByteArray() //dummy cmac
					  });

		//wait for mac ack
		QVERIFY(partner->waitForReply<MacUpdateAckMessage>([&](MacUpdateAckMessage message, bool &ok) {
			Q_UNUSED(message);
			ok = true;
		}));

		clean(partner);
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

void TestAppServer::clean(MockClient *client, bool disconnect)
{
	if(!client)
		return;
	if(disconnect)
		client->close();
	QVERIFY(client->waitForDisconnect());
	client->deleteLater();
	client = nullptr;
}

QTEST_MAIN(TestAppServer)

#include "tst_appserver.moc"
