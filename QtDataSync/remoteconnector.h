#ifndef REMOTECONNECTOR_H
#define REMOTECONNECTOR_H

#include "qtdatasync_global.h"
#include "stateholder.h"
#include <QObject>

namespace QtDataSync {

class Authenticator;

class QTDATASYNCSHARED_EXPORT RemoteConnector : public QObject
{
	Q_OBJECT

public:
	explicit RemoteConnector(QObject *parent = nullptr);

	virtual void initialize();
	virtual void finalize();

	virtual Authenticator *createAuthenticator(QObject *parent) = 0;

public slots:
	virtual void download(const QByteArray &typeName, const QString &key, const QString &value) = 0;
	virtual void upload(const QByteArray &typeName, const QString &key, const QString &value, const QJsonObject &object) = 0;
	virtual void remove(const QByteArray &typeName, const QString &key, const QString &value) = 0;
	virtual void markUnchanged(const QByteArray &typeName, const QString &key, const QString &value) = 0;

signals:
	void remoteStateChanged(bool canUpdate, const QList<StateHolder::ChangedInfo> &remoteChanges);
	void operationDone(const QByteArray &typeName, const QString &key, const QString &value, const QJsonValue &result);

protected:
	virtual QByteArray loadDeviceId();
	virtual void resetDeviceId();
};

}

#endif // REMOTECONNECTOR_H
