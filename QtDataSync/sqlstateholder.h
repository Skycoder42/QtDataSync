#ifndef SQLSTATEHOLDER_H
#define SQLSTATEHOLDER_H

#include "stateholder.h"

#include <QObject>
#include <QSqlDatabase>

namespace QtDataSync {

class SqlStateHolder : public StateHolder
{
	Q_OBJECT

public:
	explicit SqlStateHolder(QObject *parent = nullptr);

	void initialize(const QDir &storageDir) override;
	void finalize(const QDir &storageDir) override;

	ChangeHash listLocalChanges() override;
	void markLocalChanged(const ObjectKey &key, ChangeState changed) override;

private:
	QSqlDatabase database;
	QDir storageDir;
};

}

#endif // SQLSTATEHOLDER_H
