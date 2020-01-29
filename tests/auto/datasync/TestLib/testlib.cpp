#include "testlib.h"
#include <QtTest>
#include "anonauth.h"
#include <QtDataSync/private/firebaseauthenticator_p.h>
using namespace QtDataSync;
using namespace QtDataSync::__private;

namespace {

QTemporaryDir tDir;

}

QSettings *TestLib::createSettings(QObject *parent)
{
	return new QSettings{tDir.filePath(QStringLiteral("config.ini")), QSettings::IniFormat, parent};
}

FirebaseAuthenticator *TestLib::createAuth(const QString &apiKey, QObject *parent, QNetworkAccessManager *nam, QSettings *settings)
{
	return new FirebaseAuthenticator {
		new AnonAuth{parent},
		apiKey,
		settings ? settings : createSettings(parent),
		nam ? nam : new QNetworkAccessManager{parent},
		std::nullopt,
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
