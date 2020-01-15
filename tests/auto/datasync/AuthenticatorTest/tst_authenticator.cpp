#include <QtTest>
#include <QtDataSync/authenticator.h>
#include <QtDataSync/private/firebaseauthenticator_p.h>
using namespace QtDataSync;
using namespace std::chrono_literals;

class AuthenticatorTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

private:
	QTemporaryDir _tDir;
	FirebaseAuthenticator *_authenticator = nullptr;
};

void AuthenticatorTest::initTestCase()
{
}

void AuthenticatorTest::cleanupTestCase()
{
}

QTEST_MAIN(AuthenticatorTest)

#include "tst_authenticator.moc"


