#ifndef DEFAULTSQLKEEPER_H
#define DEFAULTSQLKEEPER_H

#include <QDir>
#include <QSqlDatabase>

namespace QtDataSync {

class DefaultSqlKeeper
{
public:
	static QDir storageDir();
	static QSqlDatabase aquireDatabase();
	static void releaseDatabase();

private:
	static quint64 refCounter;
	static const QString DatabaseName;
};

}

#endif // DEFAULTSQLKEEPER_H
