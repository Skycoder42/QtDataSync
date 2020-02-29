#include <QtTest>
#include <QtDataSync/Setup>
#include <QtDataSync/MailAuthenticator>
#include <QtDataSync/GoogleAuthenticator>
#include "extensions.h"

#include <QtDataSync/private/engine_p.h>
#include <QtDataSync/private/authenticator_p.h>
#include <QtDataSync/private/googleauthenticator_p.h>
using namespace QtDataSync;
using namespace std::chrono_literals;

Q_DECLARE_METATYPE(QtDataSync::ConfigType)

class SetupTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void testDefaults();
	void setSetters();
	void testExtensions();
	void testGoogleExtensions();
	void testAuthSelectionExtensions();

	void testConfigParsing_data();
	void testConfigParsing();

	void testKnownExtConfigParsing_data();
	void testKnownExtConfigParsing();

	void testException();

private:
	using ptr = QScopedPointer<Engine, QScopedPointerDeleteLater>;
};

class ExtractGoogleAuthenticator : public IAuthenticator
{
	Q_OBJECT

public:
	static GoogleAuthenticatorPrivate *extract(IAuthenticator *g) {
		const auto self = static_cast<ExtractGoogleAuthenticator*>(g);
		if (self)
			return static_cast<GoogleAuthenticatorPrivate*>(self->d_func());
		else
			return nullptr;
	}

private:
	Q_DECLARE_PRIVATE(IAuthenticator)
};

