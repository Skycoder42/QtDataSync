#include <QtTest>
#include <QtDataSync/setup_impl.h>
#include <QtDataSync/authenticator.h>
#define private public
#include <QtDataSync/private/firebaseauthenticator_p.h>
#undef private
#include "testlib.h"
#include "anonauth.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

class AuthenticatorTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testLogin();
	void testRefresh();
	void testLogOut();
	void testDeleteAccount();

private:
	FirebaseAuthenticator *_authenticator = nullptr;
};

void AuthenticatorTest::initTestCase()
{
	_authenticator = TestLib::createAuth(QStringLiteral(FIREBASE_API_KEY), this);
	QVERIFY(_authenticator);
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
	QVERIFY(successSpy.wait());
	QCOMPARE(successSpy.size(), 1);
	QVERIFY(!successSpy[0][0].toString().isEmpty());
	QVERIFY(!successSpy[0][1].toString().isEmpty());
	QVERIFY(failSpy.isEmpty());
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
	QSignalSpy successSpy{_authenticator, &FirebaseAuthenticator::signInSuccessful};
	QVERIFY(successSpy.isValid());
	QSignalSpy failSpy{_authenticator, &FirebaseAuthenticator::signInFailed};
	QVERIFY(failSpy.isValid());

	// set expired
	_authenticator->_refreshTimer->stop();
	_authenticator->_expiresAt = QDateTime::currentDateTimeUtc().addSecs(-5);

	// trigger refresh
	const auto oldId = _authenticator->_localId;
	_authenticator->signIn();
	QVERIFY(successSpy.wait());
	QCOMPARE(successSpy.size(), 1);
	QEXPECT_FAIL("", "Anonymous authentications cannot be refreshed", Abort);
	QCOMPARE(successSpy[0][0].toString(), oldId);
	QVERIFY(!successSpy[0][1].toString().isEmpty());
	QVERIFY(failSpy.isEmpty());

	// trigger automated refresh
	_authenticator->_expiresAt = QDateTime::currentDateTimeUtc().addSecs(63); // 1min 3s
	_authenticator->_refreshTimer->start(3s);
	QVERIFY(successSpy.wait(8000));
	QCOMPARE(successSpy.size(), 1);
	QCOMPARE(successSpy[0][0].toString(), oldId);
	QVERIFY(!successSpy[0][1].toString().isEmpty());
	QVERIFY(failSpy.isEmpty());
}

void AuthenticatorTest::testLogOut()
{
	const auto token = _authenticator->_idToken;
	_authenticator->logOut();

	// delete acc should fail now, as not logged in
	QSignalSpy delSpy{_authenticator, &FirebaseAuthenticator::accountDeleted};
	QVERIFY(delSpy.isValid());
	_authenticator->deleteUser();
	QCOMPARE(delSpy.size(), 1);
	QCOMPARE(delSpy[0][0].toBool(), false);

	//set token again
	_authenticator->_idToken = token;
}

void AuthenticatorTest::testDeleteAccount()
{
	QSignalSpy delSpy{_authenticator, &FirebaseAuthenticator::accountDeleted};
	QVERIFY(delSpy.isValid());
	_authenticator->deleteUser();
	if (delSpy.isEmpty())
		QVERIFY(delSpy.wait());
	QCOMPARE(delSpy[0][0].toBool(), true);
}

QTEST_MAIN(AuthenticatorTest)

#include "tst_authenticator.moc"


