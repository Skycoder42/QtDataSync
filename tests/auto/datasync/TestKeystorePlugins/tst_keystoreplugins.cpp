#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>

#include <QtDataSync/private/defaults_p.h>
using namespace QtDataSync;

class TestKeystorePlugins : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testKeystoreFunctions_data();
	void testKeystoreFunctions();
};

void TestKeystorePlugins::initTestCase()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
	try {
		TestLib::init();
		Setup setup;
		TestLib::setup(setup);
		setup.create();
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestKeystorePlugins::cleanupTestCase()
{
	Setup::removeSetup(DefaultSetup, true);
}

void TestKeystorePlugins::testKeystoreFunctions_data()
{
	QTest::addColumn<QString>("pluginname");
	QTest::addColumn<QString>("provider");
	QTest::addColumn<bool>("required");

	QTest::newRow("plain") << QStringLiteral("qplain")
						   << QStringLiteral("plain")
						   << true;
	QTest::newRow("kwallet") << QStringLiteral("qkwallet")
							 << QStringLiteral("kwallet")
							 << false;
	QTest::newRow("secretservice") << QStringLiteral("qsecretservice")
								   << QStringLiteral("secretservice")
								   << false;
	QTest::newRow("gnome-keyring") << QStringLiteral("qsecretservice")
								   << QStringLiteral("gnome-keyring")
								   << false;
#ifdef Q_OS_WIN
	QTest::newRow("wincred") << QStringLiteral("qwincred")
							 << QStringLiteral("wincred")
							 << true;
#endif
#ifdef Q_OS_DARWIN
	QTest::newRow("keychain") << QStringLiteral("qkeychain")
							  << QStringLiteral("keychain")
							  << true;
#endif
#ifdef Q_OS_ANDROID
	QTest::newRow("android") << QStringLiteral("qandroid")
							 << QStringLiteral("android")
							 << true;
#endif
}

void TestKeystorePlugins::testKeystoreFunctions()
{
	QFETCH(QString, pluginname);
	QFETCH(QString, provider);
	QFETCH(bool, required);

#ifdef Q_OS_WIN
#ifdef QT_NO_DEBUG
	QString fullPath = QStringLiteral(KEYSTORE_PATH) + pluginname;
#else
	QString fullPath = QStringLiteral(KEYSTORE_PATH) + pluginname + QStringLiteral("d");
#endif
#else
	QString fullPath = QStringLiteral(KEYSTORE_PATH) + QStringLiteral("lib") + pluginname;
#endif

	QPluginLoader loader;
	loader.setFileName(fullPath);

	if(!loader.load()) {
		if(!required)
			QEXPECT_FAIL("", "Cannot test unavailable plugin", Abort);
		QVERIFY2(loader.isLoaded(), qUtf8Printable(loader.errorString()));
	}

	auto pluginObj = loader.instance();
	QVERIFY(pluginObj);
	auto plugin = qobject_cast<KeyStorePlugin*>(pluginObj);
	QVERIFY(plugin);

	if(!plugin->keystoreAvailable(provider)) {
		if(!required)
			QEXPECT_FAIL("", "Cannot test unavailable provider", Abort);
		QVERIFY2(plugin->keystoreAvailable(provider), "Plugin can be loaded, but expected provider is not available");
	}

	//verify datasync nows about the plugin
	QVERIFY(Setup::keystoreAvailable(provider));

	auto key = QStringLiteral("some_random_key");
	QByteArray data = "random_secret_private_key";
	try {
		auto instance = plugin->createInstance(provider, DefaultsPrivate::obtainDefaults(DefaultSetup), this);
		QVERIFY(instance);
		QCOMPARE(instance->providerName(), provider);

		//open the store
		QVERIFY(!instance->isOpen());
		instance->openStore();
		QVERIFY(instance->isOpen());

		//small cleanup
		if(instance->contains(key))
			instance->remove(key);
		QVERIFY(!instance->contains(key));

		//try to store a private key (as random data)
		instance->save(key, data);
		QVERIFY(instance->contains(key));
		QCOMPARE(instance->load(key), data);

		//close and reopen (with a new instance)
		instance->closeStore();
		QVERIFY(!instance->isOpen());
		delete instance;
		instance = nullptr;

		instance = plugin->createInstance(provider, DefaultsPrivate::obtainDefaults(DefaultSetup), this);
		QVERIFY(instance);
		QCOMPARE(instance->providerName(), provider);

		QVERIFY(!instance->isOpen());
		instance->openStore();
		QVERIFY(instance->isOpen());

		//load again, then remove
		QVERIFY(instance->contains(key));
		QCOMPARE(instance->load(key), data);
		instance->remove(key);
		QVERIFY(!instance->contains(key));

		//final close
		instance->closeStore();
		QVERIFY(!instance->isOpen());

		delete instance;
	} catch(QException &e) {
		QFAIL(e.what());
	}

	delete pluginObj;
	loader.unload();
}

QTEST_MAIN(TestKeystorePlugins)

#include "tst_keystoreplugins.moc"
