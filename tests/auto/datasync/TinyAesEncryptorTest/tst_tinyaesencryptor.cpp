#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "tst.h"
#include "QtDataSync/private/qtinyaesencryptor_p.h"
using namespace QtDataSync;

class TinyAesEncryptorTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testEncryptionCycle();
	void testKeyChange();

private:
	QTinyAesEncryptor *encryptor;
};

void TinyAesEncryptorTest::initTestCase()
{
#ifdef Q_OS_LINUX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	encryptor = new QTinyAesEncryptor();

	//create setup to "init" both of them, but datasync itself is not used here
	Setup setup;
	mockSetup(setup);
	setup.setEncryptor(encryptor)
			.create();

	QThread::msleep(500);//wait for setup to complete, because of direct access
}

void TinyAesEncryptorTest::cleanupTestCase()
{
	Setup::removeSetup(Setup::DefaultSetup);
}

void TinyAesEncryptorTest::testEncryptionCycle()
{
	auto key = generateKey(42);
	auto data = generateDataJson(42);

	try {
		auto cipher = encryptor->encrypt(key, data, "id");
		QVERIFY(cipher.isObject());

		//test valid
		auto plain = encryptor->decrypt(key, cipher, "id");
		QCOMPARE(plain, data);

		//test invalid key
		QVERIFY_EXCEPTION_THROWN(encryptor->decrypt(key, cipher, "wrongType"), DecryptionFailedException);

		//test invalid data
		QVERIFY_EXCEPTION_THROWN(encryptor->decrypt(key, plain, "id"), DecryptionFailedException);

		//test invalid key
		encryptor->setKey(QByteArray("x").repeated(16));
		QVERIFY_EXCEPTION_THROWN(encryptor->decrypt(key, cipher, "id"), DecryptionFailedException);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TinyAesEncryptorTest::testKeyChange()
{
	try {
		auto newKey = QByteArray("y").repeated(16);//128 bit
		encryptor->setKey(newKey);
		QCOMPARE(encryptor->key(), newKey);

		QVERIFY_EXCEPTION_THROWN(encryptor->setKey("invalid"), InvalidKeyException);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(TinyAesEncryptorTest)

#include "tst_tinyaesencryptor.moc"
