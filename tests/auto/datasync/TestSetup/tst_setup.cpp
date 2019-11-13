#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
using namespace QtDataSync;

#include <QtDataSync/private/defaults_p.h>

class TestSetup : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testRatios();

	void testCreate();
	void testProperties();

private:
	class TestResolver : public GenericConflictResolver<TestData> {
	protected:
		TestData resolveConflict(TestData, TestData, QObject *) const override {
			throw NoConflictResultException{};
		}
	};
};

void TestSetup::initTestCase()
{

}

void TestSetup::cleanupTestCase()
{

}

void TestSetup::testRatios()
{
	QCOMPARE(KB(1), 1000);
	QCOMPARE(MB(1), KB(1000));
	QCOMPARE(GB(1), MB(1000));

#if __cplusplus >= 201402L
	using namespace QtDataSync::literals;

	QCOMPARE(1_kb, KB(1));
	QCOMPARE(1_mb, MB(1));
	QCOMPARE(1_gb, GB(1));
#endif
}

void TestSetup::testCreate()
{
	try {
		Setup aSetup;
		const auto aName = QStringLiteral("testCreate_active_setup");
		TestLib::setup(aSetup);
		aSetup.setLocalDir(aSetup.localDir() + QLatin1Char('/') + aName)
				.setRemoteObjectHost(QStringLiteral("threaded:tst_setup")); //NOTE Qt 5.12 should work with threaded variant too!!!

		QVERIFY(!Setup::exists(aName));
		aSetup.create(aName);
		QVERIFY(Setup::exists(aName));
		QVERIFY(DefaultsPrivate::obtainDefaults(aName));

		Setup pSetup;
		const auto pName = QStringLiteral("testCreate_passive_setup");
		TestLib::setup(pSetup);
		pSetup.setLocalDir(pSetup.localDir() + QLatin1Char('/') + pName)
				.setRemoteObjectHost(QStringLiteral("threaded:tst_setup"));

		QVERIFY(!Setup::exists(pName));
		pSetup.createPassive(pName, -1);
		QVERIFY(Setup::exists(pName));
		QVERIFY(DefaultsPrivate::obtainDefaults(pName));

		Setup::removeSetup(pName, true);
		Setup::removeSetup(aName, true);
	} catch(Exception &e) {
		QFAIL(e.what());
	}
}

void TestSetup::testProperties()
{
	try {
		//test generic property assignments
		const auto sName = QStringLiteral("testProperties_setup");
		Setup setup;
		auto ser = new QtJsonSerializer::JsonSerializer{this};
		auto res = new TestResolver{};
		auto ssl = QSslConfiguration::defaultConfiguration();
		ssl.setPeerVerifyMode(QSslSocket::VerifyPeer);

		TestLib::setup(setup);
		setup.setLocalDir(setup.localDir() + QLatin1Char('/') + sName)
				.setRemoteObjectHost(QStringLiteral("local:tst_setup"))
				.setSerializer(ser)
				.setConflictResolver(res)
				.setCacheSize(42000)
				.setPersistDeletedVersion(true)
				.setSyncPolicy(Setup::PreferDeleted)
				.setSslConfiguration(ssl)
				.setRemoteConfiguration(RemoteConfig{QStringLiteral("wss://example.com")})
				.setCipherScheme(Setup::TWOFISH_GCM)
				.setCipherKeySize(24)
				.setEventLoggingMode(Setup::EventMode::Disabled);

		QCOMPARE(setup.localDir(), TestLib::tDir.path() + QLatin1Char('/') + sName);
		QCOMPARE(setup.remoteObjectHost(), QStringLiteral("local:tst_setup"));
		QCOMPARE(setup.serializer(), ser);
		QCOMPARE(setup.conflictResolver(), res);
		QCOMPARE(setup.cacheSize(), 42000);
		QCOMPARE(setup.persistDeletedVersion(), true);
		QCOMPARE(setup.syncPolicy(), Setup::PreferDeleted);
		QCOMPARE(setup.sslConfiguration(), ssl);
		QCOMPARE(setup.remoteConfiguration(), RemoteConfig{QStringLiteral("wss://example.com")});
		QCOMPARE(setup.keyStoreProvider(), QStringLiteral("plain"));
		QCOMPARE(setup.signatureScheme(), Setup::RSA_PSS_SHA3_512);
		QCOMPARE(setup.signatureKeyParam(), QVariant{2048});
		QCOMPARE(setup.encryptionScheme(), Setup::RSA_OAEP_SHA3_512);
		QCOMPARE(setup.encryptionKeyParam(), QVariant{2048});
		QCOMPARE(setup.cipherScheme(), Setup::TWOFISH_GCM);
		QCOMPARE(setup.cipherKeySize(), 24);
		QCOMPARE(setup.eventLoggingMode(), Setup::EventMode::Disabled);

		//test transfer to defaults
		setup.create(sName);
		QVERIFY(Setup::exists(sName));

		Defaults defaults{DefaultsPrivate::obtainDefaults(sName)};
		QVERIFY(defaults.isValid());

		// compare transfered data properties
		QCOMPARE(defaults.setupName(), sName);
		QCOMPARE(defaults.storageDir().absolutePath(), setup.localDir());
		QCOMPARE(defaults.remoteAddress(), QStringLiteral("local:tst_setup"));
		QCOMPARE(defaults.serializer(), ser);
		QCOMPARE(defaults.conflictResolver(), res);
		QCOMPARE(defaults.property(Defaults::CacheSize), QVariant::fromValue(setup.cacheSize()));
		QCOMPARE(defaults.property(Defaults::PersistDeleted), QVariant::fromValue(setup.persistDeletedVersion()));
		QCOMPARE(defaults.property(Defaults::ConflictPolicy), QVariant::fromValue(setup.syncPolicy()));
		QCOMPARE(defaults.property(Defaults::SslConfiguration), QVariant::fromValue(setup.sslConfiguration()));
		QCOMPARE(defaults.property(Defaults::RemoteConfiguration), QVariant::fromValue(setup.remoteConfiguration()));
		QCOMPARE(defaults.property(Defaults::KeyStoreProvider), QVariant::fromValue(setup.keyStoreProvider()));
		QCOMPARE(defaults.property(Defaults::SignScheme), QVariant::fromValue(setup.signatureScheme()));
		QCOMPARE(defaults.property(Defaults::SignKeyParam), setup.signatureKeyParam());
		QCOMPARE(defaults.property(Defaults::CryptScheme), QVariant::fromValue(setup.encryptionScheme()));
		QCOMPARE(defaults.property(Defaults::CryptKeyParam), setup.encryptionKeyParam());
		QCOMPARE(defaults.property(Defaults::SymScheme), QVariant::fromValue(setup.cipherScheme()));
		QCOMPARE(defaults.property(Defaults::SymKeyParam), QVariant::fromValue(setup.cipherKeySize()));
		QCOMPARE(defaults.property(Defaults::EventLoggingMode), QVariant::fromValue(setup.eventLoggingMode()));

		// test other defaults stuff
		QVERIFY(defaults.remoteNode());
		QVERIFY(defaults.createSettings(this));
		QVERIFY(defaults.createLogger("TestSetup", this));
		{
			auto dbRef = defaults.aquireDatabase(this);
			QVERIFY(dbRef.isValid());
			QVERIFY(dbRef.database().isOpen());
			dbRef.drop();
			QVERIFY(!dbRef.isValid());
		}

		//Cleanup
		defaults.drop();
		Setup::removeSetup(sName, true);
	} catch(Exception &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(TestSetup)

#include "tst_setup.moc"
