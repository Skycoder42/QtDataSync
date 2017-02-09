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

	void initialize() override;
	void finalize() override;

	QList<ChangedInfo> listLocalChanges() override;
	void markLocalChanged(const QByteArray &typeName, const QString &key, ChangeState changed) override;
	void markAllLocalChanged(const QByteArray &typeName, ChangeState changed) override;

private:
	QSqlDatabase database;
};

}

#endif // SQLSTATEHOLDER_H
