#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <QtDataSync/private/cryptocontroller_p.h>
using namespace QtDataSync;

class TestCryptoController : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testBasic();

	void testClientCryptoKeys_data();
	void testClientCryptoKeys();
	void testClientCryptoOperations_data();
	void testClientCryptoOperations();

	//TODO test with all cipherschemes?
	void testKeyAccess();
	void testSymCrypto();

private:
	CryptoController *controller;

	void cryptoData();
};

void TestCryptoController::initTestCase()
{
	try {
		TestLib::init();
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		controller = new CryptoController(DefaultSetup, this);
		controller->initialize({});
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestCryptoController::cleanupTestCase()
{
	controller->finalize();
	delete controller;
	controller = nullptr;
	Setup::removeSetup(DefaultSetup, true);
}

void TestCryptoController::testBasic()
{
	QVERIFY(CryptoController::allKeystoreKeys().contains(QStringLiteral("plain")));
	QVERIFY(controller->canAccessStore());
}

void TestCryptoController::testClientCryptoKeys_data()
{
	cryptoData();
}

void TestCryptoController::testClientCryptoKeys()
{
	QFETCH(Setup::SignatureScheme, signScheme);
	QFETCH(QVariant, signParam);
	QFETCH(Setup::EncryptionScheme, cryptScheme);
	QFETCH(QVariant, cryptParam);
	QFETCH(bool, testSign);
	QFETCH(bool, testCrypt);

	auto crypto = new ClientCrypto(this);

	try {
		//generate (small keys to speed up test)
		crypto->generate(signScheme, signParam, cryptScheme, cryptParam);

		//sign key
		if(testSign) {
			auto data = crypto->writeSignKey();
			auto sKey = crypto->readKey(true, data);
			sKey->Validate(crypto->rng(), 3);
		}

		//crypt key
		if(testCrypt) {
			auto data = crypto->writeCryptKey();
			auto cKey = crypto->readKey(false, data);
			cKey->Validate(crypto->rng(), 3);
		}

		//private keys
		auto sScheme = crypto->signatureScheme();
		auto psKey = crypto->savePrivateSignKey();
		auto cScheme = crypto->encryptionScheme();
		auto pcKey = crypto->savePrivateCryptKey();

		//loading
		crypto->reset();
		crypto->load(sScheme, psKey, cScheme, pcKey);
	} catch(CryptoPP::Exception &e) {
		QFAIL(e.what());
	}

	crypto->deleteLater();
}

void TestCryptoController::testClientCryptoOperations_data()
{
	cryptoData();
}

void TestCryptoController::testClientCryptoOperations()
{
	QFETCH(Setup::SignatureScheme, signScheme);
	QFETCH(QVariant, signParam);
	QFETCH(Setup::EncryptionScheme, cryptScheme);
	QFETCH(QVariant, cryptParam);
	QFETCH(bool, testSign);
	QFETCH(bool, testCrypt);

	QByteArray message("some random message that should be processed");
	auto cryptoA = new ClientCrypto(this);
	auto cryptoB = new ClientCrypto(this);

	try {
		//generate (small keys to speed up test)
		cryptoA->generate(signScheme, signParam, cryptScheme, cryptParam);
		cryptoB->load(cryptoA->signatureScheme(),
					  cryptoA->savePrivateSignKey(),
					  cryptoA->encryptionScheme(),
					  cryptoA->savePrivateCryptKey());

		//signing
		if(testSign) {
			auto signature = cryptoA->sign(message);
			cryptoA->verify(cryptoA->signKey(), message, signature);
			cryptoB->verify(cryptoA->signKey(), message, signature);
			QVERIFY_EXCEPTION_THROWN(cryptoB->verify(cryptoA->signKey(), message + "a", signature), CryptoPP::Exception);
		}

		//encrypting
		if(testCrypt) {
			auto cipher = cryptoA->encrypt(cryptoB->cryptKey(), message);
			QCOMPARE(cryptoA->decrypt(cipher), message);
			QCOMPARE(cryptoB->decrypt(cipher), message);
			QVERIFY_EXCEPTION_THROWN(cryptoB->decrypt(cipher + "a"), CryptoPP::Exception);
		}
	} catch(CryptoPP::Exception &e) {
		QFAIL(e.what());
	}

	cryptoA->deleteLater();
	cryptoB->deleteLater();
}

void TestCryptoController::testKeyAccess()
{
	try {
		controller->clearKeyMaterial();

		auto testid = QUuid::createUuid();

		controller->createPrivateKeys("nonce");
		auto fPrint = controller->fingerprint();
		QVERIFY(!fPrint.isEmpty());

		controller->storePrivateKeys(testid);
		controller->clearKeyMaterial();
		QVERIFY_EXCEPTION_THROWN(controller->loadKeyMaterial(QUuid::createUuid()), KeyStoreException);
		controller->loadKeyMaterial(testid);
		QCOMPARE(controller->fingerprint(), fPrint);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestCryptoController::testSymCrypto()
{
	QByteArray message("another message to be processed");

	try {
		controller->clearKeyMaterial();
		controller->createPrivateKeys("nonce");

		//encryption
		quint32 index;
		QByteArray salt;
		QByteArray cipher;
		std::tie(index, salt, cipher) = controller->encrypt(message);
		QCOMPARE(controller->decrypt(index, salt, cipher), message);

		QVERIFY_EXCEPTION_THROWN(controller->decrypt(index + 1, salt, cipher), CryptoException);
		auto fakeSalt = salt;
		fakeSalt[2] = fakeSalt[2] + (char)1;
		QVERIFY_EXCEPTION_THROWN(controller->decrypt(index, fakeSalt, cipher), CryptoException);
		auto fakeMsg = cipher;
		fakeMsg[2] = fakeMsg[2] + (char)1;
		QVERIFY_EXCEPTION_THROWN(controller->decrypt(index, salt, fakeMsg), CryptoException);

		//cmac
		QByteArray mac;
		std::tie(index, mac) = controller->createCmac(message);
		controller->verifyCmac(index, message, mac);

		QVERIFY_EXCEPTION_THROWN(controller->verifyCmac(index + 1, message, mac), CryptoException);
		QVERIFY_EXCEPTION_THROWN(controller->verifyCmac(index, message + "a", mac), CryptoException);

	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestCryptoController::cryptoData()
{
	QTest::addColumn<Setup::SignatureScheme>("signScheme");
	QTest::addColumn<QVariant>("signParam");
	QTest::addColumn<Setup::EncryptionScheme>("cryptScheme");
	QTest::addColumn<QVariant>("cryptParam");
	QTest::addColumn<bool>("testSign");
	QTest::addColumn<bool>("testCrypt");

	QTest::newRow("RSA:2048") << Setup::RSA_PSS_SHA3_512
							  << QVariant(2048)
							  << Setup::RSA_OAEP_SHA3_512
							  << QVariant(2048)
							  << true
							  << true;

	QTest::newRow("ECDSA:brainpoolP256r1") << Setup::ECDSA_ECP_SHA3_512
										   << QVariant(Setup::brainpoolP256r1)
										   << Setup::RSA_OAEP_SHA3_512
										   << QVariant(512)
										   << true
										   << false;

	QTest::newRow("ECNR:secp256r1") << Setup::ECNR_ECP_SHA3_512
									<< QVariant(Setup::secp256r1)
									<< Setup::RSA_OAEP_SHA3_512
									<< QVariant(512)
									<< true
									<< false;
}

QTEST_MAIN(TestCryptoController)

#include "tst_cryptocontroller.moc"
