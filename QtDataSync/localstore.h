#ifndef LOCALSTORE_H
#define LOCALSTORE_H

#include "qtdatasync_global.h"
#include <QObject>
#include <QString>
#include <QJsonValue>
#include <QJsonObject>

namespace QtDataSync {

class QTDATASYNCSHARED_EXPORT LocalStore : public QObject
{
	Q_OBJECT

public:
	explicit LocalStore(QObject *parent = nullptr);

	virtual void initialize();
	virtual void finalize();

	virtual void count(quint64 id, QByteArray typeName) = 0;
	virtual void loadAll(quint64 id, QByteArray typeName) = 0;
	virtual void load(quint64 id, QByteArray typeName, const QString &key, const QString &value) = 0;
	virtual void save(quint64 id, QByteArray typeName, const QString &key, const QString &value, const QJsonObject &object) = 0;
	virtual void remove(quint64 id, QByteArray typeName, const QString &key, const QString &value) = 0;
	virtual void removeAll(quint64 id, QByteArray typeName) = 0;

signals:
	void requestCompleted(quint64 id, const QJsonValue &result);
	void requestFailed(quint64 id, const QString &errorString);
};

}

#endif // LOCALSTORE_H
