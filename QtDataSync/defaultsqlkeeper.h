#ifndef DEFAULTSQLKEEPER_H
#define DEFAULTSQLKEEPER_H

#include <QDir>
#include <QSqlDatabase>

namespace QtDataSync {

class DefaultSqlKeeper
{
public:
	static QDir storageDir(const QString &localDir);
	static QSqlDatabase aquireDatabase(const QString &localDir);
	static void releaseDatabase(const QString &localDir);

private:
	static const QString DatabaseName;
	static QHash<QString, quint64> refCounter;
};

}

#endif // DEFAULTSQLKEEPER_H
