#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <QtDataSync/private/cryptocontroller_p.h>

//fake private
#define private public
#include <QtDataSync/private/defaults_p.h>
#undef private
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

	void testKeyAccess();
	void testSymCrypto_data();
	void testSymCrypto();

	void testKeyExchange();

	void testPwCrypto_data();
	void testPwCrypto();

private:
	CryptoController *controller;

	void cryptoData();
	void symData();
};

void TestCryptoController::initTestCase()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
	QVERIFY(qputenv("PLUGIN_KEYSTORES_PATH", PLUGIN_DIR));

	try {
		TestLib::init();
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		controller = new CryptoController(DefaultsPrivate::obtainDefaults(DefaultSetup), this);
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
	try {
		QVERIFY(CryptoController::allKeystoreKeys().contains(QStringLiteral("plain")));
		controller->acquireStore(false);
	} catch(std::exception &e) {
		QFAIL(e.what());
	}
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
	} catch(std::exception &e) {
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
			QVERIFY_EXCEPTION_THROWN(cryptoB->verify(cryptoA->signKey(), message + "a", signature), std::exception);
		}

		//encrypting
		if(testCrypt) {
			auto cipher = cryptoA->encrypt(cryptoB->cryptKey(), message);
			QCOMPARE(cryptoA->decrypt(cipher), message);
			QCOMPARE(cryptoB->decrypt(cipher), message);
			QVERIFY_EXCEPTION_THROWN(cryptoB->decrypt(cipher + "a"), std::exception);
		}
	} catch(std::exception &e) {
		QFAIL(e.what());
	}

	cryptoA->deleteLater();
	cryptoB->deleteLater();
}

