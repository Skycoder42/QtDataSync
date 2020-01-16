#ifndef TESTLIB_H
#define TESTLIB_H

#include <optional>

#include <QtCore/QString>

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
	static void rmAcc(QtDataSync::FirebaseAuthenticator *auth);
};

#endif // TESTLIB_H
