#include <QtTest>
#include <QtDataSync/setup_impl.h>
#include <QtDataSync/authenticator.h>
#include <QtDataSync/private/firebaseauthenticator_p.h>
#include "testlib.h"
#include "anonauth.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

#define VERIFY_SPY(sigSpy, errorSpy) QVERIFY2(sigSpy.wait(), qUtf8Printable(errorSpy.value(0).value(0).toString()))

class AuthenticatorTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testLogin();
	void testRefresh();
	void testDeleteAccount();

private:
	QTemporaryDir _tDir;
	FirebaseAuthenticator *_authenticator = nullptr;
};

void AuthenticatorTest::initTestCase()
{
	auto setup = TestLib::loadSetup();
	QVERIFY(setup);
	QVERIFY(!setup->firebase.apiKey.isEmpty());
	_authenticator = new FirebaseAuthenticator {
		new AnonAuth{this},
		setup->firebase.apiKey,
		new QSettings{_tDir.filePath(QStringLiteral("config.ini")), QSettings::IniFormat, this},
		this
	};
}

void AuthenticatorTest::cleanupTestCase()
{
	if (_authenticator) {
		TestLib::rmAcc(_authenticator);
		_authenticator->deleteLater();
	}
}

void AuthenticatorTest::testLogin()
{
	QSignalSpy successSpy{_authenticator, &FirebaseAuthenticator::signInSuccessful};
	QVERIFY(successSpy.isValid());
	QSignalSpy failSpy{_authenticator, &FirebaseAuthenticator::signInFailed};
	QVERIFY(failSpy.isValid());

	// should start anon auth with success
	_authenticator->signIn();
	VERIFY_SPY(successSpy, failSpy);
	QCOMPARE(successSpy.size(), 1);
	QVERIFY(!successSpy[0][0].toString().isEmpty());
	QVERIFY(!successSpy[0][1].toString().isEmpty());
	const auto id = successSpy[0][0].toString();
	const auto token = successSpy[0][1].toString();

	// auth again should immediatly succed
	successSpy.clear();
	_authenticator->signIn();
	QCOMPARE(successSpy.size(), 1);
	QCOMPARE(successSpy[0][0].toString(), id);
	QCOMPARE(successSpy[0][1].toString(), token);
}

void AuthenticatorTest::testRefresh()
{
	// TODO
	QFAIL("not implemented");
}

void AuthenticatorTest::testDeleteAccount()
{
	QSignalSpy delSpy{_authenticator, &FirebaseAuthenticator::accountDeleted};
	QVERIFY(delSpy.isValid());
	_authenticator->deleteUser();
	QVERIFY(delSpy.wait());
	QCOMPARE(delSpy[0][0].toBool(), true);
}

QTEST_MAIN(AuthenticatorTest)

#include "tst_authenticator.moc"


