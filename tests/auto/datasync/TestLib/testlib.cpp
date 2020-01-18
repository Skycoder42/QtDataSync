#include "testlib.h"
#include <QtTest>
#include <QtDataSync/authenticator.h>
#define private public
#include <QtDataSync/setup_impl.h>
#undef private
#include "anonauth.h"
#include <QtDataSync/private/firebaseauthenticator_p.h>
using namespace QtDataSync;
using namespace QtDataSync::__private;

namespace {

class DummyExtender : public __private::SetupExtensionPrivate
{
public:
	QObject *createInstance(QObject *) override {
		return nullptr;
	}
};

QTemporaryDir tDir;

}

std::optional<SetupPrivate> TestLib::loadSetup(const QString &path)
{
	std::optional<SetupPrivate> setup;
	[&](){
		try {
			QFile configFile{path};
			QVERIFY(configFile.exists());
			QVERIFY2(configFile.open(QIODevice::ReadOnly | QIODevice::Text), qUtf8Printable(configFile.errorString()));
			setup = __private::SetupPrivate{};
			setup->_authExt = new DummyExtender{};
			setup->_transExt = new DummyExtender{};
			setup->readWebConfig(&configFile);
		} catch (std::exception &e) {
			setup.reset();
			QFAIL(e.what());
		}
	}();
	return setup;
}

FirebaseAuthenticator *TestLib::createAuth(const SetupPrivate &setup, QObject *parent)
{
	return new FirebaseAuthenticator {
		new AnonAuth{parent},
		setup.firebase.apiKey,
		new QSettings{tDir.filePath(QStringLiteral("config.ini")), QSettings::IniFormat, parent},
		parent
	};
}

std::optional<std::pair<QString, QString>> TestLib::doAuth(FirebaseAuthenticator *auth)
{
	std::optional<std::pair<QString, QString>> res;
	[&](){
		QSignalSpy signInSpy{auth, &FirebaseAuthenticator::signInSuccessful};
		QVERIFY(signInSpy.isValid());
		QSignalSpy errorSpy{auth, &FirebaseAuthenticator::signInFailed};
		QVERIFY(errorSpy.isValid());

		auth->signIn();
		QVERIFY(signInSpy.wait());
		QCOMPARE(signInSpy.size(), 1);
		QCOMPARE(errorSpy.size(), 0);

		res = std::make_pair(signInSpy[0][0].toString(),
							 signInSpy[0][1].toString());

	}();
	return res;
}

void TestLib::rmAcc(FirebaseAuthenticator *auth)
{
	QSignalSpy accDelSpy{auth, &FirebaseAuthenticator::accountDeleted};
	auth->deleteUser();
	if (!accDelSpy.isEmpty() || accDelSpy.wait())
		qDebug() << "deleteAcc" << accDelSpy[0][0].toBool();
	else
		qDebug() << "deleteAcc did not fire";
}
