#ifndef TESTLIB_H
#define TESTLIB_H

#include <optional>

#include <QtCore/QObject>

#include <QtNetwork/QNetworkAccessManager>

#ifndef SRCDIR
#define SRCDIR
#endif

namespace QtDataSync {
class FirebaseAuthenticator;
namespace __private {
class SetupPrivate;
}
}

class TestLib
{
public:
	static QtDataSync::FirebaseAuthenticator *createAuth(const QString &apiKey, QObject *parent, QNetworkAccessManager *nam = nullptr);
	static std::optional<std::pair<QString, QString>> doAuth(QtDataSync::FirebaseAuthenticator *auth);
	static void rmAcc(QtDataSync::FirebaseAuthenticator *auth);
};

#endif // TESTLIB_H
