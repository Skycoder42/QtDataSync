#include <QtTest>
#include <QtDataSync>

#include <QtDataSync/private/engine_p.h>
#include <QtDataSync/private/remoteconnector_p.h>

#include "anonauth.h"
using namespace QtDataSync;

class Extractor : public Engine
{
public:
	static EnginePrivate *extract(Engine *engine) {
		return static_cast<Extractor*>(engine)->d_func();
	}

	EnginePrivate *d_func() {
		Q_CAST_IGNORE_ALIGN(return reinterpret_cast<EnginePrivate*>(qGetPtrHelper(d_ptr)););
	}
};

class RemoteConnectorTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testUploadData();

private:
	QTemporaryDir _tDir;
	Engine *_engine = nullptr;
	QPointer<RemoteConnector> _connector;

	void doAuth();
};

void RemoteConnectorTest::initTestCase()
{
	qRegisterMetaType<ObjectKey>();  // TODO move to datasync

	try {
		_engine = Setup::fromConfig(QStringLiteral(SRCDIR "../../../ci/web-config.json"), Setup::ConfigType::WebConfig)
					  .setAuthenticator(new AnonAuth{})
					  .setSettings(new QSettings{_tDir.filePath(QStringLiteral("config.ini")), QSettings::IniFormat})
					  .createEngine(this);
		_connector = Extractor::extract(_engine)->connector;
		doAuth();
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void RemoteConnectorTest::cleanupTestCase()
{
	if (_connector) {
		QSignalSpy delSpy{_connector, &RemoteConnector::removedUser};
		_connector->removeUser();
		delSpy.wait();
	}

	_connector.clear();
	if (_engine)
		_engine->deleteLater();
}

void RemoteConnectorTest::testUploadData()
{
	QSignalSpy uploadSpy{_connector, &RemoteConnector::uploadedData};
	QVERIFY(uploadSpy.isValid());

	// upload unchanged data
	for (auto i = 0; i < 10; ++i) {
		const ObjectKey key {QStringLiteral("RmcTestData"), QString::number(i)};
		const auto modified = QDateTime::currentDateTimeUtc();
		CloudData data;
		data.setKey(key);
		data.setData(QJsonObject {
			{QStringLiteral("Key"), i},
			{QStringLiteral("Value"), QStringLiteral("data_%1").arg(i)}
		});
		data.setModified(modified);
		_connector->uploadChange(data);

		QVERIFY(uploadSpy.wait());
		QCOMPARE(uploadSpy.size(), 1);
		QCOMPARE(uploadSpy[0][0].value<ObjectKey>(), key);
		QCOMPARE(uploadSpy[0][1].value<QDateTime>(), modified);
		uploadSpy.clear();
	}
}

void RemoteConnectorTest::doAuth()
{
	auto auth = _engine->authenticator();
	QSignalSpy signInSpy{auth, &IAuthenticator::signInSuccessful};
	QVERIFY(signInSpy.isValid());
	QSignalSpy errorSpy{auth, &IAuthenticator::signInFailed};
	QVERIFY(errorSpy.isValid());

	auth->signIn();
	QVERIFY(signInSpy.wait());
	QCOMPARE(signInSpy.size(), 1);
	QCOMPARE(errorSpy.size(), 0);

	_connector->setUser(signInSpy[0][0].toString());
	_connector->setIdToken(signInSpy[0][1].toString());
	QVERIFY(_connector->isActive());
}

QTEST_MAIN(RemoteConnectorTest)

#include "tst_remoteconnector.moc"