void TestCryptoController::testKeyAccess()
{
	QSignalSpy fPrintSpy(controller, &CryptoController::fingerprintChanged);

	try {
		controller->clearKeyMaterial();
		QCOMPARE(fPrintSpy.size(), 1);

		auto testid = QUuid::createUuid();

		controller->createPrivateKeys("nonce");
		auto fPrint = controller->fingerprint();
		QVERIFY(!fPrint.isEmpty());
		QCOMPARE(fPrintSpy.size(), 2);

		controller->storePrivateKeys(testid);

		controller->clearKeyMaterial();
		QVERIFY(controller->fingerprint().isEmpty());
		QCOMPARE(fPrintSpy.size(), 3);
		QVERIFY_EXCEPTION_THROWN(controller->loadKeyMaterial(QUuid::createUuid()), KeyStoreException);

		controller->loadKeyMaterial(testid);
		QCOMPARE(controller->fingerprint(), fPrint);
		QCOMPARE(fPrintSpy.size(), 4);

		controller->deleteKeyMaterial(testid);
		QVERIFY(controller->fingerprint().isEmpty());
		QCOMPARE(fPrintSpy.size(), 5);
		QVERIFY_EXCEPTION_THROWN(controller->loadKeyMaterial(testid), KeyStoreException);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestCryptoController::testSymCrypto_data()
{
	symData();
}

void TestCryptoController::testSymCrypto()
{
	QFETCH(Setup::CipherScheme, scheme);

	QByteArray message("another message to be processed");

	try {
		controller->clearKeyMaterial();

		auto dPriv = DefaultsPrivate::obtainDefaults(DefaultSetup);
		dPriv->properties.insert(Defaults::SymScheme, scheme);

		controller->createPrivateKeys("nonce");

		//encryption
		quint32 index;
		QByteArray salt;
		QByteArray cipher;
		std::tie(index, salt, cipher) = controller->encryptData(message);
		QCOMPARE(controller->decryptData(index, salt, cipher), message);

		QVERIFY_EXCEPTION_THROWN(controller->decryptData(index + 1, salt, cipher), CryptoException);
		auto fakeSalt = salt;
		fakeSalt[2] = fakeSalt[2] + (char)1;
		QVERIFY_EXCEPTION_THROWN(controller->decryptData(index, fakeSalt, cipher), CryptoException);
		auto fakeMsg = cipher;
		fakeMsg[2] = fakeMsg[2] + (char)1;
		QVERIFY_EXCEPTION_THROWN(controller->decryptData(index, salt, fakeMsg), CryptoException);

		//cmac
		QByteArray mac;
		mac = controller->createCmac(message);
		controller->verifyCmac(controller->keyIndex(), message, mac);

		QVERIFY_EXCEPTION_THROWN(controller->verifyCmac(index + 1, message, mac), CryptoException);
		QVERIFY_EXCEPTION_THROWN(controller->verifyCmac(index, message + "a", mac), CryptoException);

	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestCryptoController::testKeyExchange()
{
	try {
		controller->clearKeyMaterial();
		controller->createPrivateKeys("nonce");
		auto crypto = controller->crypto();

		//secret encryption
		auto cInfo = controller->encryptSecretKey(crypto, *(crypto->cryptKey()));
		QCOMPARE(std::get<0>(cInfo), controller->keyIndex());
		auto nIndex = controller->keyIndex() + 1;
		controller->decryptSecretKey(nIndex, std::get<1>(cInfo), std::get<2>(cInfo), false);
		QCOMPARE(controller->keyIndex(), nIndex);

		//crypt cmac stuff
		auto cmac = controller->generateEncryptionKeyCmac();
		controller->verifyEncryptionKeyCmac(crypto, *(crypto->cryptKey()), cmac);

		//key updates
		nIndex = controller->keyIndex();
		auto nKey = controller->generateNextKey();
		QCOMPARE(std::get<0>(nKey), nIndex + 1);
		QCOMPARE(controller->keyIndex(), nIndex);
		controller->activateNextKey(std::get<0>(nKey));
		QCOMPARE(controller->keyIndex(), std::get<0>(nKey));
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestCryptoController::testPwCrypto_data()
{
	symData();
}

void TestCryptoController::testPwCrypto()
{
	QFETCH(Setup::CipherScheme, scheme);

	QByteArray message("another message to be processed");
	QString password(QStringLiteral("super secure password"));

	try {
		auto dPriv = DefaultsPrivate::obtainDefaults(DefaultSetup);
		dPriv->properties.insert(Defaults::SymScheme, scheme);

		auto crypto = controller->crypto();
		AsymmetricCryptoInfo *asymInfo = new AsymmetricCryptoInfo(controller->rng(),
																  crypto->signatureScheme(),
																  crypto->writeSignKey(),
																  crypto->encryptionScheme(),
																  crypto->writeCryptKey());

		QByteArray schemeName;
		QByteArray salt;
		QByteArray cmac;
		QByteArray cCmac;
		QByteArray data;

		//side A
		{
			//generate key
			CryptoPP::SecByteBlock key;
			std::tie(schemeName, salt, key) = controller->generateExportKey(password);

			//cmac
			cmac = controller->createExportCmac(schemeName, key, message);
			controller->verifyImportCmac(schemeName, key, message, cmac);

			//cCmac
			cCmac = controller->createExportCmacForCrypto(schemeName, key);
			controller->verifyImportCmacForCrypto(schemeName, key, asymInfo, cCmac);

			//encrypt
			data = controller->exportEncrypt(schemeName, salt, key, message);
		}

		//side B
		{
			auto key = controller->recoverExportKey(schemeName, salt, password);
			auto otherKey = controller->recoverExportKey(schemeName, salt, QStringLiteral("wrong password"));

			//cmac
			controller->verifyImportCmac(schemeName, key, message, cmac);
			QVERIFY_EXCEPTION_THROWN(controller->verifyImportCmac(schemeName, otherKey, message, cmac), CryptoException);

			//cCmac
			controller->verifyImportCmacForCrypto(schemeName, key, asymInfo, cCmac);
			QVERIFY_EXCEPTION_THROWN(controller->verifyImportCmacForCrypto(schemeName, otherKey, asymInfo, cCmac), CryptoException);

			//encrypt
			auto res = controller->importDecrypt(schemeName, salt, key, data);
			QCOMPARE(res, message);
			QVERIFY_EXCEPTION_THROWN(controller->importDecrypt(schemeName, salt, otherKey, data), CryptoException);
		}
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
#if CRYPTOPP_VERSION > 600
										   << Setup::ECIES_ECP_SHA3_512
										   << QVariant(Setup::brainpoolP256r1)
#else
										   << Setup::RSA_OAEP_SHA3_512
										   << QVariant(2048)
#endif
										   << true
										   << false;

	QTest::newRow("ECNR:secp256r1") << Setup::ECNR_ECP_SHA3_512
									<< QVariant(Setup::secp256r1)
									<< Setup::RSA_OAEP_SHA3_512
									<< QVariant(2048)
									<< true
									<< false;
}

void TestCryptoController::symData()
{
	QTest::addColumn<Setup::CipherScheme>("scheme");

	QTest::newRow("AES_EAX") << Setup::AES_EAX;
	QTest::newRow("AES_GCM") << Setup::AES_GCM;
	QTest::newRow("TWOFISH_EAX") << Setup::TWOFISH_EAX;
	QTest::newRow("TWOFISH_GCM") << Setup::TWOFISH_GCM;
	QTest::newRow("SERPENT_EAX") << Setup::SERPENT_EAX;
	QTest::newRow("SERPENT_GCM") << Setup::SERPENT_GCM;
	QTest::newRow("IDEA_EAX") << Setup::IDEA_EAX;
}

QTEST_MAIN(TestCryptoController)

#include "tst_cryptocontroller.moc"
