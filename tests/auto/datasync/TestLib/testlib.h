#ifndef TESTLIB_H
#define TESTLIB_H

#include <optional>

#include <QtCore/QObject>

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
	static std::optional<QtDataSync::__private::SetupPrivate> loadSetup(const QString &path = QStringLiteral(SRCDIR "../../../ci/web-config.json"));

	static QtDataSync::FirebaseAuthenticator *createAuth(const QtDataSync::__private::SetupPrivate &setup, QObject *parent);
	static std::optional<std::pair<QString, QString>> doAuth(QtDataSync::FirebaseAuthenticator *auth);
	static void rmAcc(QtDataSync::FirebaseAuthenticator *auth);
};

#endif // TESTLIB_H
