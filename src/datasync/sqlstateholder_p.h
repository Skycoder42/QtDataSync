#ifndef SQLSTATEHOLDER_P_H
#define SQLSTATEHOLDER_P_H

#include "qdatasync_global.h"
#include "stateholder.h"

#include <QtCore/QObject>
#include <QtSql/QSqlDatabase>

namespace QtDataSync {

class Q_DATASYNC_EXPORT SqlStateHolder : public StateHolder
{
	Q_OBJECT

public:
	explicit SqlStateHolder(QObject *parent = nullptr);

	void initialize(Defaults *defaults) override;
	void finalize() override;

	ChangeHash listLocalChanges() override;
	void markLocalChanged(const ObjectKey &key, ChangeState changed) override;

private:
	Defaults *defaults;
	QSqlDatabase database;
};

}

#endif // SQLSTATEHOLDER_P_H
