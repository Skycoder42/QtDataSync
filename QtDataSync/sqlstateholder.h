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

	void initialize(const QString &localDir) override;
	void finalize(const QString &localDir) override;

	ChangeHash listLocalChanges() override;
	void markLocalChanged(const ObjectKey &key, ChangeState changed) override;

private:
	QSqlDatabase database;
};

}

#endif // SQLSTATEHOLDER_H