void SetupTest::testDefaults()
{
	try {
		const ptr engine{Setup<MailAuthenticator>().createEngine(this)};
		QVERIFY(engine);
		QCOMPARE(engine->parent(), this);

		const auto setup = EnginePrivate::getD(engine.data())->setup.data();
		QVERIFY(setup);
		QCOMPARE(setup->firebase.projectId, QString{});
		QCOMPARE(setup->firebase.apiKey, QString{});
		QCOMPARE(setup->firebase.readTimeOut, 15min);
		QCOMPARE(setup->firebase.readLimit, 100);
		QCOMPARE(setup->firebase.syncTableVersions, true);
		QCOMPARE(setup->database.transactionMode, TransactionMode::Default);
		QCOMPARE(setup->database.persistDeletes, true);
		QVERIFY(setup->settings);
		QCOMPARE(setup->settings->group(), QStringLiteral("qtdatasync"));
		QVERIFY(setup->nam);
		QVERIFY(!setup->sslConfig);
		QCOMPARE(setup->roUrl, QUrl{});
		QVERIFY(!setup->roNode);
		QVERIFY(setup->attachments.isEmpty());
		QVERIFY(qobject_cast<MailAuthenticator*>(engine->authenticator()));
		QVERIFY(qobject_cast<PlainCloudTransformer*>(engine->transformer()));
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void SetupTest::setSetters()
{
	try {
		const auto settings = new QSettings{this};
		const auto nam = new QNetworkAccessManager{this};
		auto conf = QSslConfiguration::defaultConfiguration();
		conf.setProtocol(QSsl::TlsV1_3);
		ptr engine {Setup<MailAuthenticator>()
					   .setFirebaseProjectId(QStringLiteral("project"))
					   .setFirebaseApiKey(QStringLiteral("key"))
					   .setRemoteReadTimeout(10s)
					   .setRemotePageLimit(5)
					   .setSyncTableVersions(false)
					   .setTransactionMode(TransactionMode::Exclusive)
					   .setPersistDeletes(false)
					   .setSettings(settings)
					   .setNetworkAccessManager(nam)
					   .setSslConfiguration(conf)
					   .setAsyncUrl(QStringLiteral("local:test"))
					   .createEngine(this)};
		QVERIFY(engine);
		QCOMPARE(engine->parent(), this);

		auto setup = EnginePrivate::getD(engine.data())->setup.data();
		QVERIFY(setup);
		QCOMPARE(setup->firebase.projectId, QStringLiteral("project"));
		QCOMPARE(setup->firebase.apiKey, QStringLiteral("key"));
		QCOMPARE(setup->firebase.readTimeOut, 10s);
		QCOMPARE(setup->firebase.readLimit, 5);
		QCOMPARE(setup->firebase.syncTableVersions, false);
		QCOMPARE(setup->database.transactionMode, TransactionMode::Exclusive);
		QCOMPARE(setup->database.persistDeletes, false);
		QCOMPARE(setup->settings, settings);
		QCOMPARE(setup->settings->parent(), engine.data());
		QCOMPARE(setup->settings->group(), QString{});
		QCOMPARE(setup->nam, nam);
		QCOMPARE(setup->nam->parent(), engine.data());
		QCOMPARE(setup->sslConfig, conf);
		QCOMPARE(setup->roUrl, QStringLiteral("local:test"));
		QVERIFY(setup->roNode);
		QCOMPARE(setup->roNode->parent(), engine.data());

		const auto roHost = new QRemoteObjectHost{QStringLiteral("local:test"), this};
		engine.reset(Setup<MailAuthenticator>()
						 .setAsyncUrl(QStringLiteral("local:false"))
						 .setAsyncHost(roHost)
						 .createEngine(this));
		QVERIFY(engine);
		QCOMPARE(engine->parent(), this);

		setup = EnginePrivate::getD(engine.data())->setup.data();
		QCOMPARE(setup->roUrl, QUrl{});
		QVERIFY(setup->roNode);
		QCOMPARE(setup->roNode->parent(), engine.data());
		QCOMPARE(setup->roNode, roHost);
		QCOMPARE(roHost->hostUrl(), QStringLiteral("local:test"));
		QVERIFY(setup);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void SetupTest::testExtensions()
{
	try {
		const ptr engine{Setup<DummyAuth, DummyTransform>()
							 .setAuthValue(42)
							 .setTransformValue(24)
							 .createEngine(this)};
		QVERIFY(engine);
		QCOMPARE(engine->parent(), this);

		const auto auth = qobject_cast<DummyAuth*>(engine->authenticator());
		QVERIFY(auth);
		QCOMPARE(auth->value, 42);
		const auto transform = qobject_cast<DummyTransform*>(engine->transformer());
		QVERIFY(transform);
		QCOMPARE(transform->value, 24);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void SetupTest::testGoogleExtensions()
{
	try {
		// google auth
		ptr engine{Setup<GoogleAuthenticator>()
					   .setOAuthClientId(QStringLiteral("id"))
					   .setOAuthClientSecret(QStringLiteral("secret"))
					   .setOAuthClientCallbackPort(42)
					   .createEngine(this)};
		QVERIFY(engine);
		QCOMPARE(engine->parent(), this);
		QVERIFY(qobject_cast<GoogleAuthenticator*>(engine->authenticator()));
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void SetupTest::testAuthSelectionExtensions()
{
	try {
		// auth select
		Setup<AuthenticationSelector<GoogleAuthenticator, MailAuthenticator, DummyAuth>> setup;
		setup.setOAuthClientId(QStringLiteral("id"));
		setup.setAuthValue(24);
		setup.authExtension<GoogleAuthenticator>()
			.setOAuthClientSecret(QStringLiteral("secret"));
		setup.authExtension<DummyAuth>()
			.setAuthValue(42);
		const ptr engine {setup.createEngine(this)};
		QVERIFY(engine);
		QCOMPARE(engine->parent(), this);

		auto auth = dynamic_cast<AuthenticationSelector<GoogleAuthenticator, MailAuthenticator, DummyAuth>*>(engine->authenticator());
		QVERIFY(auth);
		QSignalSpy storedSpy{auth, &AuthenticationSelectorBase::selectionStoredChanged};
		QVERIFY(storedSpy.isValid());
		QSignalSpy selectedSpy{auth, &AuthenticationSelectorBase::selectedChanged};
		QVERIFY(selectedSpy.isValid());

		auth->setSelectionStored(false);
		QCOMPARE(auth->isSelectionStored(), false);
		QCOMPARE(storedSpy.size(), 1);
		QCOMPARE(storedSpy.takeFirst()[0].toBool(), false);

		QCOMPARE(auth->selectionTypes().size(), 3);
		QVERIFY(auth->selectionTypes().contains(qMetaTypeId<GoogleAuthenticator*>()));
		QVERIFY(auth->selectionTypes().contains(qMetaTypeId<MailAuthenticator*>()));
		QVERIFY(auth->selectionTypes().contains(qMetaTypeId<DummyAuth*>()));
		QVERIFY(qobject_cast<GoogleAuthenticator*>(auth->authenticator<GoogleAuthenticator>()));
		QVERIFY(qobject_cast<MailAuthenticator*>(auth->authenticator<MailAuthenticator>()));
		const auto dAuth = qobject_cast<DummyAuth*>(auth->authenticator<DummyAuth>());
		QVERIFY(dAuth);
		QCOMPARE(dAuth->value, 42);
		QCOMPARE(auth->selected(), nullptr);
		QCOMPARE(auth->select<DummyAuth>(false), dAuth);
		QCOMPARE(auth->selected(), dAuth);
		QCOMPARE(selectedSpy.size(), 1);
		QCOMPARE(selectedSpy.takeFirst()[0].value<IAuthenticator*>(), dAuth);

		QCOMPARE(auth->isSelectionStored(), false);
		auth->setSelectionStored(true);
		QCOMPARE(auth->isSelectionStored(), true);
		QCOMPARE(storedSpy.size(), 1);
		QCOMPARE(storedSpy.takeFirst()[0].toBool(), true);
		auth->setSelectionStored(false);
		QCOMPARE(auth->isSelectionStored(), false);
		QCOMPARE(storedSpy.size(), 1);
		QCOMPARE(storedSpy.takeFirst()[0].toBool(), false);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void SetupTest::testConfigParsing_data()
{
	QTest::addColumn<ConfigType>("type");
	QTest::addColumn<QByteArray>("input");
	QTest::addColumn<QString>("projectId");
	QTest::addColumn<QString>("apiKey");
	QTest::addColumn<int>("authValue");
	QTest::addColumn<int>("transformValue");
	QTest::addColumn<bool>("throws");

	QTest::addRow("empty.web") << ConfigType::WebConfig
							   << QByteArray{"{}"}
							   << QString{}
							   << QString{}
							   << -1
							   << -1
							   << false;

	QTest::addRow("empty.gs.json") << ConfigType::GoogleServicesJson
								   << QByteArray{"{}"}
								   << QString{}
								   << QString{}
								   << -1
								   << -1
								   << false;

#ifdef Q_OS_DARWIN
	QTest::addRow("empty.gs.plist") << ConfigType::GoogleServicesPlist
									<< QByteArray{}
									<< QString{}
									<< QString{}
									<< -1
									<< -1
									<< false;
#endif

	QTest::addRow("filled.web") << ConfigType::WebConfig
								<< QByteArray{R"__({
  "apiKey": "key",
  "projectId": "id",
  "DummyAuth": 5,
  "DummyTransform": 10
})__"}
								<< QStringLiteral("id")
								<< QStringLiteral("key")
								<< 5
								<< 10
								<< false;

	QTest::addRow("filled.gs.json") << ConfigType::GoogleServicesJson
									<< QByteArray{R"__({
  "project_info": {
	"project_id": "id"
  },
  "client": [
	{
	  "api_key": [
		{
		  "current_key": "key"
		}
	  ]
	}
  ],
  "configuration_version": "1",
  "DummyAuth": 5,
  "DummyTransform": 10
})__"}
									<< QStringLiteral("id")
									<< QStringLiteral("key")
									<< 6
									<< 11
									<< false;

#ifdef Q_OS_DARWIN
	QTest::addRow("filled.gs.plist") << ConfigType::GoogleServicesPlist
									 << QByteArray{R"__(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>API_KEY</key>
	<string>key</string>
	<key>PLIST_VERSION</key>
	<string>1</string>
	<key>PROJECT_ID</key>
	<string>id</string>
	<key>DummyAuth</key>
	<integer>5</integer>
	<key>DummyTransform</key>
	<integer>10</integer>
</dict>
</plist>)__"}
									 << QStringLiteral("id")
									 << QStringLiteral("key")
									 << 7
									 << 12
									 << false;
#endif

	QTest::addRow("invalid.web") << ConfigType::WebConfig
								 << QByteArray{}
								 << QString{}
								 << QString{}
								 << 0
								 << 0
								 << true;

	QTest::addRow("invalid.gs.json") << ConfigType::GoogleServicesJson
									 << QByteArray{}
									 << QString{}
									 << QString{}
									 << 0
									 << 0
									 << true;

	QTest::addRow("invalid.gs.plist") << ConfigType::GoogleServicesPlist
									  << QByteArray{}
									  << QString{}
									  << QString{}
									  << 0
									  << 0
									  << true;
}

void SetupTest::testConfigParsing()
{
	QFETCH(ConfigType, type);
	QFETCH(QByteArray, input);
	QFETCH(QString, projectId);
	QFETCH(QString, apiKey);
	QFETCH(int, authValue);
	QFETCH(int, transformValue);
	QFETCH(bool, throws);

	try {
		QScopedPointer<QIODevice, QScopedPointerDeleteLater> device;
		if (type == ConfigType::GoogleServicesPlist && !throws) {
			auto tFile = new QTemporaryFile{this};
			QVERIFY2(tFile->open(), qUtf8Printable(tFile->errorString()));
			tFile->write(input);
			tFile->close();
			device.reset(tFile);
		} else {
			device.reset(new QBuffer{&input, this});
		}
		QVERIFY2(device->open(QIODevice::ReadOnly | QIODevice::Text), qUtf8Printable(device->errorString()));

		if (throws) {
			const auto expr = [&]() { return Setup<DummyAuth, DummyTransform>::fromConfig(device.data(), type); };
			QVERIFY_EXCEPTION_THROWN(expr(), Exception);
		} else {
			const ptr engine {Setup<DummyAuth, DummyTransform>::fromConfig(device.data(), type)
								 .createEngine(this)};
			QVERIFY(engine);
			QCOMPARE(engine->parent(), this);

			const auto setup = EnginePrivate::getD(engine.data())->setup.data();
			QVERIFY(setup);
			QCOMPARE(setup->firebase.projectId, projectId);
			QCOMPARE(setup->firebase.apiKey, apiKey);
			const auto auth = qobject_cast<DummyAuth*>(engine->authenticator());
			QVERIFY(auth);
			QCOMPARE(auth->value, authValue);
			const auto transform = qobject_cast<DummyTransform*>(engine->transformer());
			QVERIFY(transform);
			QCOMPARE(transform->value, transformValue);
		}
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void SetupTest::testKnownExtConfigParsing_data()
{
	QTest::addColumn<ConfigType>("type");
	QTest::addColumn<QByteArray>("input");
	QTest::addColumn<QString>("projectId");
	QTest::addColumn<QString>("apiKey");
	QTest::addColumn<QString>("clientId");
	QTest::addColumn<QString>("secret");
	QTest::addColumn<int>("port");

	QTest::addRow("empty.web") << ConfigType::WebConfig
							   << QByteArray{"{}"}
							   << QString{}
							   << QString{}
							   << QString{}
							   << QString{}
							   << 0;

	QTest::addRow("empty.gs.json") << ConfigType::GoogleServicesJson
								   << QByteArray{"{}"}
								   << QString{}
								   << QString{}
								   << QString{}
								   << QString{}
								   << 0;

#ifdef Q_OS_DARWIN
	QTest::addRow("empty.gs.plist") << ConfigType::GoogleServicesPlist
									<< QByteArray{}
									<< QString{}
									<< QString{}
									<< QString{}
									<< QString{}
									<< 0;
#endif

	QTest::addRow("filled.web") << ConfigType::WebConfig
								<< QByteArray{R"__({
  "apiKey": "key",
  "projectId": "id",
  "oAuth": {
	"clientID": "client",
	"clientSecret": "secret",
	"callbackPort": 4200
  }
})__"}
								<< QStringLiteral("id")
								<< QStringLiteral("key")
								<< QStringLiteral("client")
								<< QStringLiteral("secret")
								<< 4200;

	QTest::addRow("filled.gs.json") << ConfigType::GoogleServicesJson
									<< QByteArray{R"__({
  "project_info": {
	"project_id": "id"
  },
  "client": [
	{
	  "oauth_client": [
		{
		  "client_id": "client",
		  "client_secret": "secret",
		  "callback_port": 4200
		}
	  ],
	  "api_key": [
		{
		  "current_key": "key"
		}
	  ]
	}
  ],
  "configuration_version": "1"
})__"}
									<< QStringLiteral("id")
									<< QStringLiteral("key")
									<< QStringLiteral("client")
									<< QStringLiteral("secret")
									<< 4200;

#ifdef Q_OS_DARWIN
	QTest::addRow("filled.gs.plist") << ConfigType::GoogleServicesPlist
									 << QByteArray{R"__(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>API_KEY</key>
	<string>key</string>
	<key>PLIST_VERSION</key>
	<string>1</string>
	<key>PROJECT_ID</key>
	<string>id</string>
	<key>CLIENT_ID</key>
	<string>client</string>
	<key>CLIENT_SECRET</key>
	<string>secret</string>
	<key>CALLBACK_PORT</key>
	<integer>4200</integer>
</dict>
</plist>)__"}
									 << QStringLiteral("id")
									 << QStringLiteral("key")
									 << QStringLiteral("client")
									 << QStringLiteral("secret")
									 << 4200;
#endif
}

void SetupTest::testKnownExtConfigParsing()
{
	QFETCH(ConfigType, type);
	QFETCH(QByteArray, input);
	QFETCH(QString, projectId);
	QFETCH(QString, apiKey);
	QFETCH(QString, clientId);
	QFETCH(QString, secret);
	QFETCH(int, port);

	try {
		QScopedPointer<QIODevice, QScopedPointerDeleteLater> device;
		if (type == ConfigType::GoogleServicesPlist) {
			auto tFile = new QTemporaryFile{this};
			QVERIFY2(tFile->open(), qUtf8Printable(tFile->errorString()));
			tFile->write(input);
			tFile->close();
			device.reset(tFile);
		} else {
			device.reset(new QBuffer{&input, this});
		}
		QVERIFY2(device->open(QIODevice::ReadOnly | QIODevice::Text), qUtf8Printable(device->errorString()));

		const ptr engine {Setup<AuthenticationSelector<GoogleAuthenticator, MailAuthenticator>>::fromConfig(device.data(), type)
							 .createEngine(this)};
		QVERIFY(engine);
		QCOMPARE(engine->parent(), this);

		const auto setup = EnginePrivate::getD(engine.data())->setup.data();
		QVERIFY(setup);
		QCOMPARE(setup->firebase.projectId, projectId);
		QCOMPARE(setup->firebase.apiKey, apiKey);
		const auto sAuth = qobject_cast<AuthenticationSelectorBase*>(engine->authenticator());
		QVERIFY(sAuth);
		const auto gAuth = ExtractGoogleAuthenticator::extract(sAuth->authenticator(qMetaTypeId<GoogleAuthenticator*>()));
		QVERIFY(gAuth);
		QCOMPARE(gAuth->oAuthFlow->clientIdentifier(), clientId);
		QCOMPARE(gAuth->oAuthFlow->clientIdentifierSharedKey(), secret);
		if (port == 0)
			QVERIFY(gAuth->oAuthFlow->requestUrl().port() != port);
		else
			QCOMPARE(gAuth->oAuthFlow->requestUrl().port(), port);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void SetupTest::testException()
{
	try {
		Setup<DummyAuth> setup;
		setup.setInvalid();
		QVERIFY_EXCEPTION_THROWN(setup.createEngine(this), SetupException);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(SetupTest)

#include "tst_setup.moc"
